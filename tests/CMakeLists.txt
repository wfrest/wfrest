add_executable(string_piece_test string_piece_test.cc)
target_link_libraries(string_piece_test wfrest)

add_executable(router_test router_test.cc params_test.cc)
target_link_libraries(router_test wfrest)

add_executable(params_test params_test.cc)
target_link_libraries(params_test wfrest)