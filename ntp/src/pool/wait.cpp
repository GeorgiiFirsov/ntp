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

    const auto callback = callbacks_.find(wait_object);
    if (callback != callbacks_.cend())
    {
        if (auto context = context_t::FromHandle(*callback); context)
        {
            CloseWait(context->native_handle);
            context_t::Destroy(context);
        }

        callbacks_.erase(callback);
    }
}

void Manager::CancelAll() noexcept
{
    std::unique_lock lock { lock_ };
    std::unique_lock removal_ban { removal_permission_ };

    for (HANDLE context_handle : callbacks_)
    {
        if (auto context = context_t::FromHandle(context_handle); context)
        {
            CloseWait(context->native_handle);
            context_t::Destroy(context);
        }
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

HANDLE Manager::SubmitInternal(HANDLE context_handle)
{
    if (auto context = context_t::FromHandle(context_handle); context)
    {
        PFILETIME wait_timeout = (context->object_context.wait_timeout.has_value())
                                   ? &context->object_context.wait_timeout.value()
                                   : nullptr;

        ntp::details::SafeThreadpoolCall<SetThreadpoolWait>(
            context->native_handle, context->object_context.wait_handle, wait_timeout);

        return context_handle;
    }

    throw exception::Win32Exception(ERROR_INVALID_HANDLE);
}

/* static */
void NTAPI Manager::InvokeCallback(PTP_CALLBACK_INSTANCE instance, HANDLE context_handle, PTP_WAIT wait, TP_WAIT_RESULT wait_result) noexcept
{
    try
    {
        if (!context_handle)
        {
            throw exception::Win32Exception(ERROR_INVALID_PARAMETER);
        }

        if (auto context = context_t::FromHandle(context_handle); context)
        {
            context->callback->Call(instance, reinterpret_cast<void*>(wait_result));

            //
            // BUGBUG: need to think about exceptions in user-defined callback
            // Probably need to implement something like std::async here
            //

            auto manager = context->meta_context.manager;
            return manager->Remove(context->meta_context.iterator);
        }

        throw exception::Win32Exception(ERROR_INVALID_HANDLE);
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
