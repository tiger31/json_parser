cmake_minimum_required(VERSION 3.10)
project(json_parser C)

set(CMAKE_C_STANDARD 11)

add_library(json_parser SHARED json.c json.h)
add_executable(json_test test/main.c test/json.h test/minunit.h)