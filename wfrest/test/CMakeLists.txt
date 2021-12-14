add_executable(router_test router_test.cc)
target_link_libraries(router_test wfrest)

add_executable(multi_part_test multi_part_test.cc)

add_executable(StringPiece_feature StringPiece_feature.cc)
target_link_libraries(StringPiece_feature wfrest)

add_executable(logger_test logger_test.cc)
target_link_libraries(logger_test wfrest)

add_executable(AsyncLogger_test AsyncLogger_test.cc)
target_link_libraries(AsyncLogger_test wfrest)

add_executable(RouteTableNode_test RouteTableNode_test.cc)
target_link_libraries(RouteTableNode_test wfrest)
