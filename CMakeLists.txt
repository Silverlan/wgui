cmake_minimum_required(VERSION 3.12)

set(INCLUDE_DIRS)
function(add_include_dir IDENTIFIER DEFAULT_INCLUDE_DIR)
	set(PRETTYNAME ${IDENTIFIER})
	set(ARGV ${ARGN})
	list(LENGTH ARGV ARGC)
	if(${ARGC} GREATER 0)
		list(GET ARGV 0 PRETTYNAME)
	endif()
	string(TOUPPER ${IDENTIFIER} UIDENTIFIER)

	set(DEPENDENCY_${UIDENTIFIER}_INCLUDE "${DEFAULT_INCLUDE_DIR}" CACHE PATH "Path to ${PRETTYNAME} include directory.")
	set(INCLUDE_DIRS ${INCLUDE_DIRS} DEPENDENCY_${UIDENTIFIER}_INCLUDE PARENT_SCOPE)
endfunction(add_include_dir)

function(setup_library_dependency_var LIB_PATH LIB_NAME STATIC_LIB VAR_NAME)
	if(WIN32)
		set(LIB_FILE_NAME "${LIB_NAME}.lib")
	else()
		if(STATIC_LIB MATCHES 1)
			set(FILE_EXTENSION ".a")
		else()
			set(FILE_EXTENSION ".so")
		endif()
		
		set(LIB_FILE_NAME "${LIB_NAME}${FILE_EXTENSION}")
	endif()
	set(${VAR_NAME} "${LIB_PATH}/${LIB_FILE_NAME}" CACHE PATH "Path to library." FORCE)
endfunction(setup_library_dependency_var)

set(LIBRARIES)
function(add_external_library IDENTIFIER DEFAULT_LIBRARY_DIR LIB_NAME STATIC_LIB)
	set(PRETTYNAME ${IDENTIFIER})
	set(ARGV ${ARGN})
	list(LENGTH ARGV ARGC)
	if(${ARGC} GREATER 0)
		list(GET ARGV 0 PRETTYNAME)
	endif()
	string(TOUPPER ${IDENTIFIER} UIDENTIFIER)

	setup_library_dependency_var("${DEFAULT_LIBRARY_DIR}" "${LIB_NAME}" ${STATIC_LIB} DEPENDENCY_${UIDENTIFIER}_LIBRARY)
	# set(DEPENDENCY_${UIDENTIFIER}_LIBRARY "" CACHE FILEPATH "Path to ${PRETTYNAME} library.")
	set(LIBRARIES ${LIBRARIES} DEPENDENCY_${UIDENTIFIER}_LIBRARY PARENT_SCOPE)
endfunction(add_external_library)

function(link_external_library IDENTIFIER DEFAULT_INCLUDE_DIR DEFAULT_LIBRARY_DIR LIB_NAME STATIC_LIB)
	set(PRETTYNAME ${IDENTIFIER})
	set(ARGV ${ARGN})
	list(LENGTH ARGV ARGC)
	if(${ARGC} GREATER 0)
		list(GET ARGV 0 PRETTYNAME)
	endif()
	string(TOUPPER ${IDENTIFIER} UIDENTIFIER)

	set(DEPENDENCY_${UIDENTIFIER}_INCLUDE "${DEFAULT_INCLUDE_DIR}" CACHE PATH "Path to ${PRETTYNAME} include directory.")
	set(INCLUDE_DIRS ${INCLUDE_DIRS} DEPENDENCY_${UIDENTIFIER}_INCLUDE PARENT_SCOPE)

	setup_library_dependency_var("${DEFAULT_LIBRARY_DIR}" "${LIB_NAME}" ${STATIC_LIB} DEPENDENCY_${UIDENTIFIER}_LIBRARY)
	# set(DEPENDENCY_${UIDENTIFIER}_LIBRARY "${DEFAULT_LIBRARY_DIR}" CACHE FILEPATH "Path to ${PRETTYNAME} library.")
	set(LIBRARIES ${LIBRARIES} DEPENDENCY_${UIDENTIFIER}_LIBRARY PARENT_SCOPE)
endfunction(link_external_library)

function(register_third_party_library LIB_NAME)
	message("Processing third-party library '${LIB_NAME}'...")
	set(extra_macro_args ${ARGN})

	list(LENGTH extra_macro_args num_extra_args)
	if(${num_extra_args} GREATER 0)
		list(GET extra_macro_args 0 optional_arg)
		add_subdirectory(third_party_libs/${LIB_NAME} third_party_libs/${optional_arg})
		return()
	endif()
	add_subdirectory(third_party_libs/${LIB_NAME})
endfunction(register_third_party_library)

##### CONFIGURATION #####

set(PROJ_NAME wgui)

project(${PROJ_NAME} CXX)

set(CMAKE_CXX_STANDARD 17)
set(ENABLE_STATIC_LIBRARY_FLAG 0)
# set(PRECOMPILED_HEADER "stdafx")

# Build Type
if(WIN32)
	set(CONFIG_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build Type")
else()
	set(CONFIG_BUILD_TYPE "Release" CACHE STRING "Build Type")
endif()
set_property(CACHE CONFIG_BUILD_TYPE PROPERTY STRINGS RelWithDebInfo Release Debug)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
if(UNIX)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()
set(CMAKE_BUILD_TYPE ${CONFIG_BUILD_TYPE} CACHE STRING "" FORCE)
#

if(UNIX)
	set(VULKAN_SDK "" CACHE PATH "Path to Vulkan SDK.")
	search_lib_recursive(DEPENDENCY_GLSLANG_OS_DEPENDENT_LIBRARY third_party_libs/anvil/deps/glslang/glslang/OSDependent/Unix OSDependent STATIC)
else()
	set(VULKAN_SDK "$ENV{VULKAN_SDK}" CACHE INTERNAL "Path to Vulkan SDK.")
endif()
link_external_library(vulkan "${VULKAN_SDK}/Include" "${VULKAN_SDK}/Lib" "vulkan-1" 0)
link_external_library(glslang "${CMAKE_CURRENT_LIST_DIR}/third_party_libs/anvil/deps/glslang" "${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/anvil/deps/glslang/glslang/${CMAKE_BUILD_TYPE}" "glslang" 1)

link_external_library(wgui "${CMAKE_CURRENT_LIST_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}" "wgui" 0)
link_external_library(sharedutils "${CMAKE_CURRENT_LIST_DIR}/external_libs/sharedutils/include" "${CMAKE_CURRENT_BINARY_DIR}/external_libs/sharedutils/${CMAKE_BUILD_TYPE}" "sharedutils" 0)
link_external_library(sharedutils_static "${CMAKE_CURRENT_LIST_DIR}/external_libs/sharedutils/include" "${CMAKE_CURRENT_BINARY_DIR}/external_libs/sharedutils-static/${CMAKE_BUILD_TYPE}" "sharedutils-static" 1)
set(DEPENDENCY_SHAREDUTILS_LIBRARY_STATIC ${DEPENDENCY_SHAREDUTILS_STATIC_LIBRARY} CACHE STRING "" FORCE)
link_external_library(mathutil "${CMAKE_CURRENT_LIST_DIR}/external_libs/mathutil/include" "${CMAKE_CURRENT_BINARY_DIR}/external_libs/mathutil/${CMAKE_BUILD_TYPE}" "mathutil" 0)
link_external_library(vfilesystem "${CMAKE_CURRENT_LIST_DIR}/external_libs/vfilesystem/include" "${CMAKE_CURRENT_BINARY_DIR}/external_libs/vfilesystem/${CMAKE_BUILD_TYPE}" "vfilesystem" 0)
link_external_library(iglfw "${CMAKE_CURRENT_LIST_DIR}/external_libs/iglfw/include" "${CMAKE_CURRENT_BINARY_DIR}/external_libs/iglfw/${CMAKE_BUILD_TYPE}" "iglfw" 0)
link_external_library(datasystem "${CMAKE_CURRENT_LIST_DIR}/external_libs/datasystem/include" "${CMAKE_CURRENT_BINARY_DIR}/external_libs/datasystem/${CMAKE_BUILD_TYPE}" "datasystem" 0)
link_external_library(materialsystem "${CMAKE_CURRENT_LIST_DIR}/external_libs/materialsystem/materialsystem/include" "${CMAKE_CURRENT_BINARY_DIR}/external_libs/materialsystem/${CMAKE_BUILD_TYPE}" "materialsystem" 0)
link_external_library(cmaterialsystem "${CMAKE_CURRENT_LIST_DIR}/external_libs/materialsystem/cmaterialsystem/include" "${CMAKE_CURRENT_BINARY_DIR}/external_libs/materialsystem/${CMAKE_BUILD_TYPE}" "cmaterialsystem" 0)
link_external_library(prosper "${CMAKE_CURRENT_LIST_DIR}/external_libs/prosper/include" "${CMAKE_CURRENT_BINARY_DIR}/external_libs/prosper/${CMAKE_BUILD_TYPE}" "prosper" 0)
link_external_library(util_formatted_text "${CMAKE_CURRENT_LIST_DIR}/external_libs/util_formatted_text/include" "${CMAKE_CURRENT_BINARY_DIR}/external_libs/util_formatted_text/${CMAKE_BUILD_TYPE}" "util_formatted_text" 1)
link_external_library(util_image "${CMAKE_CURRENT_LIST_DIR}/external_libs/util_image/include" "${CMAKE_CURRENT_BINARY_DIR}/external_libs/util_image/${CMAKE_BUILD_TYPE}" "util_image" 1)

link_external_library(glfw "${CMAKE_CURRENT_LIST_DIR}/third_party_libs/glfw/include" "${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/glfw/src/${CMAKE_BUILD_TYPE}" "glfw3dll" 0)
link_external_library(anvil "${CMAKE_CURRENT_LIST_DIR}/third_party_libs/anvil/include" "${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/anvil/${CMAKE_BUILD_TYPE}" "anvil" 1)
link_external_library(freetype "${CMAKE_CURRENT_LIST_DIR}/third_party_libs/freetype/include" "${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/freetype/${CMAKE_BUILD_TYPE}" "freetype" 0)

if(WIN32)
	set(LIBPNG_LIB_NAME "libpng16" CACHE INTERNAL "" FORCE)
else()
	set(LIBPNG_LIB_NAME "png16" CACHE INTERNAL "" FORCE)
endif()
link_external_library(lpng "${CMAKE_CURRENT_LIST_DIR}/third_party_libs/libpng" "${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/libpng/${CMAKE_BUILD_TYPE}" "${LIBPNG_LIB_NAME}" 0)
set(DEPENDENCY_LPNG_BUILD_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/libpng" CACHE PATH "Path to include directory.")

if(WIN32)
	set(ZLIB_LIB_NAME "zlib" CACHE INTERNAL "" FORCE)
else()
	set(ZLIB_LIB_NAME "z" CACHE INTERNAL "" FORCE)
endif()
link_external_library(zlib "${CMAKE_CURRENT_LIST_DIR}/third_party_libs/zlib" "${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/zlib/${CMAKE_BUILD_TYPE}" "${ZLIB_LIB_NAME}" 0)
set(DEPENDENCY_ZLIB_BUILD_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/zlib" CACHE PATH "Path to include directory.")
set(ZLIB_INCLUDE_DIR ${DEPENDENCY_ZLIB_INCLUDE} CACHE STRING "" FORCE)
set(ZLIB_LIBRARY ${DEPENDENCY_ZLIB_LIBRARY} CACHE STRING "" FORCE)

add_external_library(glslang_ogl_compiler "${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/anvil/deps/glslang/OGLCompilersDLL/${CMAKE_BUILD_TYPE}" "OGLCompiler" 1)
add_external_library(glslang_os_dependent "${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/anvil/deps/glslang/glslang/OSDependent/Windows/${CMAKE_BUILD_TYPE}" "OSDependent" 1)
add_external_library(spirv "${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/anvil/deps/glslang/SPIRV/${CMAKE_BUILD_TYPE}" "SPIRV" 1)
add_external_library(spirv_tools "${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/vulkan/spirv-tools/source/${CMAKE_BUILD_TYPE}" "SPIRV-Tools" 1)
add_external_library(spirv_tools_opt "${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/vulkan/spirv-tools/source/opt/${CMAKE_BUILD_TYPE}" "SPIRV-Tools-opt" 1)
add_external_library(spv_remapper "${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/anvil/deps/glslang/SPIRV/${CMAKE_BUILD_TYPE}" "SPVRemapper" 1)

add_include_dir(squish "${CMAKE_CURRENT_LIST_DIR}/third_party_libs/squish")
set(DEPENDENCY_SQUISH_LIBRARY "" CACHE PATH "Path to library.")
add_include_dir(glm "${CMAKE_CURRENT_LIST_DIR}/third_party_libs/glm")
add_include_dir(gli "${CMAKE_CURRENT_LIST_DIR}/third_party_libs/gli")
add_include_dir(anvil_build "${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/anvil/include")
add_include_dir(bzip2 "${CMAKE_CURRENT_LIST_DIR}/third_party_libs/bzip2")
add_include_dir(exprtk "${CMAKE_CURRENT_LIST_DIR}/third_party_libs/exprtk")

# Anvil
set(ANVIL_LINK_STATICALLY_WITH_VULKAN_LIB ON CACHE BOOL ON FORCE)
set(ANVIL_USE_BUILT_IN_VULKAN_HEADERS OFF CACHE BOOL OFF FORCE)
set(ENABLE_HLSL ON CACHE BOOL ".." FORCE)
set(SKIP_GLSLANG_INSTALL ON CACHE BOOL ON FORCE)
set(VULKAN_LIBRARY ${DEPENDENCY_VULKAN_LIBRARY} CACHE PATH "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL OFF FORCE)
register_third_party_library(anvil)
set(BUILD_SHARED_LIBS ON CACHE BOOL ON FORCE)
set_target_properties(Anvil PROPERTIES FOLDER third_party_libs/anvil)
set_target_properties(glslangValidator PROPERTIES FOLDER third_party_libs/anvil/tools)
set_target_properties(spirv-remap PROPERTIES FOLDER third_party_libs/anvil/tools)
set_target_properties(glslang PROPERTIES FOLDER third_party_libs/anvil/glslang)
set_target_properties(glslang-default-resource-limits PROPERTIES FOLDER third_party_libs/anvil/glslang)
set_target_properties(OGLCompiler PROPERTIES FOLDER third_party_libs/anvil/glslang)
set_target_properties(OSDependent PROPERTIES FOLDER third_party_libs/anvil/glslang)
set_target_properties(SPIRV PROPERTIES FOLDER third_party_libs/anvil/glslang)
set_target_properties(SPVRemapper PROPERTIES FOLDER third_party_libs/anvil/glslang)
#

# Vulkan
set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL ON FORCE)
set(SPIRV_SKIP_TESTS ON CACHE BOOL ".." FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL OFF FORCE)

add_subdirectory("${VULKAN_SDK}/spirv-tools" third_party_libs/vulkan/spirv-tools)
set(BUILD_SHARED_LIBS ON CACHE BOOL ".." FORCE)
set_target_properties(core_tables PROPERTIES FOLDER third_party_libs/anvil/tools/spirv)
set_target_properties(enum_string_mapping PROPERTIES FOLDER third_party_libs/anvil/tools/spirv)
set_target_properties(extinst_tables PROPERTIES FOLDER third_party_libs/anvil/tools/spirv)
set_target_properties(spirv-tools-pkg-config PROPERTIES FOLDER third_party_libs/anvil/tools/spirv)
set_target_properties(spirv-tools-shared-pkg-config PROPERTIES FOLDER third_party_libs/anvil/tools/spirv)

set_target_properties(spirv-tools-build-version PROPERTIES FOLDER third_party_libs/anvil/tools/spirv/build)
# set_target_properties(spirv-tools-debuginfo PROPERTIES FOLDER third_party_libs/anvil/tools/spirv/build)
set_target_properties(spirv-tools-header-DebugInfo PROPERTIES FOLDER third_party_libs/anvil/tools/spirv/build)
# set_target_properties(spirv-tools-spv-amd-gcn-shader PROPERTIES FOLDER third_party_libs/anvil/tools/spirv/build)
# set_target_properties(spirv-tools-spv-amd-shader-ballot PROPERTIES FOLDER third_party_libs/anvil/tools/spirv/build)
# set_target_properties(spirv-tools-spv-amd-shader-explicit-vertex-parameter PROPERTIES FOLDER third_party_libs/anvil/tools/spirv/build)
# set_target_properties(spirv-tools-spv-amd-shader-trinary-minmax PROPERTIES FOLDER third_party_libs/anvil/tools/spirv/build)

set_target_properties(SPIRV-Tools PROPERTIES FOLDER third_party_libs/anvil/tools/spirv/libraries)
set_target_properties(SPIRV-Tools-link PROPERTIES FOLDER third_party_libs/anvil/tools/spirv/libraries)
set_target_properties(SPIRV-Tools-opt PROPERTIES FOLDER third_party_libs/anvil/tools/spirv/libraries)
set_target_properties(SPIRV-Tools-reduce PROPERTIES FOLDER third_party_libs/anvil/tools/spirv/libraries)
set_target_properties(SPIRV-Tools-shared PROPERTIES FOLDER third_party_libs/anvil/tools/spirv/libraries)

set_target_properties(spirv-tools-vimsyntax PROPERTIES FOLDER third_party_libs/anvil/tools/spirv/utilities)
#

# Freetype
register_third_party_library(freetype)
set_target_properties(freetype PROPERTIES FOLDER third_party_libs)
target_include_directories(freetype PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/zlib)
#

# squish
if(WIN32)
	include_external_msproject(squish ${CMAKE_CURRENT_LIST_DIR}/third_party_libs/squish/vs9/squish/squish.vcproj)
	set_target_properties(squish PROPERTIES FOLDER third_party_libs)
endif()
#

# zlib
register_third_party_library(zlib)
set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/third_party_libs/zlib CACHE STRING "" FORCE)
set(DEPENDENCY_ZLIB_INCLUDE ${ZLIB_INCLUDE_DIR} CACHE STRING "" FORCE)
target_include_directories(zlib PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/zlib)
target_include_directories(zlibstatic PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/zlib)
set_target_properties(example PROPERTIES FOLDER third_party_libs/zlib)
set_target_properties(minigzip PROPERTIES FOLDER third_party_libs/zlib)
set_target_properties(zlib PROPERTIES FOLDER third_party_libs/zlib)
set_target_properties(zlibstatic PROPERTIES FOLDER third_party_libs/zlib)
#

# libpng
register_third_party_library(libpng)
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
set(VULKAN_INCLUDE_DIR ${DEPENDENCY_VULKAN_INCLUDE} CACHE STRING "" FORCE)
set(VULKAN_LIBRARY ${DEPENDENCY_VULKAN_LIBRARY} CACHE STRING "" FORCE)
register_third_party_library(glfw)
set_target_properties(glfw PROPERTIES FOLDER third_party_libs/glfw)
#

set(DEFINITIONS
	GLM_FORCE_DEPTH_ZERO_TO_ONE
	WGUI_DLL
	ANVIL_VULKAN_CPP
)

##### CONFIGURATION #####

set(LIB_TYPE SHARED)
if(ENABLE_STATIC_LIBRARY_FLAG MATCHES 1)
	set(CONFIG_STATIC_LIBRARY 0 CACHE BOOL "Build as static library?")
endif()
if(DEFINED ${CONFIG_STATIC_LIBRARY} AND ${CONFIG_STATIC_LIBRARY})
	set(LIB_TYPE STATIC)
endif()

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
add_library(${PROJ_NAME} ${LIB_TYPE} ${SRC_FILES})
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
if(DEFINED PRECOMPILED_HEADER)
	include(PrecompiledHeader.cmake)
	add_precompiled_header(${PROJ_NAME} "src/${PRECOMPILED_HEADER}.h" c++17 FORCEINCLUDE)
endif()
set_target_properties(${PROJ_NAME} PROPERTIES ${TARGET_PROPERTIES})

add_subdirectory(examples)

##### EXTERNAL LIBRARIES #####

message("Processing external libraries...")
set(MATHUTIL_ENABLE_MESH_FUNCTIONS OFF CACHE BOOL OFF FORCE)
set(CONFIG_ENABLE_VTF_SUPPORT OFF CACHE BOOL FORCE "Enable support for VMT/VTF files.")
add_subdirectory(external_libs)

target_include_directories(materialsystem PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/zlib)
target_include_directories(util_image PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/zlib)
target_include_directories(cmaterialsystem PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/third_party_libs/zlib)

##### DEPENDENCIES #####

add_dependencies(png zlib)
add_dependencies(freetype zlib)
add_dependencies(mathutil sharedutils-static)
add_dependencies(sharedutils mathutil)
add_dependencies(vfilesystem sharedutils)
add_dependencies(datasystem vfilesystem)
add_dependencies(materialsystem datasystem util_image png)
add_dependencies(iglfw glfw)
add_dependencies(iglfw vfilesystem)
add_dependencies(prosper vfilesystem iglfw Anvil util_image SPIRV SPIRV-Tools OSDependent OGLCompiler SPIRV-Tools-opt SPVRemapper glslang)

add_dependencies(cmaterialsystem materialsystem prosper)
add_dependencies(wgui cmaterialsystem freetype glslang util_formatted_text)
add_dependencies(sample_application wgui)
