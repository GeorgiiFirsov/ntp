#
# Directories
#
set(LIB_INCLUDE_ROOT        ${NTP_LIBRARY_ROOT}/include)
set(LIB_DETAILS_INCLUDE     ${LIB_INCLUDE_ROOT}/details)

set(LIB_SOURCE_ROOT         ${NTP_LIBRARY_ROOT}/src)
set(LIB_DETAILS_SOURCE      ${LIB_SOURCE_ROOT}/details)

set(LIB_INCLUDE_DIRECTORIES ${LIB_INCLUDE_ROOT}
                            ${LIB_INCLUDE_ROOT}/details)

#
# Sources and headers
#
set(LIB_SOURCE_FILES ${LIB_DETAILS_SOURCE}/threadpool.cpp
                     ${LIB_DETAILS_SOURCE}/exception.cpp)

set(LIB_HEADER_FILES ${LIB_INCLUDE_ROOT}/ntp.hpp
                     ${LIB_DETAILS_INCLUDE}/config.hpp
                     ${LIB_DETAILS_INCLUDE}/threadpool.hpp
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
