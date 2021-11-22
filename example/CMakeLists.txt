add_executable(01_basic 01_basic.cc 05_header.cc 09_json.cc)
target_link_libraries(01_basic wfrest)

add_executable(02_parameter 02_parameter_path.cc)
target_link_libraries(02_parameter wfrest)

add_executable(03_query_params 03_query_params.cc)
target_link_libraries(03_query_params wfrest)

add_executable(04_post_form 04_post_form.cc)
target_link_libraries(04_post_form wfrest)

add_executable(05_header 05_header.cc)
target_link_libraries(05_header wfrest)

add_executable(06_file_read 06_file_read.cc)
target_link_libraries(06_file_read wfrest)

add_executable(07_file_write 07_file_write.cc)
target_link_libraries(07_file_write wfrest)

add_executable(08_upload_file 08_upload_file.cc)
target_link_libraries(08_upload_file wfrest)

add_executable(09_json 09_json.cc)
target_link_libraries(09_json wfrest)

