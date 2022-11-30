#
# Find googletest library
#
find_package(GTest CONFIG REQUIRED)

#
# Necessary variables
#
set(TEST_ROOT                ${NTP_ROOT}/tests)
set(TEST_SOURCE_ROOT         ${TEST_ROOT})
set(TEST_CASES_ROOT          ${TEST_SOURCE_ROOT}/cases)

set(TEST_INCLUDE_DIRECTORIES ${NTP_INCLUDE_DIR} ${TEST_ROOT}
                             ${gtest_SOURCE_DIR}/include 
                             ${gtest_SOURCE_DIR})

#
# Sources and headers
#
set(TEST_SOURCE_FILES ${TEST_CASES_ROOT}/pool_creation_test.cpp
                      ${TEST_CASES_ROOT}/work_test.cpp
                      ${TEST_CASES_ROOT}/wait_test.cpp
                      ${TEST_CASES_ROOT}/timer_test.cpp
                      ${TEST_CASES_ROOT}/io_test.cpp
                      ${TEST_CASES_ROOT}/logger_test.cpp)

set(TEST_HEADER_FILES ${TEST_SOURCE_ROOT}/test_config.hpp
                      ${TEST_SOURCE_ROOT}/utils.hpp)

set(TEST_SOURCES      ${TEST_SOURCE_FILES} 
                      ${TEST_HEADER_FILES})

#
# Test executable
#
add_executable(ntp_test ${TEST_SOURCES})

#
# Links and include directories
#
target_link_libraries(ntp_test PRIVATE ntp GTest::gtest GTest::gtest_main)
target_include_directories(ntp_test PRIVATE ${TEST_INCLUDE_DIRECTORIES})

#
# And now test itself
#
add_test(ntp_test ntp_test)
