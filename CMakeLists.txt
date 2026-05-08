cmake_minimum_required(VERSION 3.20)
project(nanomatch CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

# Google Benchmark
include(FetchContent)
FetchContent_Declare(googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.8.3)
set(BENCHMARK_ENABLE_TESTING OFF)
FetchContent_MakeAvailable(googlebenchmark)

# Main executable
add_executable(nanomatch src/main.cpp)
target_include_directories(nanomatch PRIVATE include)

# Benchmark executable
add_executable(nanomatch_bench bench/bench_main.cpp)
target_include_directories(nanomatch_bench PRIVATE include)
target_link_libraries(nanomatch_bench benchmark::benchmark)
