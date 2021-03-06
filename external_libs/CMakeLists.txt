cmake_minimum_required(VERSION 3.12)

project(external_libs)

function(register_external_lib SUB_PROJ_NAME)
	message("Processing external library '${SUB_PROJ_NAME}'...")
	add_subdirectory(${SUB_PROJ_NAME})

	set(SOLUTION_DIR_NAME external_libs)
	set(ARGV ${ARGN})
	list(LENGTH ARGV ARGC)
	if(${ARGC} GREATER 0)
		list(GET ARGV 0 SOLUTION_DIR_NAME)
	endif()

	set_target_properties(${SUB_PROJ_NAME} PROPERTIES FOLDER ${SOLUTION_DIR_NAME})
endfunction(register_external_lib)

register_external_lib(datasystem)
register_external_lib(iglfw)
register_external_lib(materialsystem)
register_external_lib(mathutil)
register_external_lib(prosper)
register_external_lib(sharedutils)
register_external_lib(util_image external_libs/util)
register_external_lib(util_formatted_text external_libs/util)
register_external_lib(vfilesystem)

message("Processing external library 'sharedutils-static'...")
add_subdirectory(sharedutils-static)
set_target_properties(sharedutils-static PROPERTIES FOLDER external_libs)

set_target_properties(materialsystem PROPERTIES FOLDER external_libs/materialsystem)
set_target_properties(cmaterialsystem PROPERTIES FOLDER external_libs/materialsystem)

set(CMAKE_CXX_STANDARD 20)
