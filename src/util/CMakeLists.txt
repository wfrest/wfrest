cmake_minimum_required(VERSION 3.6)
project(util)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SRC
    FileUtil.cc
    MysqlUtil.cc
    PathUtil.cc 
    StrUtil.cc
    UriUtil.cc
    CodeUtil.cc
)

add_library(${PROJECT_NAME} OBJECT ${SRC})

