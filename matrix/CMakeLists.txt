cmake_minimum_required(VERSION 3.20)
project(matrix)

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} "src/main.cpp")

# Tests -----------------------------------------------------------------------
set(PROJECT_TEST_NAME "run_test_${PROJECT_NAME}")
project(${PROJECT_TEST_NAME})

ADD_SUBDIRECTORY (googletest)
include_directories (googletest/googletest/include ${gtest_SOURCE_DIR})
enable_testing ()

add_executable(${PROJECT_TEST_NAME})
target_sources(${PROJECT_TEST_NAME} PUBLIC src/tests/test_mc.cpp)

#Link test executable against gtest and gtest_main
target_link_libraries (${PROJECT_TEST_NAME} gtest gtest_main)
