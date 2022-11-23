cmake_minimum_required(VERSION 3.0.0)
project(ntp VERSION 0.1.0)

#
# Global config
#
set(CMAKE_CXX_STANDARD 17)

#
# Options
#
option(NTP_ENABLE_TESTING "Enable testing of ntp library." ON)

#
# Basic common directories
#
set(NTP_ROOT ${CMAKE_SOURCE_DIR}/ntp)
set(NTP_INCLUDE_DIR ${NTP_ROOT}/include)

#
# Library itself
#
add_subdirectory(ntp)

#
# Tests of ntp library
#
if (NTP_ENABLE_TESTING)
    message("[${CMAKE_PROJECT_NAME}] Testing is enabled. Adding test projects.")
    add_subdirectory(tests)
else()
    message("[${CMAKE_PROJECT_NAME}] Testing is disabled.")
endif()
