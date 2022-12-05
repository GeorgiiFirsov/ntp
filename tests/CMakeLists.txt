#
# Find googletest library
#
find_package(GTest CONFIG REQUIRED)

#
# Necessary variables
#
set(NTP_TEST_ROOT                ${NTP_ROOT}/tests)
set(NTP_TEST_SOURCE_ROOT         ${NTP_TEST_ROOT})
set(NTP_TEST_CASES_ROOT          ${NTP_TEST_SOURCE_ROOT}/cases)

set(NTP_TEST_INCLUDE_DIRECTORIES ${NTP_GENERIC_INCLUDE_DIRECTORIES}
                                 ${NTP_TEST_ROOT}
                                 ${gtest_SOURCE_DIR}/include 
                                 ${gtest_SOURCE_DIR})

#
# Sources and headers
#
set(NTP_TEST_SOURCE_FILES ${NTP_TEST_CASES_ROOT}/pool_creation_test.cpp
                          ${NTP_TEST_CASES_ROOT}/work_test.cpp
                          ${NTP_TEST_CASES_ROOT}/wait_test.cpp
                          ${NTP_TEST_CASES_ROOT}/timer_test.cpp
                          ${NTP_TEST_CASES_ROOT}/io_test.cpp
                          ${NTP_TEST_CASES_ROOT}/logger_test.cpp)

set(NTP_TEST_HEADER_FILES ${NTP_TEST_SOURCE_ROOT}/test_config.hpp
                          ${NTP_TEST_SOURCE_ROOT}/utils.hpp)

set(NTP_TEST_SOURCES      ${NTP_TEST_SOURCE_FILES} 
                          ${NTP_TEST_HEADER_FILES})

#
# Test executable
#
add_executable(ntp_test ${NTP_TEST_SOURCES})

#
# Links and include directories
#
target_link_libraries(ntp_test PRIVATE ntp GTest::gtest GTest::gtest_main)
target_include_directories(ntp_test PRIVATE ${NTP_TEST_INCLUDE_DIRECTORIES})

#
# And now test itself
#
add_test(ntp_test ntp_test)
