cmake_minimum_required(VERSION 3.6)

set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "build type")

project(wfrest_example
		LANGUAGES C CXX
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

if (NOT "$ENV{LIBRARY_PATH}" STREQUAL "")
	string(REPLACE ":" ";" LIBRARY_PATH $ENV{LIBRARY_PATH})
	set(CMAKE_SYSTEM_LIBRARY_PATH ${LIBRARY_PATH};${CMAKE_SYSTEM_LIBRARY_PATH})
endif ()

if (NOT "$ENV{CPLUS_INCLUDE_PATH}" STREQUAL "")
	string(REPLACE ":" ";" INCLUDE_PATH $ENV{CPLUS_INCLUDE_PATH})
	set(CMAKE_SYSTEM_INCLUDE_PATH ${INCLUDE_PATH};${CMAKE_SYSTEM_INCLUDE_PATH})
endif ()

find_package(OpenSSL REQUIRED)

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/workflow/workflow-config.cmake.in")
	find_package(Workflow REQUIRED CONFIG HINTS ../workflow)
endif ()

find_package(ZLIB REQUIRED)

find_package(wfrest REQUIRED CONFIG HINTS ..)
include_directories(
	${OPENSSL_INCLUDE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${WORKFLOW_INCLUDE_DIR}
	${WFREST_INCLUDE_DIR}
)

link_directories(${WFREST_LIB_DIR} ${WORKFLOW_LIB_DIR})

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -fPIC -pipe -std=gnu90")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -pipe -std=c++11 -fno-exceptions")

if (APPLE)
	set(WFREST_LIB wfrest workflow pthread OpenSSL::SSL OpenSSL::Crypto protobuf z)
else ()
	set(WFREST_LIB wfrest)
endif ()

set(EXAMPLE_LIST
    01_basic
    02_parameter_path
    03_query_params
    04_post_form
    05_header
    06_file_send
    07_file_save
    08_upload_file
    09_json
    11_compute_handler
    12_series_interface
    13_compession
    13_compess_client
    # 14
    15_serve_static_file
    16_cookie_simple
    16_cookie
    17_server_config
    18_aop
    18_global_aop
    19_https
    20_proxy
    21_mysql
    22_redis
    23_compute
    24_peer_ip_address
    25_cn_url_server
    25_cn_url_client
    26_timer
    27_sse
    28_benchmark_static_files
    29_file_cache
    30_cache_benchmark
)

foreach(src ${EXAMPLE_LIST})
	add_executable(${src} ${src}.cc)
	target_link_libraries(${src} ${WFREST_LIB})
endforeach()

add_executable(14_blueprint 14_blue_print/blog.cc)
target_link_libraries(14_blueprint ${WFREST_LIB})




