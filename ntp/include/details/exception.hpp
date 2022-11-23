#pragma once

#include <Windows.h>

#include <stdexcept>
#include <string>


namespace ntp::exception {

class Win32Exception
    : public std::exception
{
public:
    template<typename... Tys>
    explicit Win32Exception(DWORD code, Tys... args)
        : message_(FormatMessage(code, args...))
    { }

    explicit Win32Exception()
        : Win32Exception(GetLastError())
    { }

private:
    std::string FormatMessage(DWORD code, ... /* C-style varargs are necessary :( */);

private:
    std::string message_;
};

}  // namespace ntp::exception
