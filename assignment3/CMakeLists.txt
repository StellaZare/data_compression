# Specify minimum required version of CMake
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

# Specify project, compiler and languages
project(assignment3 LANGUAGES CXX)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -std=c++20")

add_executable(uvg_compress ${CMAKE_CURRENT_SOURCE_DIR}/uvg_compress.cpp)
add_executable(uvg_decompress ${CMAKE_CURRENT_SOURCE_DIR}/uvg_decompress.cpp)
add_executable(dct ${CMAKE_CURRENT_SOURCE_DIR}/dct.cpp)