#pragma once

//
// Necessary external defines
//

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX


//
// Library configuration
//

#define NTP_ALLOCATION_ALIGNMENT MEMORY_ALLOCATION_ALIGNMENT

#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606
#   define NTP_INLINE inline
#else  // ^^^ has inline variables ^^^ vvv has no inline variables vvv
#   define NTP_INLINE
#endif  // inline variables
