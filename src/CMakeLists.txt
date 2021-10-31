set(SRCS
        WFHttpMsg.cc
        WFWebServerTask.cc
        WFWebServer.cc
)

add_library(wfrest ${SRCS})
target_link_libraries(wfrest workflow OpenSSL::SSL OpenSSL::Crypto pthread rt spdlog)

install(TARGETS wfrest DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/wfrest)