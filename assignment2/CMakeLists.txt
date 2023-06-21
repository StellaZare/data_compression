cmake_minimum_required(VERSION 3.13)
project(uvgz)
set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "-O3 -Wall -Wextra")

add_executable(test_prefix_codes test_prefix_codes.cpp)
add_executable(uvgz uvgz.cpp)
