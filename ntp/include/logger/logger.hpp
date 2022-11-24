/**
 * @file logger.hpp
 * @brief Interface for custom logger function
 */

#pragma once


namespace ntp::logger {

/**
 * @brief Severity of a message
 */
enum class Severity : unsigned char
{
    kNormal   = 0, /**< Normal message */
    kExtended = 1, /**< Not very important message */
    kError    = 2, /**< Error message */
    kCritical = 3  /**< Critical error message */
};


/**
 * @brief Logger function type
 * 
 * @param severity Message severity (refer to ntp::logger::Severity for list of possible values)
 * @param message Formatted message
 */
using logger_t = void (*)(Severity severity, const wchar_t* message);


/**
 * @brief Replaces a logger function
 * 
 * @param new_logger New user-defined logger function
 * @returns previously installed logger function
 */
logger_t SetLogger(logger_t new_logger);

}  // namespace ntp::logger
