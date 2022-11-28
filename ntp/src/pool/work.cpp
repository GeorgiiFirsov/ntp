#include "pool/work.hpp"
#include "details/exception.hpp"
#include "logger/logger_internal.hpp"


namespace ntp::work::details {

WorkManager::WorkManager(PTP_CALLBACK_ENVIRON environment)
    : BasicManager(environment)
    , queue_()
    , work_()
    , done_event_(TRUE, FALSE)
{
    work_ = CreateThreadpoolWork(reinterpret_cast<PTP_WORK_CALLBACK>(InvokeCallback),
        static_cast<PSLIST_HEADER>(queue_), Environment());

    if (!work_)
    {
        throw exception::Win32Exception();
    }
}

WorkManager::~WorkManager()
{
    CancelAll();

    //
    // Actually we dont need to free PTP_WORK here,
    // because it will be freed via cleanup group.
    //
}

bool WorkManager::WaitAll(const ntp::details::test_cancel_t& test_cancel) noexcept
{
    //
    // Suppose we have something working...
    //

    done_event_.Reset();

    //
    // Now try to submit waiting callback
    //

    const auto submitted = TrySubmitThreadpoolCallback(
        reinterpret_cast<PTP_SIMPLE_CALLBACK>(WaitAllCallback), this, Environment());

    if /* [[unlikely]] */ (!submitted)
    {
        logger::details::Logger::Instance().TraceMessage(logger::Severity::kError,
            L"[WorkManager::WaitAll]: cannot wait in separate thread, waiting in current one, cancellation is unavailable");

        ntp::details::SafeThreadpoolCall<WaitForThreadpoolWorkCallbacks>(work_, FALSE);
        done_event_.Set();
    }

    //
    // And... Wait for everithing to complete :)
    //

    bool cancelled = false;

    while (WAIT_TIMEOUT == WaitForSingleObject(done_event_, ntp::details::kTestCancelTimeout))
    {
        if (test_cancel())
        {
            CancelAll();
            cancelled = true;
        }
    }

    logger::details::Logger::Instance().TraceMessage(logger::Severity::kExtended,
        L"[WorkManager::WaitAll]: wait completed");

    return !cancelled;
}

void WorkManager::CancelAll() noexcept
{
    ntp::details::SafeThreadpoolCall<WaitForThreadpoolWorkCallbacks>(work_, TRUE);
    done_event_.Set();

    size_t left_unprocessed = ClearList();
    logger::details::Logger::Instance().TraceMessage(logger::Severity::kNormal,
        L"[WorkManager::CancelAll]: tasks cancelled and %1!zu! left unprocessed", left_unprocessed);
}

size_t WorkManager::ClearList() noexcept
{
    PSLIST_ENTRY entry = nullptr;
    size_t entries     = 0;

    for (entry = InterlockedPopEntrySList(queue_); entry;
         entry = InterlockedPopEntrySList(queue_), ++entries)
    {
        delete static_cast<ntp::details::ICallback*>(entry);
    }

    return entries;
}

/* static */
void NTAPI WorkManager::InvokeCallback(PTP_CALLBACK_INSTANCE instance, PSLIST_HEADER queue, PTP_WORK work) noexcept
{
    try
    {
        if (!queue)
        {
            throw exception::Win32Exception(ERROR_INVALID_PARAMETER);
        }

        auto entry = InterlockedPopEntrySList(queue);
        if (!entry)
        {
            throw exception::Win32Exception(ERROR_NO_MORE_ITEMS);
        }

        ntp::details::callback_t(
            static_cast<ntp::details::ICallback*>(entry))
            ->Call(instance, nullptr);
    }
    catch (const std::exception& error)
    {
        logger::details::Logger::Instance().TraceMessage(logger::Severity::kError, error.what());
    }
    catch (...)
    {
        logger::details::Logger::Instance().TraceMessage(logger::Severity::kCritical,
            L"[WorkManager::InvokeCallback]: unknown error");
    }
}

/* static */
void CALLBACK WorkManager::WaitAllCallback(PTP_CALLBACK_INSTANCE instance, WorkManager* self) noexcept
{
    logger::details::Logger::Instance().TraceMessage(logger::Severity::kExtended,
        L"[WorkManager::WaitAllCallback]: wait started");

    if (self)
    {
        ntp::details::SafeThreadpoolCall<SetEventWhenCallbackReturns>(instance, self->done_event_);
        ntp::details::SafeThreadpoolCall<CallbackMayRunLong>(instance);

        //
        // And now I wait for callbacks
        //

        ntp::details::SafeThreadpoolCall<WaitForThreadpoolWorkCallbacks>(self->work_, FALSE);
    }
    else
    {
        logger::details::Logger::Instance().TraceMessage(logger::Severity::kError,
            L"[WorkManager::WaitAllCallback]: pointer to manager is NULL");
    }

    logger::details::Logger::Instance().TraceMessage(logger::Severity::kExtended,
        L"[WorkManager::WaitAllCallback]: wait finished");
}

}  // namespace ntp::work::details
