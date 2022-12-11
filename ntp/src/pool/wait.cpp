#include "pool/wait.hpp"
#include "details/utils.hpp"
#include "logger/logger_internal.hpp"


namespace ntp::wait::details {

WaitManager::WaitManager(PTP_CALLBACK_ENVIRON environment)
    : BasicManagerEx(environment)
{ }

void WaitManager::SubmitInternal(native_handle_t native_handle, object_context_t& object_context) noexcept
{
    PFILETIME wait_timeout = (object_context.wait_timeout.has_value())
                               ? &object_context.wait_timeout.value()
                               : nullptr;

    ntp::details::SafeThreadpoolCall<SetThreadpoolWait>(
        native_handle, object_context.wait_handle, wait_timeout);
}

/* static */
void NTAPI WaitManager::InvokeCallback(PTP_CALLBACK_INSTANCE instance, context_pointer_t context, PTP_WAIT wait, TP_WAIT_RESULT wait_result) noexcept
{
    try
    {
        if (!context)
        {
            throw exception::Win32Exception(ERROR_INVALID_PARAMETER);
        }

        //
        // BUGBUG: need to think about exceptions in user-defined callback
        // Probably need to implement something like std::async here
        //

        context->callback->Call(instance, &wait_result);

        //
        // Clean object here
        //

        CleanupContext(instance, context);
    }
    catch (const std::exception& error)
    {
        logger::details::Logger::Instance().TraceMessage(logger::Severity::kError, error.what());
    }
    catch (...)
    {
        logger::details::Logger::Instance().TraceMessage(logger::Severity::kCritical,
            L"[WaitManager::InvokeCallback]: unknown error");
    }
}

/* static */
void WaitManager::CloseInternal(native_handle_t native_handle) noexcept
{
    if (!native_handle)
    {
        //
        // Just to be confident if we are working with a valid object
        //

        return;
    }

    ntp::details::SafeThreadpoolCall<SetThreadpoolWait>(native_handle, nullptr, nullptr);
    ntp::details::SafeThreadpoolCall<WaitForThreadpoolWaitCallbacks>(native_handle, TRUE);
    ntp::details::SafeThreadpoolCall<CloseThreadpoolWait>(native_handle);
}

}  // namespace ntp::wait::details
