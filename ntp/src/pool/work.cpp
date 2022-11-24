#include "pool/work.hpp"
#include "details/exception.hpp"
#include "logger/logger_internal.hpp"


namespace ntp::work::details {

Manager::Manager(PTP_CALLBACK_ENVIRON environment, const ntp::details::test_cancel_t& test_cancel)
    : BasicManager(environment)
    , queue_()
    , work_()
    , done_event_(TRUE, FALSE)
    , test_cancel_(test_cancel)
{
    work_ = CreateThreadpoolWork(reinterpret_cast<PTP_WORK_CALLBACK>(InvokeCallback),
        static_cast<PSLIST_HEADER>(queue_), Environment());

    if (!work_)
    {
        throw exception::Win32Exception();
    }
}

Manager::~Manager()
{
    if (work_)
    {
        ntp::details::SafeThreadpoolCall<WaitForThreadpoolWorkCallbacks>(work_, TRUE);
        ntp::details::SafeThreadpoolCall<CloseThreadpoolWork>(std::exchange(work_, nullptr));
    }

    if (queue_)
    {
        ClearList();
    }
}

bool Manager::WaitAll() noexcept
{
    //
    // Suppose we have something working...
    //

    done_event_.Reset();

    //
    // Now
    //

    const auto submitted = TrySubmitThreadpoolCallback(
        reinterpret_cast<PTP_SIMPLE_CALLBACK>(WaitAllCallback), this, Environment());

    if /* [[unlikely]] */ (!submitted)
    {
        logger::details::Logger::Instance().TraceMessage(logger::Severity::kError,
            L"[Manager::WaitAll]: cannot wait in separate thread, waiting in current one, cancellation is unavailable");

        ntp::details::SafeThreadpoolCall<WaitForThreadpoolWorkCallbacks>(work_, FALSE);
        done_event_.Set();
    }

    bool cancelled = false;

    while (WAIT_TIMEOUT == WaitForSingleObject(done_event_, ntp::details::kTestCancelTimeout))
    {
        if (test_cancel_())
        {
            CancelAll();
            cancelled = true;
        }
    }

    logger::details::Logger::Instance().TraceMessage(logger::Severity::kExtended,
        L"[Manager::WaitAll]: wait completed");

    return !cancelled;
}

void Manager::CancelAll() noexcept
{
    ntp::details::SafeThreadpoolCall<WaitForThreadpoolWorkCallbacks>(work_, TRUE);
    done_event_.Set();

    size_t left_unprocessed = ClearList();
    logger::details::Logger::Instance().TraceMessage(logger::Severity::kNormal,
        L"[Manager::CancelAll]: tasks cancelled and %1!zu! left unprocessed", left_unprocessed);
}

size_t Manager::ClearList() noexcept
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
void NTAPI Manager::InvokeCallback(PTP_CALLBACK_INSTANCE instance, PSLIST_HEADER queue, PTP_WORK work)
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

        std::unique_ptr<ntp::details::ICallback> callback(
            static_cast<ntp::details::ICallback*>(entry));

        callback->Call(nullptr);
    }
    catch (const std::exception& error)
    {
        logger::details::Logger::Instance().TraceMessage(logger::Severity::kError, error.what());
    }
    catch (...)
    {
        logger::details::Logger::Instance().TraceMessage(logger::Severity::kCritical,
            L"[Manager::InvokeCallback]: unknown error");
    }
}

/* static */
void CALLBACK Manager::WaitAllCallback(PTP_CALLBACK_INSTANCE instance, Manager* self)
{
    logger::details::Logger::Instance().TraceMessage(logger::Severity::kExtended,
        L"[Manager::WaitAllCallback]: wait started");

    if (self)
    {
        SetEventWhenCallbackReturns(instance, self->done_event_);
        CallbackMayRunLong(instance);

        //
        // And now I wait for callbacks
        //

        ntp::details::SafeThreadpoolCall<WaitForThreadpoolWorkCallbacks>(self->work_, FALSE);
    }
    else
    {
        logger::details::Logger::Instance().TraceMessage(logger::Severity::kError,
            L"[Manager::WaitAllCallback]: pointer to manager is NULL");
    }

    logger::details::Logger::Instance().TraceMessage(logger::Severity::kExtended,
        L"[Manager::WaitAllCallback]: wait finished");
}

}  // namespace ntp::work::details
