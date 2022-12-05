/**
 * @file utils.cpp
 * @brief Implementation of helper functions and classes
 */

#include <vector>

#include "ntp_config.hpp"
#include "native/ntrtl.h"
#include "details/utils.hpp"
#include "details/exception.hpp"


namespace ntp::details {

/**
 * @brief Basic format function. Used by FormatMessage
 * 
 * @tparam FormatTraits Traits for internal formatting function dispatch
 * @param flags formatting flags (refer to Win32 API for possible values)
 * @param source optional format string (if FORMAT_MESSAGE_FROM_SYSTEM is specified this parameter MUST be NULL)
 * @param message_id optional message identifier (ignored if FORMAT_MESSAGE_FROM_STRING is specified)
 * @param args C-style variadic arguments pack
 * @returns formatted message of empty string in case of error
 */
template<typename FormatTraits>
typename FormatTraits::result_t BasicFormatMessage(DWORD flags, typename FormatTraits::source_t source, DWORD message_id, va_list* args) noexcept
{
    using result_t = typename FormatTraits::result_t;
    using buffer_t = typename FormatTraits::buffer_t;

    flags |= FORMAT_MESSAGE_ALLOCATE_BUFFER;

    buffer_t buffer           = nullptr;
    const DWORD chars_written = FormatTraits::Format(
        flags, source, message_id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<buffer_t>(&buffer), 0, args);

    //
    // Write result to string and free intermediate buffer
    //

    result_t result;

    if (chars_written)
    {
        result.assign(buffer, chars_written);
        LocalFree(buffer);
    }

    return result;
}


std::string FormatMessage(DWORD flags, LPCSTR source, DWORD message_id, ...) noexcept
{
    struct Traits
    {
        using result_t = std::string;
        using source_t = LPCSTR;
        using buffer_t = LPSTR;

        static DWORD Format(DWORD flags, LPCVOID source, DWORD message_id, DWORD language_id, LPSTR buffer, DWORD size, va_list* args) noexcept
        {
            return ::FormatMessageA(flags, source, message_id, language_id, buffer, size, args);
        }
    };

    va_list args;
    va_start(args, message_id);

    const auto result = BasicFormatMessage<Traits>(flags, source, message_id, &args);

    va_end(args);

    return result;
}


std::wstring FormatMessage(DWORD flags, LPCWSTR source, DWORD message_id, ...) noexcept
{
    struct Traits
    {
        using result_t = std::wstring;
        using source_t = LPCWSTR;
        using buffer_t = LPWSTR;

        static DWORD Format(DWORD flags, LPCVOID source, DWORD message_id, DWORD language_id, LPWSTR buffer, DWORD size, va_list* args) noexcept
        {
            return ::FormatMessageW(flags, source, message_id, language_id, buffer, size, args);
        }
    };

    va_list args;
    va_start(args, message_id);

    const auto result = BasicFormatMessage<Traits>(flags, source, message_id, &args);

    va_end(args);

    return result;
}


std::wstring Convert(const std::string& source) noexcept
{
    static constexpr UINT kCodePage = 1251;  // Windows-1251

    if (source.empty())
    {
        return {};
    }

    auto buffer_size = ::MultiByteToWideChar(kCodePage, 0, source.c_str(), -1, NULL, 0);

    if (0 != buffer_size)
    {
        std::vector<wchar_t> buffer(buffer_size);

        buffer_size = MultiByteToWideChar(kCodePage, 0, source.c_str(), -1, buffer.data(), buffer_size);
        if (0 != buffer_size)
        {
            return std::wstring(buffer.data(), static_cast<size_t>(buffer_size) - 1);
        }
    }

    return {};
}


NativeSlist::NativeSlist()
    : header_(allocator_t::Allocate<NTP_ALLOCATION_ALIGNMENT>())
{
    if (!header_)
    {
        throw exception::Win32Exception(ERROR_NOT_ENOUGH_MEMORY);
    }

    InitializeSListHead(header_);
}

NativeSlist::~NativeSlist()
{
    if (header_)
    {
        allocator_t::Free(header_);
    }
}

void NativeSlist::Push(PSLIST_ENTRY entry) noexcept
{
    InterlockedPushEntrySList(header_, entry);
}


RtlResource::RtlResource()
    : resource_()
{
    RtlInitializeResource(&resource_);
}

RtlResource::~RtlResource()
{
    RtlDeleteResource(&resource_);
}

void RtlResource::lock()
{
    RtlAcquireResourceExclusive(&resource_, TRUE);
}

bool RtlResource::try_lock()
{
    return RtlAcquireResourceExclusive(&resource_, FALSE);
}

void RtlResource::unlock() noexcept
{
    RtlReleaseResource(&resource_);
}

void RtlResource::lock_shared()
{
    RtlAcquireResourceShared(&resource_, TRUE);
}

bool RtlResource::try_lock_shared()
{
    return RtlAcquireResourceShared(&resource_, FALSE);
}

void RtlResource::unlock_shared() noexcept
{
    RtlReleaseResource(&resource_);
}


Event::Event(LPSECURITY_ATTRIBUTES security_attributes, BOOL manual_reset, BOOL initially_signaled, LPCWSTR name /* = nullptr */)
    : event_(CreateEvent(security_attributes, manual_reset, initially_signaled, name))
{
    if (!event_)
    {
        throw exception::Win32Exception();
    }
}

Event::Event(BOOL manual_reset, BOOL initially_signaled, LPCWSTR name /* = nullptr */)
    : Event(nullptr, manual_reset, initially_signaled, name)
{ }

Event::~Event()
{
    if (event_)
    {
        CloseHandle(event_);
    }
}

void Event::Set()
{
    if (!SetEvent(event_))
    {
        throw exception::Win32Exception();
    }
}

void Event::Reset()
{
    if (!ResetEvent(event_))
    {
        throw exception::Win32Exception();
    }
}

}  // namespace ntp::details
