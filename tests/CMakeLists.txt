add_executable(string_piece_test string_piece_test.cc)
target_link_libraries(string_piece_test wfrest)

add_executable(router_test router_test.cc)
target_link_libraries(router_test wfrest)

add_executable(multi_part_test multi_part_test.cc)

add_executable(strutil_test strutil_test.cc)
target_link_libraries(strutil_test wfrest)

add_executable(strutil_split strutil_split.cc)
target_link_libraries(strutil_split wfrest)

add_executable(strutil_trim strutil_trim.cc)
target_link_libraries(strutil_trim wfrest)

add_executable(logger_test logger_test.cc)
target_link_libraries(logger_test wfrest)

add_executable(AsyncLogger_test AsyncLogger_test.cc)
target_link_libraries(AsyncLogger_test wfrest)
