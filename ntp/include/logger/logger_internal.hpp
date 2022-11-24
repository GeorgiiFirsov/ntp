/**
 * @file logger_internal.hpp
 * @brief Internal logger subsystem implementation
 */

#pragma once

#include <Windows.h>

#include <atomic>

#include "details/utils.hpp"
#include "logger/logger.hpp"


namespace ntp::logger::details {

/**
 * @brief Internal logger class. Implemented as singleton.
 * 
 * By default internal logger function is not defined (set to NULL),
 * hence logger ignores any messages.
 */
class Logger final
{
    explicit Logger() noexcept
        : logger_(nullptr)
    { }

public:
    /**
     * @brief Const instance accessor
     */
    static const Logger& Instance() noexcept { return InstanceMut(); }

    /**
     * @brief Mutable instance accessor
     */
    static Logger& InstanceMut() noexcept
    {
        static Logger logger;
        return logger;
    }

public:
    /**
     * @brief Replaces internal logger with a new one
     * 
     * @param new_logger New used-defined logger function
     * @returns Previously installed logger function
     */
    logger_t Exchange(logger_t new_logger) noexcept
    {
        const auto old_logger = logger_.load(std::memory_order_acquire);
        logger_.store(new_logger, std::memory_order_release);
        return old_logger;
    }

    /**
     * @brief Formats message and forwards formatted message to internal logger function
     * 
     * @tparam Args... Types of arguments, which will be inserted
     * @param severity Message severity
     * @param message Format of the message
     * @param args Optional pack of arguments to embed into formatted message
     */
    template<typename... Args>
    void TraceMessage(Severity severity, const wchar_t* message, Args&&... args) const noexcept
    {
        if (const auto logger = logger_.load(std::memory_order_acquire); logger)
        {
            const DWORD format_flags = FORMAT_MESSAGE_FROM_STRING;
            const auto formatted     = ntp::details::FormatMessage(format_flags, message, 0, std::forward<Args>(args)...);

            logger(severity, formatted.c_str());
        }
    }

    /**
	 * @brief Formats message and forwards formatted message to internal logger function
     * 
     * After formatting it converts formatted message into std::wstring using ntp::details::Convert.
	 *
	 * @tparam Args... Types of arguments, which will be inserted
	 * @param severity Message severity
	 * @param message Format of the message
	 * @param args Optional pack of arguments to embed into formatted message
	 */
    template<typename... Args>
    void TraceMessage(Severity severity, const char* message, Args&&... args) const noexcept
    {
        if (const auto logger = logger_.load(std::memory_order_acquire); logger)
        {
            const DWORD format_flags = FORMAT_MESSAGE_FROM_STRING;
            const auto formatted     = ntp::details::FormatMessage(format_flags, message, 0, std::forward<Args>(args)...);
            const auto converted     = ntp::details::Convert(formatted);

            logger(severity, converted.c_str());
        }
    }

private:
    // Pointer to logger is atomic to prevent races
    std::atomic<logger_t> logger_;
};

}  // namespace ntp::logger::details
