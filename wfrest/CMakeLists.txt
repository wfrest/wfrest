set(SRCS
        HttpMsg.cc
        Router.cc
        RouteTable.cc
        UriUtil.cc
        HttpDef.cc
        HttpContent.cc
        MultiPartParser.c
        StrUtil.cc
        HttpFile.cc
        PathUtil.cc
        HttpServerTask.cc
        HttpServer.cc
        Logger.cc
        LogStream.cc
        Timestamp.cc
        SysInfo.cc
        AsyncFileLogger.cc
        Compress.cc
        base64.cc
        BluePrint.cc
        )

add_library(wfrest ${SRCS})
# add_dependencies(wfrest workflow)

target_link_libraries(wfrest workflow OpenSSL::SSL OpenSSL::Crypto pthread rt ZLIB::ZLIB)
if (Brotli_FOUND)
    target_link_libraries(wfrest Brotli_lib)
endif ()

install(TARGETS wfrest DESTINATION lib)

# file(GLOB HEADERS "*.h")
set(HEADERS
        Compress.h
        HttpMsg.h
        HttpServer.h
        Logger.h
        StringPiece.h
        PathUtil.h
        base64.h
  )

install(FILES ${HEADERS} DESTINATION include/wfrest)
