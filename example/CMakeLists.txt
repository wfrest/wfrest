add_executable(01_basic 01_basic.cc 05_header.cc)
target_link_libraries(01_basic wfrest)

add_executable(02_parameter 02_parameter_path.cc)
target_link_libraries(02_parameter wfrest)

add_executable(03_query_params 03_query_params.cc)
target_link_libraries(03_query_params wfrest)

add_executable(04_post_form 04_post_form.cc)
target_link_libraries(04_post_form wfrest)

add_executable(05_header 05_header.cc)
target_link_libraries(05_header wfrest)