/**
 * @file exception.hpp
 * @brief Win32 error wrapper implementations
 *
 * This file contains an implementation of a wrapper for Win32 
 * error code represented as an exception.
 */

#pragma once

#include <Windows.h>

#include <stdexcept>
#include <string>


namespace ntp::exception {

/**
 * @brief Wrapper for Win32 error code, that has C++ interface
 */
class Win32Exception
    : public std::exception
{
public:
    /**
     * @brief Constructor, that formats message for a specific error code
     * 
     * @tparam Tys Variadic pack of argument types (always deduced automatically)
     * @param code Win32 error code
     * @param args Optional variable number of arguments, that can be used for message formatting
     */
    template<typename... Tys>
    explicit Win32Exception(DWORD code, Tys... args) noexcept
        : message_(FormatMessage(code, args...))
    { }

    /**
     * @brief Default constructor, that uses GetLastError as an error code
     */
    explicit Win32Exception() noexcept
        : Win32Exception(GetLastError())
    { }

    /**
     * @brief Error description (inherited from std::exception)
     * 
     * @returns Error description as a C-string
     */
    const char* what() const noexcept override { return message_.c_str(); }

private:
    std::string FormatMessage(DWORD code, ... /* C-style varargs are necessary :( */) noexcept;

private:
    // Error description
    std::string message_;
};

}  // namespace ntp::exception
