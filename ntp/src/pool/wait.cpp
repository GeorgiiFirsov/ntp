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

void Manager::Cancel(HANDLE wait_object) noexcept
{
    std::unique_lock lock { lock_ };

    const auto native_handle = static_cast<native_handle_t>(wait_object);
    if (const auto callback = callbacks_.find(native_handle); callback != callbacks_.cend())
    {
        CloseWait(native_handle);
        callbacks_.erase(callback);
    }
}

void Manager::CancelAll() noexcept
{
    std::unique_lock lock { lock_ };
    std::unique_lock removal_ban { removal_permission_ };

    for (auto& [native_handle, _] : callbacks_)
    {
        CloseWait(native_handle);
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

HANDLE Manager::SubmitInternal(callbacks_t::iterator callback)
{
    auto& [native_handle, context] = *callback;

    PFILETIME wait_timeout = (context->wait_timeout.has_value())
                               ? &context->wait_timeout.value()
                               : nullptr;

    ntp::details::SafeThreadpoolCall<SetThreadpoolWait>(
        native_handle, context->wait_handle, wait_timeout);

    return native_handle;
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

        auto manager = context->meta_context.manager;
        return manager->Remove(context->meta_context.iterator);
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

}  // namespace ntp::wait::details
