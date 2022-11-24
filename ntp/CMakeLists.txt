#
# Directories
#
set(LIB_INCLUDE_ROOT        ${NTP_LIBRARY_ROOT}/include)
set(LIB_DETAILS_INCLUDE     ${LIB_INCLUDE_ROOT}/details)
set(LIB_POOL_INCLUDE        ${LIB_INCLUDE_ROOT}/pool)

set(LIB_SOURCE_ROOT         ${NTP_LIBRARY_ROOT}/src)
set(LIB_DETAILS_SOURCE      ${LIB_SOURCE_ROOT}/details)
set(LIB_POOL_SOURCE         ${LIB_SOURCE_ROOT}/pool)

set(LIB_INCLUDE_DIRECTORIES ${LIB_INCLUDE_ROOT}
                            ${LIB_DETAILS_INCLUDE}
                            ${LIB_POOL_INCLUDE})

#
# Sources and headers
#
set(LIB_SOURCE_FILES ${LIB_POOL_SOURCE}/threadpool.cpp
                     ${LIB_POOL_SOURCE}/work.cpp
                     ${LIB_DETAILS_SOURCE}/exception.cpp
                     ${LIB_DETAILS_SOURCE}/utils.cpp)

set(LIB_HEADER_FILES ${LIB_INCLUDE_ROOT}/ntp.hpp
                     ${LIB_INCLUDE_ROOT}/config.hpp
                     ${LIB_POOL_INCLUDE}/threadpool.hpp
                     ${LIB_POOL_INCLUDE}/basic_callback.hpp
                     ${LIB_POOL_INCLUDE}/work.hpp
                     ${LIB_DETAILS_INCLUDE}/allocator.hpp
                     ${LIB_DETAILS_INCLUDE}/exception.hpp
                     ${LIB_DETAILS_INCLUDE}/utils.hpp)

set(LIB_SOURCES      ${LIB_SOURCE_FILES} 
                     ${LIB_HEADER_FILES})

#
# Library itself
#
add_library(ntp ${LIB_SOURCES})

#
# Includes
#
target_include_directories(ntp PRIVATE ${LIB_INCLUDE_DIRECTORIES})
