/**
 * @file time.hpp
 * @brief Utils to work with std::chrono
 */

#pragma once

#include <Windows.h>

#include <ratio>
#include <chrono>

#include "config.hpp"


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
NTP_INLINE constexpr bool is_duration_v = is_duration<Ty>::value;

}  // namespace details


/**
 * @brief Native 100-ns duration interval.
 */
using native_duration_t = std::chrono::duration<unsigned long long, std::ratio<1, 10'000'000>>;


/**
 * @brief Maximum supported native duration count.
 */
NTP_INLINE constexpr auto max_native_duration = (native_duration_t::max)();


/**
 * @brief Converts std::chrono::duration to FILETIME.
 * 
 * @param duration Duration to convert
 * @returns Converted into FILETIME duration
 */
template<typename Rep, typename Period>
FILETIME AsFiletime(const std::chrono::duration<Rep, Period>& duration) noexcept
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

}  // namespace ntp::time
