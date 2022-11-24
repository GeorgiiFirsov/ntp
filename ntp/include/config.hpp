#pragma once

//
// ntp library version
//

#define NTP_VERSION_MAJOR 1
#define NTP_VERSION_MINOR 0
#define NTP_VERSION_PATCH 0


//
// Necessary external defines
//

#define WIN32_LEAN_AND_MEAN


//
// Library configuration
//

#define NTP_ALLOCATION_ALIGNMENT MEMORY_ALLOCATION_ALIGNMENT

#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606
#   define NTP_INLINE inline
#else  // ^^^ has inline variables ^^^ vvv has no inline variables vvv
#   define NTP_INLINE
#endif  // inline variables
