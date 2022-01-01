cmake_minimum_required(VERSION 3.6)

project(benchmark)

find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)

find_package(wfrest REQUIRED CONFIG HINTS ..)

if(wfrest_FOUND)
    message(STATUS "Build wfrest benchmark")
else()
    message(FATAL_ERROR "can't find wfrest, please build it first")
endif()


add_executable(wfrest_bench wfrest_bench.cc)
target_link_libraries(wfrest_bench wfrest)
