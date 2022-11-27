#include "pool/wait.hpp"
#include "details/utils.hpp"
#include "details/exception.hpp"
#include "logger/logger_internal.hpp"


namespace ntp::wait::details {

Manager::Manager(PTP_CALLBACK_ENVIRON environment)
    : BasicManager(environment)
    , callbacks_()
    , lock_()
    , removal_permission_()
{ }

Manager::~Manager()
{
    CancelAll();
}

void Manager::Cancel(HANDLE wait_handle) noexcept
{
    std::unique_lock lock { lock_ };

    const auto callback = callbacks_.find(wait_handle);
    if (callback != callbacks_.cend())
    {
        CloseWait(callback->second->native_handle);
        callbacks_.erase(callback);
    }
}

void Manager::CancelAll() noexcept
{
    std::unique_lock lock { lock_ };
    std::unique_lock removal_ban { removal_permission_ };

    for (auto& [_, callback] : callbacks_)
    {
        CloseWait(callback->native_handle);
    }

    callbacks_.clear();
}

void Manager::Remove(callbacks_t::iterator callback) noexcept
{
    std::unique_lock lock { lock_ };

    if (removal_permission_)
    {
        //
        // We may iterate over callbacks_ when this function is called,
        // hence in this case we must keep container as is and not
        // remove callback
        //

        callbacks_.erase(callback);
    }
}

void Manager::SubmitInternal(Context& context) noexcept
{
    HANDLE wait_handle = context.meta.iterator->first;
    PFILETIME timeout  = context.wait_timeout.has_value()
                           ? &context.wait_timeout.value()
                           : nullptr;

    ntp::details::SafeThreadpoolCall<SetThreadpoolWait>(
        context.native_handle, wait_handle, timeout);
}

/* static */
void NTAPI Manager::InvokeCallback(PTP_CALLBACK_INSTANCE instance, Context* context, PTP_WAIT wait, TP_WAIT_RESULT wait_result) noexcept
{
    try
    {
        if (!context)
        {
            throw exception::Win32Exception(ERROR_INVALID_PARAMETER);
        }

        context->callback->Call(instance, reinterpret_cast<void*>(wait_result));

        //
        // BUGBUG: need to think about exceptions in user-defined callback
        // Probably need to implement something like std::async here
        //

        auto manager = context->meta.manager;
        manager->Remove(context->meta.iterator);
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
void Manager::CloseWait(PTP_WAIT wait) noexcept
{
    ntp::details::SafeThreadpoolCall<SetThreadpoolWait>(wait, nullptr, nullptr);
    ntp::details::SafeThreadpoolCall<WaitForThreadpoolWaitCallbacks>(wait, TRUE);
    ntp::details::SafeThreadpoolCall<CloseThreadpoolWait>(wait);
}

/* static */
void Manager::Wait(PTP_WAIT wait) noexcept
{
    ntp::details::SafeThreadpoolCall<WaitForThreadpoolWaitCallbacks>(wait, FALSE);
}

}  // namespace ntp::wait::details
