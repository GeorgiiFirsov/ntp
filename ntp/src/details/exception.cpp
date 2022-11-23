#include "exception.hpp"


namespace ntp::exception {

std::string Win32Exception::FormatMessage(DWORD code, ...) noexcept
{
    va_list args;
    va_start(args, code);

    LPSTR buffer      = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

    const DWORD chars_written = ::FormatMessageA(flags, nullptr, code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&buffer), 0, &args);

    //
    // In case of error just write error code as a number
    // Otherwise copy formatted message
    //

    if (0 != chars_written)
    {
        message_.assign(buffer, chars_written);
        LocalFree(buffer);
    }
    else
    {
        message_ = std::to_string(code);
    }

    va_end(args);

    return message_;
}

}  // namespace ntp::exception
