add_executable(StrUtil_unittest StrUtil_unittest.cc)
target_link_libraries(StrUtil_unittest wfrest GTest::GTest)
add_test(NAME StrUtil_unittest COMMAND StrUtil_unittest)

add_executable(StringPiece_unittest StringPiece_unittest.cc)
target_link_libraries(StringPiece_unittest wfrest GTest::GTest)
add_test(NAME StringPiece_unittest COMMAND StringPiece_unittest)

add_executable(Compress_unittest Compress_unittest.cc)
target_link_libraries(Compress_unittest wfrest GTest::GTest)
add_test(NAME Compress_unittest COMMAND Compress_unittest)