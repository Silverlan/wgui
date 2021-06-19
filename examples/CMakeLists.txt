cmake_minimum_required(VERSION 3.12)

function(register_example SUB_PROJ_NAME)
	message("Processing example '${SUB_PROJ_NAME}'...")
	add_subdirectory(${SUB_PROJ_NAME})

	set(SOLUTION_DIR_NAME samples)
	set(ARGV ${ARGN})
	list(LENGTH ARGV ARGC)
	if(${ARGC} GREATER 0)
		list(GET ARGV 0 SOLUTION_DIR_NAME)
	endif()

	set_target_properties(${SUB_PROJ_NAME} PROPERTIES FOLDER ${SOLUTION_DIR_NAME})
endfunction(register_example)

register_example(sample_application)

set(CMAKE_CXX_STANDARD 20)
