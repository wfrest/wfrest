cmake_minimum_required(VERSION 2.8)
project(wfrest C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# only build examples if this is the main project
if(CMAKE_PROJECT_NAME STREQUAL "wfrest")
    option(WFREST_BUILD_EXAMPLES "Build wfrest examples" ON)
endif()

set(CXX_FLAGS
        -g
        -std=c++11
        -rdynamic
        -pthread
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

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

find_package(OpenSSL REQUIRED)
find_package(Brotli QUIET)
if (Brotli_FOUND)
    message(STATUS "Brotli found")
    add_definitions(-DUSE_BROTLI)
    target_link_libraries(${PROJECT_NAME} PRIVATE Brotli_lib)
endif (Brotli_FOUND)

find_package(workflow QUIET)
if (NOT workflow_FOUND)  
    MESSAGE("-- Third Party workflow not found, build target") 
    include(ExternalProject)
    ExternalProject_Add(
        workflow_repo
        GIT_REPOSITORY https://github.com/sogou/workflow.git
        CONFIGURE_COMMAND mkdir -p build
        BUILD_IN_SOURCE TRUE
        BUILD_COMMAND cd build && cmake ../ -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR} && make -j
        INSTALL_COMMAND cd build && make install && cd ../ && rm -rf build
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
    MESSAGE("-- Third Party workflow exist, not build")  
endif()

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

include_directories(${PROJECT_SOURCE_DIR}/src ${OPENSSL_INCLUDE_DIR})
add_subdirectory(${PROJECT_SOURCE_DIR}/src)
#if(WFREST_BUILD_EXAMPLES)
add_subdirectory(${PROJECT_SOURCE_DIR}/example)
add_subdirectory(${PROJECT_SOURCE_DIR}/tests/test)
#endif()


# unit test part
find_package(GTest)
if(GTest_FOUND)
    message(STATUS "Building tests")
    enable_testing()
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests/unit_test)
endif(GTest_FOUND)
