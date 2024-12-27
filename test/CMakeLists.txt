cmake_minimum_required(VERSION 3.6)

set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "build type")

project(wfrest_test
		LANGUAGES C CXX
)

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

find_package(wfrest REQUIRED CONFIG HINTS ..)
include_directories(
	${OPENSSL_INCLUDE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${WFREST_INCLUDE_DIR}
	${WORKFLOW_INCLUDE_DIR}
)

link_directories(${WFREST_LIB_DIR} ${WORKFLOW_LIB_DIR})

find_program(CMAKE_MEMORYCHECK_COMMAND valgrind)
set(memcheck_command ${CMAKE_MEMORYCHECK_COMMAND} ${CMAKE_MEMORYCHECK_COMMAND_OPTIONS} --error-exitcode=1 --leak-check=full --show-leak-kinds=all)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

enable_testing()
find_package(GTest REQUIRED)

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -fPIC -pipe -std=gnu90")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -pipe -std=c++14 -fno-exceptions")

set(GTEST_LIB GTest::GTest GTest::Main)
set(WFREST_LIB wfrest workflow pthread OpenSSL::SSL OpenSSL::Crypto z)

set(UNIT_TEST_LIST
	TimeStamp_unittest
	StrUtil_unittest
	StringPiece_unittest
	Compress_unittest
	Router_unittest
	base64_unittest
	BluePrint_unittest
	HttpCookie_unittest
	RouteTable_unittest
	HttpDef_unittest
	FileUtil_unittest
	PathUtil_unittest
)

foreach(src ${UNIT_TEST_LIST})
	add_executable(${src} EXCLUDE_FROM_ALL ${src}.cc)
	target_link_libraries(${src} ${WFREST_LIB} ${GTEST_LIB})
	add_test(${src} ${src})
	add_dependencies(check ${src})
endforeach()

foreach(src ${UNIT_TEST_LIST})
    add_test(${src}-memory-check ${memcheck_command} ./${src})
endforeach()

set(SERVER_UNIT_TEST_LIST
	send_test
	param_test
	json_test
	file_test
	proxy_test
	static_test
	compute_test
	send_form_test
	blueprint_test
	cn_url_test
)

foreach(src ${SERVER_UNIT_TEST_LIST})
	add_executable(server_${src} EXCLUDE_FROM_ALL ./HttpServer/${src}.cc)
	target_link_libraries(server_${src} ${WFREST_LIB} ${GTEST_LIB})
	add_test(server_${src} server_${src})
	add_dependencies(check server_${src})
endforeach()


foreach(src ${SERVER_UNIT_TEST_LIST})
    add_test(server_${src}-memory-check ${memcheck_command} ./server_${src})
endforeach()

set(TEST_LIST
	router_test
	multi_part_test
	StringPiece_feature
	RouteTableNode_test
	multi_verb_test
)

foreach(src ${TEST_LIST})
	add_executable(${src} EXCLUDE_FROM_ALL ${src}.cc)
	target_link_libraries(${src} ${WFREST_LIB})
	add_dependencies(check ${src})
endforeach()


