cmake_minimum_required(VERSION 3.6)
project(core)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SRC
    BluePrint.cc
    HttpContent.cc
    HttpFile.cc 
    HttpServerTask.cc
    RouteTable.cc
    Aspect.cc 
    HttpDef.cc    
    HttpServer.cc  
    Router.cc
    HttpCookie.cc   
    HttpMsg.cc   
    MultiPartParser.c  
    FileCache.cc  
)

add_library(${PROJECT_NAME} OBJECT ${SRC})

