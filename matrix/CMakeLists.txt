cmake_minimum_required(VERSION 3.20)
project(matrix)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Ofast -march=x86-64-v3)

add_executable(${PROJECT_NAME} "src/main.cpp")
target_compile_options(${PROJECT_NAME} PRIVATE -Ofast)
target_link_options(${PROJECT_NAME} PRIVATE -Ofast)

## Enable ASAN
# target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
# target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)

# Graphics --------------------------------------------------------------------
project(graphics)

add_executable(${PROJECT_NAME} "src/graphics.cpp")
target_compile_options(${PROJECT_NAME} PRIVATE -Ofast)
target_link_options(${PROJECT_NAME} PRIVATE -Ofast)

# Tests -----------------------------------------------------------------------
set(PROJECT_TEST_NAME "run_test_${PROJECT_NAME}")
project(${PROJECT_TEST_NAME})

ADD_SUBDIRECTORY (googletest)
include_directories (googletest/googletest/include ${gtest_SOURCE_DIR})
enable_testing ()

add_executable(${PROJECT_TEST_NAME})
file(GLOB test_srcs
     "src/tests/*.h"
     "src/tests/*.cpp"
)

target_sources(${PROJECT_TEST_NAME} PUBLIC ${test_srcs})

#Link test executable against gtest and gtest_main
target_link_libraries (${PROJECT_TEST_NAME} gtest gtest_main)
