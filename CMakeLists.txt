cmake_minimum_required(VERSION 3.6)

project(wfrest
        VERSION 0.1.1
        LANGUAGES C CXX)

if(CMAKE_BUILD_TYPE)
else()
    set(CMAKE_BUILD_TYPE "Release")
endif()
message(STATUS "Build Type: " ${CMAKE_BUILD_TYPE})

option(WFREST_BUILD_EXAMPLES "Build wfrest examples" OFF)
option(WFREST_BUILD_TEST "Build wfrest test" OFF)
option(DOWNLOAD_BUILD_WORKFLOW "Download and build workflow" OFF)

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

# find_package(Brotli QUIET)
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

SET(WORKFLOW_MIN_VERSION "0.9.9")
find_package(workflow 
    ${WORKFLOW_MIN_VERSION} 
    QUIET 
)

if (NOT workflow_FOUND)
    MESSAGE("-- workflow in system not found or version < 0.9.9")
    MESSAGE("-- Building workflow in this project automatically")
    set(DOWNLOAD_BUILD_WORKFLOW ON)

    include(ExternalProject)
    ExternalProject_Add(
        workflow_repo
        GIT_REPOSITORY https://github.com/sogou/workflow.git
        GIT_TAG v0.9.10
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND mkdir -p build
        BUILD_COMMAND cd build && cmake ../ -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR} && make -j
        INSTALL_COMMAND cd build && make install && cd ../ && rm -rf build
        UPDATE_COMMAND ""
        GIT_SHALLOW TRUE
    )

    add_library(workflow_external STATIC IMPORTED GLOBAL)                                                                                   
    add_dependencies(workflow_external workflow_repo)
    set_target_properties(
        workflow_external PROPERTIES
        IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/libworkflow.a                                                                    
        INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR}/include
    )
    include_directories(${CMAKE_BINARY_DIR}/include)
    link_directories(${CMAKE_BINARY_DIR}/lib)
else()
    MESSAGE("-- workflow exists(version >= 0.9.9), not build")
endif()

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

include_directories(${PROJECT_SOURCE_DIR}/ ${OPENSSL_INCLUDE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/test)
add_subdirectory(${PROJECT_SOURCE_DIR}/wfrest)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
set(CMAKE_VERSION_FILE       ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake)
set(CMAKE_CONFIG_FILE        ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}-config.cmake)

configure_package_config_file(
     ${PROJECT_NAME}-config.cmake.in
     ${CMAKE_CONFIG_FILE}
     INSTALL_DESTINATION  ${CMAKE_CONFIG_INSTALL_DIR}
)

write_basic_package_version_file(
     ${CMAKE_VERSION_FILE}
     VERSION ${PROJECT_VERSION}
     COMPATIBILITY AnyNewerVersion 
)

install(
    FILES       ${CMAKE_VERSION_FILE}   ${CMAKE_CONFIG_FILE}
    DESTINATION ${CMAKE_CONFIG_INSTALL_DIR}
    COMPONENT devel
)

if(WFREST_BUILD_EXAMPLES)
    message(STATUS "Build wfrest examples")
    add_subdirectory(${PROJECT_SOURCE_DIR}/example)
endif()

if(WFREST_BUILD_TEST)
    find_package(GTest REQUIRED)
    if(GTest_FOUND)
        message(STATUS "Build wfrest tests")
        enable_testing()
        add_subdirectory(${PROJECT_SOURCE_DIR}/test)
    else()
        message(FATAL_ERROR  "can't find GTest, please install it first")
    endif()
endif()

