#
# Directories
#
set(LIB_INCLUDE_ROOT        ${NTP_LIBRARY_ROOT}/include)
set(LIB_DETAILS_INCLUDE     ${LIB_INCLUDE_ROOT}/details)
set(LIB_POOL_INCLUDE        ${LIB_INCLUDE_ROOT}/pool)
set(LIB_LOGGER_INCLUDE      ${LIB_INCLUDE_ROOT}/logger)
set(LIB_NATIVE_INCLUDE      ${LIB_INCLUDE_ROOT}/native)

set(LIB_SOURCE_ROOT         ${NTP_LIBRARY_ROOT}/src)
set(LIB_DETAILS_SOURCE      ${LIB_SOURCE_ROOT}/details)
set(LIB_POOL_SOURCE         ${LIB_SOURCE_ROOT}/pool)
set(LIB_LOGGER_SOURCE       ${LIB_SOURCE_ROOT}/logger)

set(LIB_INCLUDE_DIRECTORIES ${LIB_INCLUDE_ROOT}
                            ${LIB_DETAILS_INCLUDE}
                            ${LIB_POOL_INCLUDE}
                            ${LIB_LOGGER_INCLUDE})

#
# Sources and headers
#
set(LIB_SOURCE_FILES ${LIB_POOL_SOURCE}/threadpool.cpp
                     ${LIB_POOL_SOURCE}/work.cpp
                     ${LIB_POOL_SOURCE}/wait.cpp
                     ${LIB_POOL_SOURCE}/timer.cpp
                     ${LIB_POOL_SOURCE}/io.cpp
                     ${LIB_LOGGER_SOURCE}/logger.cpp
                     ${LIB_DETAILS_SOURCE}/utils.cpp)

set(LIB_HEADER_FILES ${LIB_INCLUDE_ROOT}/ntp.hpp
                     ${LIB_INCLUDE_ROOT}/config.hpp
                     ${LIB_POOL_INCLUDE}/threadpool.hpp
                     ${LIB_POOL_INCLUDE}/basic_callback.hpp
                     ${LIB_POOL_INCLUDE}/work.hpp
                     ${LIB_POOL_INCLUDE}/wait.hpp
                     ${LIB_POOL_INCLUDE}/timer.hpp
                     ${LIB_POOL_INCLUDE}/io.hpp
                     ${LIB_LOGGER_INCLUDE}/logger.hpp
                     ${LIB_LOGGER_INCLUDE}/logger_internal.hpp
                     ${LIB_NATIVE_INCLUDE}/ntrtl.h
                     ${LIB_DETAILS_INCLUDE}/allocator.hpp
                     ${LIB_DETAILS_INCLUDE}/exception.hpp
                     ${LIB_DETAILS_INCLUDE}/time.hpp
                     ${LIB_DETAILS_INCLUDE}/utils.hpp)

set(LIB_SOURCES      ${LIB_SOURCE_FILES} 
                     ${LIB_HEADER_FILES})

#
# Library itself
#
add_library(ntp ${LIB_SOURCES})

#
# Link with ntdll.lib
#
target_link_libraries(ntp PRIVATE ntdll)

#
# Includes
#
target_include_directories(ntp PRIVATE ${LIB_INCLUDE_DIRECTORIES})
