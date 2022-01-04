add_executable(01_basic 01_basic.cc)
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

add_executable(10_logger 10_logger.cc)
target_link_libraries(10_logger wfrest)

add_executable(11_compute_handler 11_compute_handler.cc)
target_link_libraries(11_compute_handler wfrest)

add_executable(12_series_interface 12_series_interface.cc)
target_link_libraries(12_series_interface wfrest)

add_executable(13_compession 13_compession.cc)
target_link_libraries(13_compession wfrest)

add_executable(13_compess_client 13_compess_client.cc)
target_link_libraries(13_compess_client wfrest)

add_executable(14_blue_print 14_blue_print/blog.cc)
target_link_libraries(14_blue_print wfrest)

add_executable(15_serve_static_file 15_serve_static_file.cc)
target_link_libraries(15_serve_static_file wfrest)

add_executable(16_cookie_simple 16_cookie_simple.cc)
target_link_libraries(16_cookie_simple wfrest)

add_executable(16_cookie 16_cookie.cc)
target_link_libraries(16_cookie wfrest)

add_executable(17_server_config 17_server_config.cc)
target_link_libraries(17_server_config wfrest)

add_executable(18_aop 18_aop.cc)
target_link_libraries(18_aop wfrest)

add_executable(19_https 19_https.cc)
target_link_libraries(19_https wfrest)

add_executable(20_proxy 20_proxy.cc)
target_link_libraries(20_proxy wfrest)

add_executable(21_mysql_show_db 21_mysql/21_show_db.cc)
target_link_libraries(21_mysql_show_db wfrest)

add_executable(21_mysql_create_add_query 21_mysql/21_create_add_query.cc)
target_link_libraries(21_mysql_create_add_query wfrest)

add_executable(21_mysql_web_db 21_mysql/21_web_db.cc)
target_link_libraries(21_mysql_web_db wfrest)

add_executable(21_mysql_json 21_mysql/21_db_json.cc)
target_link_libraries(21_mysql_json wfrest)





