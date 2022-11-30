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
void IoManager::Close(native_handle_t native_handle) noexcept
{
    ntp::details::SafeThreadpoolCall<CancelThreadpoolIo>(native_handle);
    ntp::details::SafeThreadpoolCall<WaitForThreadpoolIoCallbacks>(native_handle, TRUE);
    ntp::details::SafeThreadpoolCall<CloseThreadpoolIo>(native_handle);
}

}  // namespace ntp::io::details
