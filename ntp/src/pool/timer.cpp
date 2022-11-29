#include "pool/timer.hpp"
#include "details/utils.hpp"
#include "logger/logger_internal.hpp"


namespace ntp::timer::details {

TimerManager::TimerManager(PTP_CALLBACK_ENVIRON environment)
    : BasicManagerEx(environment)
{ }

void TimerManager::SubmitInternal(native_handle_t native_handle, object_context_t& object_context)
{
    ntp::details::SafeThreadpoolCall<SetThreadpoolTimer>(
        native_handle, &object_context.timer_timeout, object_context.timer_period, 0);
}

/* static */
void NTAPI TimerManager::InvokeCallback(PTP_CALLBACK_INSTANCE instance, context_pointer_t context, PTP_TIMER timer) noexcept
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

        context->callback->Call(instance, nullptr);

        //
        // Clean object here
        //

        if (0 == context->object_context.timer_period)
        {
            CleanupContext(instance, context);
        }
    }
    catch (const std::exception& error)
    {
        logger::details::Logger::Instance().TraceMessage(logger::Severity::kError, error.what());
    }
    catch (...)
    {
        logger::details::Logger::Instance().TraceMessage(logger::Severity::kCritical,
            L"[TimerManager::InvokeCallback]: unknown error");
    }
}

/* static */
void TimerManager::Close(native_handle_t native_handle) noexcept
{
    ntp::details::SafeThreadpoolCall<SetThreadpoolTimerEx>(native_handle, nullptr, 0, 0);
    ntp::details::SafeThreadpoolCall<WaitForThreadpoolTimerCallbacks>(native_handle, TRUE);
    ntp::details::SafeThreadpoolCall<CloseThreadpoolTimer>(native_handle);
}

}  // namespace ntp::timer::details
