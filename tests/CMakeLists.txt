#
# Find googletest library
#
find_package(GTest CONFIG REQUIRED)

#
# Necessary variables
#
set(TEST_ROOT                ${NTP_ROOT}/tests)
set(TEST_SOURCE_ROOT         ${TEST_ROOT})

set(TEST_INCLUDE_DIRECTORIES ${NTP_INCLUDE_DIR} 
                             ${gtest_SOURCE_DIR}/include 
                             ${gtest_SOURCE_DIR})

#
# Sources and headers
#
set(TEST_SOURCE_FILES ${TEST_SOURCE_ROOT}/pool_creation_test.cpp
                      ${TEST_SOURCE_ROOT}/work_test.cpp)

set(TEST_HEADER_FILES )

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
