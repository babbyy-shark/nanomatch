cmake_minimum_required(VERSION 3.20)
project(nanomatch CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

add_executable(nanomatch src/main.cpp)
target_include_directories(nanomatch PRIVATE include)
