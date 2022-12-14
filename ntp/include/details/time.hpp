/**
 * @file time.hpp
 * @brief Utils to work with std::chrono
 */

#pragma once

#include <ratio>
#include <chrono>

#include "details/windows.hpp"


namespace ntp::time {
namespace details {

/**
 * @brief Trait that detects std::chono::duration specializations.
 * 
 * Generic version (for all types, except std::chono::duration).
 */
template<typename>
struct is_duration : std::false_type
{ };

/**
 * @brief Trait that detects std::chono::duration specializations.
 *
 * std::chono::duration specialization.
 */
template<typename Rep, typename Period>
struct is_duration<std::chrono::duration<Rep, Period>> : std::true_type
{ };

/**
 * @brief Trait that detects std::chono::duration specializations.
 * 
 * Helper inline (since C++17) variable.
 */
template<typename Ty>
inline constexpr bool is_duration_v = is_duration<Ty>::value;


/**
 * @brief Trait that detects std::chono::time_point specializations.
 *
 * Generic version (for all types, except std::chono::time_point).
 */
template<typename>
struct is_time_point : std::false_type
{ };

/**
 * @brief Trait that detects std::chono::time_point specializations.
 *
 * std::chono::time_point specialization.
 */
template<typename Clock, typename Duration>
struct is_time_point<std::chrono::time_point<Clock, Duration>> : std::true_type
{ };

/**
 * @brief Trait that detects std::chono::time_point specializations.
 *
 * Helper inline (since C++17) variable.
 */
template<typename Ty>
inline constexpr bool is_time_point_v = is_time_point<Ty>::value;

}  // namespace details


/**
 * @brief Native 100-ns duration interval.
 */
using native_duration_t = std::chrono::duration<long long, std::ratio<1, 10'000'000>>;


/**
 * @brief Maximum supported native duration count.
 */
inline constexpr auto max_native_duration = (native_duration_t::max)();


/**
 * @brief Clock to measure deadlines
 */
using deadline_clock_t = std::chrono::steady_clock;


/**
 * @brief Time point, that specifies a deadline
 */
template<typename Duration>
using deadline_t = std::chrono::time_point<deadline_clock_t, Duration>;


/**
 * @brief Converts std::chrono::duration to FILETIME.
 * 
 * @param duration Duration to convert
 * @returns Converted into FILETIME duration
 */
template<typename Rep, typename Period>
FILETIME AsFileTime(const std::chrono::duration<Rep, Period>& duration) noexcept
{
    const auto native       = std::chrono::duration_cast<native_duration_t>(duration);
    const auto native_count = native.count();

    //
    // Now we need to construct low and high parts of native_count
    //

    const auto low_part  = static_cast<DWORD>(native_count);
    const auto high_part = static_cast<DWORD>(native_count >> 32);

    return FILETIME { low_part, high_part };
}


/**
 * @brief Negates a duration value stored in FILETIME structure.
 */
inline FILETIME Negate(FILETIME time) noexcept
{
    time.dwLowDateTime = static_cast<DWORD>(-static_cast<LONG>(time.dwLowDateTime));
    return time;
}

}  // namespace ntp::time
