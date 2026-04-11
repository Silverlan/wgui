include(${CMAKE_SOURCE_DIR}/cmake/pr_common.cmake)

option(WGUI_INCLUDE_DEPENDENCIES "Include dependencies required to build the wgui library?" ON)

set(PROJ_NAME wgui)
pr_add_library(${PROJ_NAME} SHARED)
pr_add_export_macro(${PROJ_NAME} DLLWGUI)

if (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# WINDOWS_CLANG_COMPILER_FIX
	target_compile_definitions(${PROJ_NAME} PUBLIC
		"MSAA_SAMPLE_COUNT=GET_MSAA_SAMPLE_COUNT()"
	)
endif()

pr_add_dependency(${PROJ_NAME} util_image TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} iglfw TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} util_unicode TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} datasystem TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} prosper TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} cmaterialsystem TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} util_formatted_text TARGET PUBLIC)

# pr_add_dependency(${PROJ_NAME} squish TARGET)
pr_add_dependency(${PROJ_NAME} bz2 TARGET)
pr_add_dependency(${PROJ_NAME} gli TARGET)
pr_add_third_party_dependency(${PROJ_NAME} exprtk)
pr_add_third_party_dependency(${PROJ_NAME} libpng)
pr_add_third_party_dependency(${PROJ_NAME} zlib)

# zlib
if(WIN32)
	set(ZLIB_LIB_NAME "zlib")
else()
	set(ZLIB_LIB_NAME "z")
endif()

# Reset paths (TODO: Why do we need to do this?)
set(ZLIB_INCLUDE_DIR ${DEPENDENCY_ZLIB_INCLUDE} CACHE STRING "" FORCE)
set(ZLIB_LIBRARY ${DEPENDENCY_ZLIB_LIBRARY} CACHE STRING "" FORCE)

if(UNIX)
	# Use system freetype
	find_package(Freetype REQUIRED)
	target_include_directories(${PROJ_NAME} PUBLIC ${FREETYPE_INCLUDE_DIR_freetype2})
	target_link_libraries(${PROJ_NAME} PUBLIC ${FREETYPE_LIBRARIES})
else()
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
	find_package(freetype_win REQUIRED)

	add_library(freetype SHARED IMPORTED)
	set_property(TARGET freetype PROPERTY IMPORTED_IMPLIB "${freetype_win_LIBRARIES}")

	target_include_directories(freetype INTERFACE ${freetype_win_INCLUDE_DIRS})
	set_target_properties(freetype PROPERTIES FOLDER third_party_libs)

	pr_add_third_party_dependency(${PROJ_NAME} freetype_win PUBLIC)
endif()

pr_init_module(${PROJ_NAME})

pr_add_compile_definitions(
	${PROJ_NAME}
		-DWGUI_DLL
)

add_subdirectory(examples)

pr_finalize(${PROJ_NAME})
