set(SRCS
        WFHttpMsg.cc
        WFWebServerTask.cc
        WFWebServer.cc
        RouteTable.h Router.cc Router.h StringPiece.h UniquePtr.h Macro.h CallbackWrapper.h)

add_library(wfrest ${SRCS})
target_link_libraries(wfrest workflow OpenSSL::SSL OpenSSL::Crypto pthread rt)

install(TARGETS wfrest DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/wfrest)