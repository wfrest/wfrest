cmake_minimum_required(VERSION 3.6)

if (NOT "$ENV{LIBRARY_PATH}" STREQUAL "")
	string(REPLACE ":" ";" LIBRARY_PATH $ENV{LIBRARY_PATH})
	set(CMAKE_SYSTEM_LIBRARY_PATH ${LIBRARY_PATH};${CMAKE_SYSTEM_LIBRARY_PATH})
endif ()

if (NOT "$ENV{CPLUS_INCLUDE_PATH}" STREQUAL "")
	string(REPLACE ":" ";" INCLUDE_PATH $ENV{CPLUS_INCLUDE_PATH})
	set(CMAKE_SYSTEM_INCLUDE_PATH ${INCLUDE_PATH};${CMAKE_SYSTEM_INCLUDE_PATH})
endif ()

find_package(OpenSSL REQUIRED)
set (HAVE_SYS_UIO_H_01 true)

if (WITH_VCPKG_TOOLCHAIN)
	find_package(Workflow REQUIRED CONFIG)
else ()
	# Try to find the package if workflow is not installed, otherwise proceed with find_package()
	if (NOT WORKFLOW_INSTALLED)
		find_package(Workflow REQUIRED CONFIG HINTS ../workflow)
	endif ()
endif()

include_directories(
	${OPENSSL_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${WORKFLOW_INCLUDE_DIR}
	${INC_DIR}/wfrest
)

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -fPIC -pipe -std=gnu90")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -pipe -std=c++11 -fno-exceptions")

add_subdirectory(base)
add_subdirectory(util)
add_subdirectory(core)

add_dependencies(base LINK_HEADERS)
add_dependencies(util LINK_HEADERS)
add_dependencies(core LINK_HEADERS)

add_library(
	${PROJECT_NAME} STATIC
	$<TARGET_OBJECTS:base>
	$<TARGET_OBJECTS:util>
	$<TARGET_OBJECTS:core>
)
add_dependencies(${PROJECT_NAME} LINK_HEADERS)

install(
	TARGETS ${PROJECT_NAME}
	ARCHIVE
	DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT devel
)

if (APPLE)
	set(LIBSO ${LIB_DIR}/libwfrest.a)
else ()
	set(LIBSO ${LIB_DIR}/libwfrest.so)
	add_custom_target(
		SCRIPT_SHARED_LIB ALL
		COMMAND ${CMAKE_COMMAND} -E echo 'GROUP ( libwfrest.a AS_NEEDED ( libz.so libworkflow.so ) ) ' > ${LIBSO}
	)
	add_dependencies(SCRIPT_SHARED_LIB ${PROJECT_NAME})
endif ()

install(
	FILES ${LIBSO}
	DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT devel
)

install (
	TARGETS ${CMAKE_PROJECT_NAME} EXPORT ${CMAKE_PROJECT_NAME}-targets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install (
	EXPORT ${CMAKE_PROJECT_NAME}-targets
	FILE ${CMAKE_PROJECT_NAME}-targets.cmake
	DESTINATION "${CMAKE_CONFIG_INSTALL_DIR}"
)


