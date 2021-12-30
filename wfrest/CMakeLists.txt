set(SRCS
        StrUtil.cc
        HttpMsg.cc
        Router.cc
        RouteTable.cc
        UriUtil.cc
        HttpDef.cc
        HttpContent.cc
        MultiPartParser.c
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
        FileUtil.cc
        HttpCookie.cc
        BlockSeries.cc
        Mysql.cc
        )

add_library(wfrest ${SRCS})

target_link_libraries(wfrest workflow OpenSSL::SSL OpenSSL::Crypto pthread rt ZLIB::ZLIB)
if (Brotli_FOUND)
    target_link_libraries(wfrest Brotli_lib)
endif ()

install(TARGETS wfrest DESTINATION lib)

file(GLOB HEADERS "*.h")
file(GLOB HEADERS_HPP "*.hpp")

# set(HEADERS
#         Compress.h
#         HttpMsg.h
#         HttpServer.h
#         Logger.h
#         StringPiece.h
#         PathUtil.h
#         base64.h
#         FileUtil.h
#         Aspect.h
#         json.hpp
#         Mysql.h
#   )

install(FILES ${HEADERS} DESTINATION include/wfrest)
install(FILES ${HEADERS_HPP} DESTINATION include/wfrest)