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
option(NTP_ENABLE_DOCS "Enable building docs for ntp library." ON)

#
# Basic common directories
#
set(NTP_ROOT ${CMAKE_SOURCE_DIR})
set(NTP_LIBRARY_ROOT ${NTP_ROOT}/ntp)
set(NTP_INCLUDE_DIR ${NTP_LIBRARY_ROOT}/include)

#
# Library itself
#
add_subdirectory(ntp)

# 
# Docs
#
if (NTP_ENABLE_DOCS)
    message(NOTICE "[${CMAKE_PROJECT_NAME}] Docs building is enabled. Adding test projects.")
    add_subdirectory(docs)
else(NTP_ENABLE_DOCS)
    message(NOTICE "[${CMAKE_PROJECT_NAME}] Docs building is disabled.")
endif(NTP_ENABLE_DOCS)

#
# Tests of ntp library
#
if (NTP_ENABLE_TESTING)
    message(NOTICE "[${CMAKE_PROJECT_NAME}] Testing is enabled. Adding test projects.")
    add_subdirectory(tests)
else(NTP_ENABLE_TESTING)
    message(NOTICE "[${CMAKE_PROJECT_NAME}] Testing is disabled.")
endif(NTP_ENABLE_TESTING)
