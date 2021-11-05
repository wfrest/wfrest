add_executable(01_basic 01_basic.cc)
target_link_libraries(01_basic wfrest)

add_executable(02_parameter 02_parameter_path.cc)
target_link_libraries(02_parameter wfrest)

add_executable(03_query_params 03_query_params.cc)
target_link_libraries(03_query_params wfrest)