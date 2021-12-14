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

add_executable(FileUtil_unittest FileUtil_unittest.cc)
target_link_libraries(FileUtil_unittest wfrest GTest::GTest)
add_test(NAME FileUtil_unittest COMMAND FileUtil_unittest)

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





