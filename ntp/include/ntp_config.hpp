#pragma once

//
// Necessary external defines
//

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX


//
// Minimum supported Windows version
//

#ifndef WINVER
#   define WINVER 0x0601  // Windows 7
#endif

#ifndef _WIN32_WINNT
#   define _WIN32_WINNT 0x0601  // Windows 7
#endif


//
// Library configuration
//

#define NTP_ALLOCATION_ALIGNMENT MEMORY_ALLOCATION_ALIGNMENT


//
// Check minimum supported Windows version (just to be sure, if no one set it before)
//

#if _WIN32_WINNT < 0x0601
#   error ntp library requires _WIN32_WINNT to be equal to at least 0x0601 (Windows 7)
#endif  // Prior to Windows 7
