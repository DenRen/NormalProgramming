cmake_minimum_required(VERSION 3.5)
project(split_ordered_list)

set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_FLAGS "-g3 -fsanitize=address -fsanitize=undefined")
# set(CMAKE_CXX_FLAGS "-g3")

file (GLOB ${PROJECT_NAME}_SOURCES src/*.cpp)

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})
target_link_libraries(${PROJECT_NAME})
