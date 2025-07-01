include(${CMAKE_SOURCE_DIR}/cmake/pr_common.cmake)

option(WGUI_INCLUDE_DEPENDENCIES "Include dependencies required to build the wgui library?" ON)

set(PROJ_NAME wgui)
pr_add_library(${PROJ_NAME} SHARED)

pr_add_dependency(${PROJ_NAME} util_image TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} iglfw TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} util_unicode TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} datasystem TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} prosper TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} cmaterialsystem TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} util_formatted_text TARGET PUBLIC)

# pr_add_dependency(${PROJ_NAME} squish TARGET)
pr_add_dependency(${PROJ_NAME} bz2 TARGET)
pr_add_third_party_dependency(${PROJ_NAME} exprtk)
pr_add_third_party_dependency(${PROJ_NAME} gli)
pr_add_third_party_dependency(${PROJ_NAME} libpng)
pr_add_third_party_dependency(${PROJ_NAME} zlib)

# zlib
if(WIN32)
	set(ZLIB_LIB_NAME "zlib")
else()
	set(ZLIB_LIB_NAME "z")
endif()
if(WGUI_INCLUDE_DEPENDENCIES)
	pr_set_default_include_path(zlib "${CMAKE_CURRENT_LIST_DIR}/third_party_libs/zlib")
	pr_set_default_library_file_path(zlib "${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/zlib/${CMAKE_BUILD_TYPE}" SHARED LIB_NAME ${ZLIB_LIB_NAME})
endif()

# Reset paths (TODO: Why do we need to do this?)
set(ZLIB_INCLUDE_DIR ${DEPENDENCY_ZLIB_INCLUDE} CACHE STRING "" FORCE)
set(ZLIB_LIBRARY ${DEPENDENCY_ZLIB_LIBRARY} CACHE STRING "" FORCE)

if(WGUI_INCLUDE_DEPENDENCIES)
	# Freetype
	if(UNIX)
		find_package(Freetype REQUIRED)
	else()
		add_subdirectory(third_party_libs/freetype)
		target_include_directories(freetype PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/zlib)
	endif()
	set_target_properties(freetype PROPERTIES FOLDER third_party_libs)

	# squish
	add_subdirectory(third_party_libs/squish)
	set_target_properties(squish PROPERTIES FOLDER third_party_libs)
	#

	# zlib
	add_subdirectory(third_party_libs/zlib)
	set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/third_party_libs/zlib CACHE STRING "" FORCE)
	pr_set_include_path(zlib "${ZLIB_INCLUDE_DIR}" FORCE)
	target_include_directories(zlib PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/zlib)
	target_include_directories(zlibstatic PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/zlib)
	set_target_properties(example PROPERTIES FOLDER third_party_libs/zlib)
	set_target_properties(minigzip PROPERTIES FOLDER third_party_libs/zlib)
	set_target_properties(zlib PROPERTIES FOLDER third_party_libs/zlib)
	set_target_properties(zlibstatic PROPERTIES FOLDER third_party_libs/zlib)
	#

	# libpng
	add_subdirectory(third_party_libs/libpng)
	set_target_properties(genfiles PROPERTIES FOLDER third_party_libs/libpng)
	set_target_properties(png PROPERTIES FOLDER third_party_libs/libpng)
	set_target_properties(png_static PROPERTIES FOLDER third_party_libs/libpng)
	set_target_properties(pngfix PROPERTIES FOLDER third_party_libs/libpng)
	set_target_properties(png-fix-itxt PROPERTIES FOLDER third_party_libs/libpng)
	set_target_properties(pngimage PROPERTIES FOLDER third_party_libs/libpng)
	set_target_properties(pngstest PROPERTIES FOLDER third_party_libs/libpng)
	set_target_properties(pngtest PROPERTIES FOLDER third_party_libs/libpng)
	set_target_properties(pngunknown PROPERTIES FOLDER third_party_libs/libpng)
	set_target_properties(pngvalid PROPERTIES FOLDER third_party_libs/libpng)

	target_include_directories(png PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/zlib)
	#

	# GLFW
	set(GLFW_BUILD_DOCS OFF CACHE BOOL OFF FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL OFF FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE BOOL OFF FORCE)
	set(GLFW_INSTALL OFF CACHE BOOL OFF FORCE)
	add_subdirectory(third_party_libs/glfw)
	set_target_properties(glfw PROPERTIES FOLDER third_party_libs/glfw)
	#
else()
	if(UNIX)
		find_package(Freetype REQUIRED)
		target_include_directories(${PROJ_NAME} PUBLIC ${FREETYPE_INCLUDE_DIR_freetype2})
		target_link_libraries(${PROJ_NAME} PUBLIC ${FREETYPE_LIBRARIES})
	else()
		set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
		pr_add_third_party_dependency(${PROJ_NAME} freetype_win PUBLIC)
	endif()
endif()

if(WGUI_INCLUDE_DEPENDENCIES)
	message("Processing external libraries...")

	unset(MATHUTIL_ENABLE_MESH_FUNCTIONS CACHE)
	SET(MATHUTIL_ENABLE_MESH_FUNCTIONS OFF CACHE BOOL "")

	unset(CONFIG_ENABLE_VTF_SUPPORT CACHE)
	SET(CONFIG_ENABLE_VTF_SUPPORT OFF CACHE BOOL "Enable support for VMT/VTF files.")

	add_subdirectory(external_libs)

	target_include_directories(materialsystem PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/zlib)
	target_include_directories(util_image PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/zlib)
	target_include_directories(cmaterialsystem PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/zlib)

	##### DEPENDENCIES #####

	add_dependencies(png zlib)
	if(WIN32)
		add_dependencies(freetype zlib)
	endif()
	add_dependencies(mathutil sharedutils-static)
	add_dependencies(sharedutils mathutil)
	add_dependencies(vfilesystem sharedutils)
	add_dependencies(datasystem vfilesystem)
	add_dependencies(materialsystem datasystem util_image png)
	add_dependencies(iglfw glfw)
	add_dependencies(iglfw vfilesystem)
	add_dependencies(prosper vfilesystem iglfw util_image)

	add_dependencies(cmaterialsystem materialsystem prosper)
	add_dependencies(util_formatted_text util_unicode)
	add_dependencies(wgui cmaterialsystem freetype util_formatted_text)
	add_dependencies(sample_application wgui)
endif()

pr_add_headers(${PROJ_NAME} "include/")
pr_add_sources(${PROJ_NAME} "src/")

pr_add_compile_definitions(
	${PROJ_NAME}
		-DWGUI_DLL
)

add_subdirectory(examples)

pr_finalize(${PROJ_NAME})
