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
        Global.cc
        PathUtil.cc
        HttpServerTask.cc
        HttpServer.cc
        Logger.cc
        LogStream.cc
        Timestamp.cc
        SysInfo.cc
        )

add_library(wfrest ${SRCS})
add_dependencies(wfrest workflow_repo)

target_link_libraries(wfrest workflow OpenSSL::SSL OpenSSL::Crypto pthread rt)

install(TARGETS wfrest DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/wfrest)
