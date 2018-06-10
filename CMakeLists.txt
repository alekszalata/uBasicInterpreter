cmake_minimum_required(VERSION 3.10)
project(uBasicInterpreter)

set(CMAKE_CXX_STANDARD 11)

add_executable(uBasicInterpreter main/analyzer.c main/main.c main/analyzer.h)