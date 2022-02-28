file(GLOB SRCS "*.cc")

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
        Timestamp.cc
        SysInfo.cc
        Compress.cc
        base64.cc
        BluePrint.cc
        FileUtil.cc
        HttpCookie.cc
        MysqlUtil.cc
        ErrorCode.cc
        Aspect.cc
        )

add_library(wfrest ${SRCS})
target_link_libraries(wfrest workflow OpenSSL::SSL OpenSSL::Crypto pthread rt ZLIB::ZLIB)
set(CMAKE_TARGETS_FILE ${PROJECT_NAME}-targets)

# https://github.com/tevador/RandomX/issues/157
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
        EXPORT  ${CMAKE_TARGETS_FILE}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(CMAKE_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
message(STATUS  ${CMAKE_CONFIG_INSTALL_DIR})

install(EXPORT      ${CMAKE_TARGETS_FILE}
        DESTINATION ${CMAKE_CONFIG_INSTALL_DIR}
)

file(GLOB HEADERS "*.h")
file(GLOB HEADERS_HPP "*.hpp")
file(GLOB HEADERS_INL "*.inl")

install(FILES ${HEADERS}     DESTINATION include/wfrest)
install(FILES ${HEADERS_HPP} DESTINATION include/wfrest)
install(FILES ${HEADERS_INL} DESTINATION include/wfrest)

