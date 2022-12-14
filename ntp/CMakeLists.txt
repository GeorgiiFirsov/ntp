#
# Directories
#
set(NTP_LIB_INCLUDE_ROOT        ${NTP_LIBRARY_ROOT}/include)
set(NTP_LIB_DETAILS_INCLUDE     ${NTP_LIB_INCLUDE_ROOT}/details)
set(NTP_LIB_POOL_INCLUDE        ${NTP_LIB_INCLUDE_ROOT}/pool)
set(NTP_LIB_LOGGER_INCLUDE      ${NTP_LIB_INCLUDE_ROOT}/logger)
set(NTP_LIB_NATIVE_INCLUDE      ${NTP_LIB_INCLUDE_ROOT}/native)

set(NTP_LIB_SOURCE_ROOT         ${NTP_LIBRARY_ROOT}/src)
set(NTP_LIB_DETAILS_SOURCE      ${NTP_LIB_SOURCE_ROOT}/details)
set(NTP_LIB_POOL_SOURCE         ${NTP_LIB_SOURCE_ROOT}/pool)
set(NTP_LIB_LOGGER_SOURCE       ${NTP_LIB_SOURCE_ROOT}/logger)

set(NTP_LIB_INCLUDE_DIRECTORIES ${NTP_GENERIC_INCLUDE_DIRECTORIES})

#
# Sources and headers
#
set(NTP_LIB_SOURCE_FILES ${NTP_LIB_POOL_SOURCE}/threadpool.cpp
                         ${NTP_LIB_POOL_SOURCE}/work.cpp
                         ${NTP_LIB_POOL_SOURCE}/wait.cpp
                         ${NTP_LIB_POOL_SOURCE}/timer.cpp
                         ${NTP_LIB_POOL_SOURCE}/io.cpp
                         ${NTP_LIB_LOGGER_SOURCE}/logger.cpp
                         ${NTP_LIB_DETAILS_SOURCE}/utils.cpp)

set(NTP_LIB_HEADER_FILES ${NTP_LIB_INCLUDE_ROOT}/ntp.hpp
                         ${NTP_LIB_INCLUDE_ROOT}/ntp_config.hpp
                         ${NTP_LIB_POOL_INCLUDE}/threadpool.hpp
                         ${NTP_LIB_POOL_INCLUDE}/basic_callback.hpp
                         ${NTP_LIB_POOL_INCLUDE}/work.hpp
                         ${NTP_LIB_POOL_INCLUDE}/wait.hpp
                         ${NTP_LIB_POOL_INCLUDE}/timer.hpp
                         ${NTP_LIB_POOL_INCLUDE}/io.hpp
                         ${NTP_LIB_LOGGER_INCLUDE}/logger.hpp
                         ${NTP_LIB_LOGGER_INCLUDE}/logger_internal.hpp
                         ${NTP_LIB_NATIVE_INCLUDE}/ntrtl.h
                         ${NTP_LIB_DETAILS_INCLUDE}/allocator.hpp
                         ${NTP_LIB_DETAILS_INCLUDE}/exception.hpp
                         ${NTP_LIB_DETAILS_INCLUDE}/time.hpp
                         ${NTP_LIB_DETAILS_INCLUDE}/utils.hpp
                         ${NTP_LIB_DETAILS_INCLUDE}/windows.hpp)

set(NTP_LIB_SOURCES      ${NTP_LIB_SOURCE_FILES} 
                         ${NTP_LIB_HEADER_FILES})

#
# Library itself
#
add_library(ntp ${NTP_LIB_SOURCES})

#
# Link with ntdll.lib
#
target_link_libraries(ntp PRIVATE ntdll)

#
# Includes
#
target_include_directories(ntp PRIVATE ${NTP_LIB_INCLUDE_DIRECTORIES})
