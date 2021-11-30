add_executable(string_piece_test string_piece_test.cc)
target_link_libraries(string_piece_test wfrest)

add_executable(router_test router_test.cc)
target_link_libraries(router_test wfrest)

add_executable(multi_part_test multi_part_test.cc)

add_executable(StringPiece_feature StringPiece_feature.cc)
target_link_libraries(StringPiece_feature wfrest)

add_executable(logger_test_01 logger_test_01.cc)
target_link_libraries(logger_test_01 wfrest)

add_executable(AsyncLogger_test AsyncLogger_test.cc)
target_link_libraries(AsyncLogger_test wfrest)

add_executable(StrUtil_unittest StrUtil_unittest.cc)
target_link_libraries(StrUtil_unittest wfrest GTest::GTest)