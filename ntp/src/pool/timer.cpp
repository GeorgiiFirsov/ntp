#include "pool/timer.hpp"
#include "details/time.hpp"
#include "details/utils.hpp"
#include "logger/logger_internal.hpp"


namespace ntp::timer::details {

TimerManager::TimerManager(PTP_CALLBACK_ENVIRON environment)
    : BasicManagerEx(environment)
{ }

void TimerManager::SubmitInternal(native_handle_t native_handle, object_context_t& object_context)
{
    //
    // Here we need to decrease timeout, because this function may be called in replacement
    // function and some time was elapsed since this handle was submitted. If current timer
    // is being submitted for the first time, this step handles some delays elapsed after
    // callback creation.
    // BUGBUG: not implemented for now
    //

    //
    // Here we need relative timeout, so I will invert it (negative timeout represets a relative time interval):
    // https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-setthreadpooltimerex
    //

    FILETIME timeout = ntp::time::AsFileTime(object_context.timer_timeout);
    timeout          = ntp::time::Negate(timeout);

    const auto period = static_cast<DWORD>(object_context.timer_period.count());

    ntp::details::SafeThreadpoolCall<SetThreadpoolTimer>(native_handle, &timeout, period, 0);
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

        if (0 == context->object_context.timer_period.count())
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
void TimerManager::CloseInternal(native_handle_t native_handle) noexcept
{
    if (!native_handle)
    {
        //
        // Just to be confident if we are working with a valid object
        //

        return;
    }

    ntp::details::SafeThreadpoolCall<SetThreadpoolTimerEx>(native_handle, nullptr, 0, 0);
    ntp::details::SafeThreadpoolCall<WaitForThreadpoolTimerCallbacks>(native_handle, TRUE);
    ntp::details::SafeThreadpoolCall<CloseThreadpoolTimer>(native_handle);
}

}  // namespace ntp::timer::details
