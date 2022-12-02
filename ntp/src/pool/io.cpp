#include "pool/io.hpp"
#include "details/utils.hpp"
#include "logger/logger_internal.hpp"


namespace ntp::io::details {

IoManager::IoManager(PTP_CALLBACK_ENVIRON environment)
    : BasicManagerEx(environment)
{ }

void IoManager::SubmitInternal(native_handle_t native_handle, object_context_t& object_context)
{
    ntp::details::SafeThreadpoolCall<StartThreadpoolIo>(native_handle);
}

/* static */
void NTAPI IoManager::InvokeCallback(PTP_CALLBACK_INSTANCE instance, context_pointer_t context, PVOID overlapped,
    ULONG result, ULONG_PTR bytes_transferred, PTP_IO io) noexcept
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

        IoData io_data { overlapped, result, bytes_transferred };
        context->callback->Call(instance, &io_data);

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
            L"[IoManager::InvokeCallback]: unknown error");
    }
}

/* static */
void IoManager::CloseInternal(native_handle_t native_handle) noexcept
{
    if (!native_handle)
    {
        //
        // Just to be confident if we are working with a valid object
        //

        return;
    }

    ntp::details::SafeThreadpoolCall<WaitForThreadpoolIoCallbacks>(native_handle, TRUE);
    ntp::details::SafeThreadpoolCall<CloseThreadpoolIo>(native_handle);
}

/* static */
void IoManager::AbortInternal(native_handle_t native_handle) noexcept
{
    if (!native_handle)
    {
        //
        // Just to be confident if we are working with a valid object
        //

        return;
    }

    //
    // Here we need to cancel IO object first. This function prevents 
    // memory leaks if async IO failed to start.
    //

    ntp::details::SafeThreadpoolCall<CancelThreadpoolIo>(native_handle);

    //
    // And now we are ready to close native handle
    //

    CloseInternal(native_handle);
}

}  // namespace ntp::io::details
