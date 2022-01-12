cmake_minimum_required(VERSION 3.6)

project(unit_test)

find_package(GTest REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)

find_package(wfrest REQUIRED)
if(NOT wfrest_FOUND)
    message(FATAL_ERROR "can't find wfrest, please build it first")
endif()

if(GTest_FOUND)
    message(STATUS "Build wfrest unit test")
    enable_testing()
else()
    message(FATAL_ERROR  "can't find GTest,please install it first")
endif()

include_directories(${PROJECT_SOURCE_DIR})
# test

add_executable(router_test router_test.cc)
target_link_libraries(router_test wfrest)

add_executable(multi_part_test multi_part_test.cc)

add_executable(StringPiece_feature StringPiece_feature.cc)
target_link_libraries(StringPiece_feature wfrest)

add_executable(RouteTableNode_test RouteTableNode_test.cc)
target_link_libraries(RouteTableNode_test wfrest)

# unit test
add_executable(StrUtil_unittest StrUtil_unittest.cc)
target_link_libraries(StrUtil_unittest wfrest GTest::GTest)
add_test(NAME StrUtil_unittest COMMAND StrUtil_unittest)

add_executable(StringPiece_unittest StringPiece_unittest.cc)
target_link_libraries(StringPiece_unittest wfrest GTest::GTest)
add_test(NAME StringPiece_unittest COMMAND StringPiece_unittest)

add_executable(Compress_unittest Compress_unittest.cc)
target_link_libraries(Compress_unittest wfrest GTest::GTest)
add_test(NAME Compress_unittest COMMAND Compress_unittest)

add_executable(Router_unittest Router_unittest.cc)
target_link_libraries(Router_unittest wfrest GTest::GTest)
add_test(NAME Router_unittest COMMAND Router_unittest)

add_executable(base64_unittest base64_unittest.cc)
target_link_libraries(base64_unittest wfrest GTest::GTest)
add_test(NAME base64_unittest COMMAND base64_unittest)

add_executable(BluePrint_unittest BluePrint_unittest.cc)
target_link_libraries(BluePrint_unittest wfrest GTest::GTest)
add_test(NAME BluePrint_unittest COMMAND BluePrint_unittest)

add_executable(FileUtil_unittest FileUtil_unittest.cc FileTestUtil.cc)
target_link_libraries(FileUtil_unittest wfrest GTest::GTest)
add_test(NAME FileUtil_unittest COMMAND FileUtil_unittest)

add_executable(PathUtil_unittest PathUtil_unittest.cc FileTestUtil.cc)
target_link_libraries(PathUtil_unittest wfrest GTest::GTest)
add_test(NAME PathUtil_unittest COMMAND PathUtil_unittest)

add_executable(TimeStamp_unittest TimeStamp_unittest.cc)
target_link_libraries(TimeStamp_unittest wfrest GTest::GTest)
add_test(NAME TimeStamp_unittest COMMAND TimeStamp_unittest)

add_executable(HttpCookie_unittest HttpCookie_unittest.cc)
target_link_libraries(HttpCookie_unittest wfrest GTest::GTest)
add_test(NAME HttpCookie_unittest COMMAND HttpCookie_unittest)

add_executable(RouteTable_unittest RouteTable_unittest.cc)
target_link_libraries(RouteTable_unittest wfrest GTest::GTest)
add_test(NAME RouteTable_unittest COMMAND RouteTable_unittest)

add_executable(HttpDef_unittest HttpDef_unittest.cc)
target_link_libraries(HttpDef_unittest wfrest GTest::GTest)
add_test(NAME HttpDef_unittest COMMAND HttpDef_unittest)


# server

add_executable(server_send_unittest ./HttpServer/send_unittest.cc)
target_link_libraries(server_send_unittest wfrest GTest::GTest)
add_test(NAME server_send_unittest COMMAND server_send_unittest)

add_executable(server_param_unittest ./HttpServer/param_unittest.cc)
target_link_libraries(server_param_unittest wfrest GTest::GTest)
add_test(NAME server_param_unittest COMMAND server_param_unittest)

add_executable(server_json_unittest ./HttpServer/json_unittest.cc)
target_link_libraries(server_json_unittest wfrest GTest::GTest)
add_test(NAME server_json_unittest COMMAND server_json_unittest)

add_executable(server_file_unittest ./HttpServer/file_unittest.cc FileTestUtil.cc)
target_link_libraries(server_file_unittest wfrest GTest::GTest)
add_test(NAME server_file_unittest COMMAND server_file_unittest)

# add_executable(server_proxy_unittest ./HttpServer/proxy_unittest.cc)
# target_link_libraries(server_proxy_unittest wfrest GTest::GTest)
# add_test(NAME server_proxy_unittest COMMAND server_proxy_unittest)




