cmake_minimum_required(VERSION 3.12)

set(INCLUDE_DIRS)
function(add_include_dir IDENTIFIER)
	set(PRETTYNAME ${IDENTIFIER})
	set(ARGV ${ARGN})
	list(LENGTH ARGV ARGC)
	if(${ARGC} GREATER 0)
		list(GET ARGV 0 PRETTYNAME)
	endif()
	string(TOUPPER ${IDENTIFIER} UIDENTIFIER)

	set(${DEFAULT_DIR} "")
	set(DEPENDENCY_${UIDENTIFIER}_INCLUDE ${DEFAULT_DIR} CACHE PATH "Path to ${PRETTYNAME} include directory.")
	set(INCLUDE_DIRS ${INCLUDE_DIRS} DEPENDENCY_${UIDENTIFIER}_INCLUDE PARENT_SCOPE)
endfunction(add_include_dir)

set(LIBRARIES)
function(add_external_library IDENTIFIER)
	set(PRETTYNAME ${IDENTIFIER})
	set(ARGV ${ARGN})
	list(LENGTH ARGV ARGC)
	if(${ARGC} GREATER 0)
		list(GET ARGV 0 PRETTYNAME)
	endif()
	string(TOUPPER ${IDENTIFIER} UIDENTIFIER)

	set(DEPENDENCY_${UIDENTIFIER}_LIBRARY "" CACHE FILEPATH "Path to ${PRETTYNAME} library.")
	set(LIBRARIES ${LIBRARIES} DEPENDENCY_${UIDENTIFIER}_LIBRARY PARENT_SCOPE)
endfunction(add_external_library)

function(link_external_library IDENTIFIER)
	set(PRETTYNAME ${IDENTIFIER})
	set(ARGV ${ARGN})
	list(LENGTH ARGV ARGC)
	if(${ARGC} GREATER 0)
		list(GET ARGV 0 PRETTYNAME)
	endif()
	string(TOUPPER ${IDENTIFIER} UIDENTIFIER)

	set(${DEFAULT_DIR} "")
	set(DEPENDENCY_${UIDENTIFIER}_INCLUDE ${DEFAULT_DIR} CACHE PATH "Path to ${PRETTYNAME} include directory.")
	set(INCLUDE_DIRS ${INCLUDE_DIRS} DEPENDENCY_${UIDENTIFIER}_INCLUDE PARENT_SCOPE)

	set(DEPENDENCY_${UIDENTIFIER}_LIBRARY "" CACHE FILEPATH "Path to ${PRETTYNAME} library.")
	set(LIBRARIES ${LIBRARIES} DEPENDENCY_${UIDENTIFIER}_LIBRARY PARENT_SCOPE)
endfunction(link_external_library)

##### CONFIGURATION #####

set(PROJ_NAME sample_application)

project(${PROJ_NAME} CXX)

set(CMAKE_CXX_STANDARD 17)

link_external_library(sharedutils)
link_external_library(mathutil)
link_external_library(vfilesystem)
link_external_library(glfw)
link_external_library(iglfw)
link_external_library(datasystem)
link_external_library(materialsystem)
link_external_library(cmaterialsystem)
link_external_library(prosper)
link_external_library(vulkan)
link_external_library(anvil)
link_external_library(freetype)
link_external_library(glslang)
link_external_library(wgui)

add_include_dir(glm)
add_include_dir(anvil_build)

set(DEFINITIONS
	GLM_FORCE_DEPTH_ZERO_TO_ONE
	ANVIL_VULKAN_CPP
)

##### CONFIGURATION #####

foreach(def IN LISTS DEFINITIONS)
	add_definitions(-D${def})
endforeach(def)

function(def_vs_filters FILE_LIST)
	foreach(source IN LISTS FILE_LIST)
	    get_filename_component(source_path "${source}" PATH)
	    string(REPLACE "${CMAKE_CURRENT_LIST_DIR}" "" source_path_relative "${source_path}")
	    string(REPLACE "/" "\\" source_path_msvc "${source_path_relative}")
	    source_group("${source_path_msvc}" FILES "${source}")
	endforeach()
endfunction(def_vs_filters)

file(GLOB_RECURSE SRC_FILES
    "${CMAKE_CURRENT_LIST_DIR}/src/*.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/include/*.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/*.hpp"
)
add_executable(${PROJ_NAME} ${SRC_FILES})
if(WIN32)
	target_compile_options(${PROJ_NAME} PRIVATE /wd4251)
	target_compile_options(${PROJ_NAME} PRIVATE /wd4996)
endif()
def_vs_filters("${SRC_FILES}")

foreach(LIB IN LISTS LIBRARIES)
	target_link_libraries(${PROJ_NAME} ${${LIB}})
endforeach(LIB)

target_include_directories(${PROJ_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(${PROJ_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

foreach(INCLUDE_PATH IN LISTS INCLUDE_DIRS)
	target_include_directories(${PROJ_NAME} PRIVATE ${${INCLUDE_PATH}})
endforeach(INCLUDE_PATH)

set(TARGET_PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJ_NAME} PROPERTIES ${TARGET_PROPERTIES})
