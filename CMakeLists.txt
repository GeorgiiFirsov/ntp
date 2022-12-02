cmake_minimum_required(VERSION 3.0.0)
project(ntp VERSION 0.1.3)

#
# Global config
#
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#
# Options
#
option(NTP_ENABLE_TESTING      "Enable testing of ntp library." ON)
option(NTP_ENABLE_DOCS         "Enable building docs for ntp library." ON)
option(NTP_ENABLE_GH_DOCS_ONLY "Building documentation only (used by GitHub Actions)" OFF)

#
# Configuration
#
if (NTP_ENABLE_GH_DOCS_ONLY)

    #
    # Here we build GitHub documentation only
    #

    set(NTP_BUILD_LIBRARY OFF)
    set(NTP_BUILD_TESTS   OFF)
    set(NTP_BUILD_DOCS    ON)
    set(NTP_BUILD_GH_DOCS ON)

    message(NOTICE "[${CMAKE_PROJECT_NAME}] Building GitHub documentation only. Flags NTP_ENABLE_TESTING and NTP_ENABLE_DOCS are ignored")

else (NTP_ENABLE_GH_DOCS_ONLY)

    #
    # Always build library itself
    #

    set(NTP_BUILD_LIBRARY ON)

    #
    # For tests and docs we need to analyze flags
    #

    if (NTP_ENABLE_TESTING)
        set(NTP_BUILD_TESTS ON)
    else (NTP_ENABLE_TESTING)
        set(NTP_BUILD_TESTS OFF)
    endif (NTP_ENABLE_TESTING)

    if (NTP_ENABLE_DOCS)
        set(NTP_BUILD_DOCS    ON)
        set(NTP_BUILD_GH_DOCS OFF)
    else (NTP_ENABLE_DOCS)
        set(NTP_BUILD_DOCS    OFF)
        set(NTP_BUILD_GH_DOCS OFF)
    endif (NTP_ENABLE_DOCS)

endif (NTP_ENABLE_GH_DOCS_ONLY)

#
# Print current configuration
#
message(NOTICE "[${CMAKE_PROJECT_NAME}] Building library: ${NTP_BUILD_LIBRARY}")
message(NOTICE "[${CMAKE_PROJECT_NAME}] Building tests: ${NTP_BUILD_TESTS} (ignored if NTP_BUILD_LIBRARY is OFF)")
message(NOTICE "[${CMAKE_PROJECT_NAME}] Building docs: ${NTP_BUILD_DOCS} (for GitHub: ${NTP_BUILD_GH_DOCS})")

#
# Basic common directories
#
set(NTP_ROOT         ${CMAKE_SOURCE_DIR})
set(NTP_LIBRARY_ROOT ${NTP_ROOT}/ntp)
set(NTP_INCLUDE_DIR  ${NTP_LIBRARY_ROOT}/include)

#
# Preprocessor definitions
#
add_compile_definitions(UNICODE)  # Build with unicode characters (wchar_t)

#
# Library itself
#
if (NTP_BUILD_LIBRARY)
    add_subdirectory(ntp)
endif(NTP_BUILD_LIBRARY)

# 
# Docs
#
if (NTP_BUILD_DOCS)
    add_subdirectory(docs)
endif (NTP_BUILD_DOCS)

#
# Tests of ntp library (can be built only if library is built too)
#
if (NTP_BUILD_LIBRARY AND NTP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif (NTP_BUILD_LIBRARY AND NTP_BUILD_TESTS)
