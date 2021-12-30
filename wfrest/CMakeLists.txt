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

# install(TARGETS wfrest DESTINATION lib)
install(TARGETS
        ${PROJECT_NAME}
    EXPORT wfrest-config
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
export(TARGETS
        ${PROJECT_NAME}
    FILE "${CMAKE_CURRENT_BINARY_DIR}/wfrest-config.cmake"
)

# todo : ${CMAKE_INSTALL_LIBDIR} can't find
set(CMAKE_CONFIG_INSTALL_DIR "/usr/local/lib/cmake/${PROJECT_NAME}")
message(${CMAKE_CONFIG_INSTALL_DIR})
install(EXPORT
        wfrest-config
    DESTINATION ${CMAKE_CONFIG_INSTALL_DIR}
)

file(GLOB HEADERS "*.h")
file(GLOB HEADERS_HPP "*.hpp")
file(GLOB HEADERS_INL "*.inl")
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
install(FILES ${HEADERS_INL} DESTINATION include/wfrest)