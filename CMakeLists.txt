cmake_minimum_required(VERSION 3.6)

project(wfrest C CXX)

if(CMAKE_BUILD_TYPE)
else()
    set(CMAKE_BUILD_TYPE "Release")
endif()
message(STATUS "Build Type: " ${CMAKE_BUILD_TYPE})

option(WFREST_BUILD_EXAMPLES "Build wfrest examples" OFF)

option(WFREST_BUILD_BENCHMARK "Build wfrest benchmark" OFF)

option(WFREST_BUILD_TEST "Build wfrest unit test" OFF)

set(CXX_FLAGS
        -g
        -std=c++11
        -rdynamic
        -pthread
        -fno-exceptions
        )

if(CMAKE_BUILD_BITS EQUAL 32)
    list(APPEND CXX_FLAGS "-m32")
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    list(APPEND CXX_FLAGS "-Wno-null-dereference")
    list(APPEND CXX_FLAGS "-Wno-sign-conversion")
    list(APPEND CXX_FLAGS "-Wno-unused-local-typedef")
    list(APPEND CXX_FLAGS "-Wthread-safety")
    list(REMOVE_ITEM CXX_FLAGS "-rdynamic")
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/)

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# option(BUILD_BROTLI "Build Brotli" OFF)
# if (BUILD_BROTLI)
#     find_package(Brotli QUIET)
#     if (Brotli_FOUND)
#         message(STATUS "Brotli found")
#         add_definitions(-DUSE_BROTLI)
#     else()
#         MESSAGE("Brotli Not Found")  
#     endif (Brotli_FOUND)
# endif (BUILD_BROTLI)

find_package(Brotli QUIET)
if (Brotli_FOUND)
    message(STATUS "Brotli found")
    add_definitions(-DUSE_BROTLI)
else()
    MESSAGE("Brotli Not Found")  
endif (Brotli_FOUND)

# set(workflow_DIR ${PROJECT_BINARY_DIR}/lib/cmake/workflow)
SET(WORKFLOW_MIN_VERSION "0.9.9")
find_package(workflow 
    ${WORKFLOW_MIN_VERSION} 
    QUIET 
)

if (NOT workflow_FOUND)
    MESSAGE("-- workflow in system not found or version < 0.9.9")
    MESSAGE("-- Building workflow in this project automatically")
    include(ExternalProject)
    ExternalProject_Add(
        workflow_repo
        GIT_REPOSITORY https://github.com/sogou/workflow.git
        CONFIGURE_COMMAND mkdir -p build
        BUILD_IN_SOURCE TRUE
        BUILD_COMMAND cd build && cmake ../ -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR} && make -j
        INSTALL_COMMAND cd build && make install && cd ../ && rm -rf build
        UPDATE_COMMAND ""
        GIT_SHALLOW         TRUE
    )
    add_library(workflow STATIC IMPORTED GLOBAL)                                                                                   
    add_dependencies(workflow workflow_repo)
    set_target_properties(
        workflow PROPERTIES
        IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/libworkflow.a                                                                    
        INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR}/include
    )
    include_directories(${PROJECT_BINARY_DIR}/include)
    link_directories(${PROJECT_BINARY_DIR}/lib)
else()
    MESSAGE("-- workflow exists(version >= 0.9.9), not build")
endif()

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

include_directories(${PROJECT_SOURCE_DIR}/ ${OPENSSL_INCLUDE_DIR})
add_subdirectory(${PROJECT_SOURCE_DIR}/wfrest)

if(WFREST_BUILD_EXAMPLES)
    message(STATUS "Build wfrest examples")
    add_subdirectory(${PROJECT_SOURCE_DIR}/example)
endif()


if(WFREST_BUILD_BENCHMARK)
    message(STATUS "Build wfrest benchmark")
    add_subdirectory(${PROJECT_SOURCE_DIR}/benchmark)
endif()

if(WFREST_BUILD_TEST)
    find_package(GTest)
    if(GTest_FOUND)
        message(STATUS "Build wfrest unit tests")
        enable_testing()
        add_subdirectory(${PROJECT_SOURCE_DIR}/wfrest/unittest)
    else()
        message(STATUS "GTest Not Found. wfrest unit tests will not be built")
    endif()
endif()








