cmake_minimum_required(VERSION 3.15)

set (PROJECT_NAME "pfmviewer")
project(${PROJECT_NAME} C CXX)

if (NOT DEFINED ISPC_EXECUTABLE)
    find_program (ISPC_EXECUTABLE ispc)
endif()

if (NOT ISPC_EXECUTABLE OR (${ISPC_EXECUTABLE} EQUAL ISPC_EXECUTABLE-NOTFOUND))
	set (ISPC_EXECUTABLE "$ENV{ProgramFiles}/ISPC/ispc-v1.18.0-windows/bin/ispc.exe")
endif()

if (NOT ISPC_EXECUTABLE OR (${ISPC_EXECUTABLE} EQUAL ISPC_EXECUTABLE-NOTFOUND))
	message(FATAL_ERROR "Failed to find ispc" )
endif()

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/simd_routines.o ${PROJECT_BINARY_DIR}/include/simd_routines.h
					COMMAND ${ISPC_EXECUTABLE}
					ARGS ${CMAKE_CURRENT_SOURCE_DIR}/simd_routines.ispc --target=avx512skx-x64 --arch=x86-64 --header-outfile=${PROJECT_BINARY_DIR}/include/simd_routines.h -o ${PROJECT_BINARY_DIR}/simd_routines.o
					DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/simd_routines.ispc
)

add_custom_target(simd_routinesGen DEPENDS ${PROJECT_BINARY_DIR}/include/simd_routines.h)
add_library(ispcLibrary STATIC ${PROJECT_BINARY_DIR}/simd_routines.o)
add_dependencies(ispcLibrary simd_routinesGen)
set_target_properties(
    ispcLibrary
    PROPERTIES
    LINKER_LANGUAGE C
)

set(CMAKE_CXX_STANDARD 20)

if(MSVC)
    set(CMAKE_CXX_FLAGS "/EHsc")
endif(MSVC)

add_executable(PfmViewerApp ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

add_dependencies(PfmViewerApp ispcLibrary)
add_library(nana STATIC IMPORTED)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set_target_properties(nana PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/nana/BIN/nana_v142_Debug_x64.lib")
else()
	set_target_properties(nana PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/nana/BIN/nana_v142_Release_x64.lib")
endif()
target_include_directories(PfmViewerApp PRIVATE ${PROJECT_BINARY_DIR}/include)
target_include_directories(PfmViewerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(PfmViewerApp ispcLibrary)
target_link_libraries(PfmViewerApp nana)
add_compile_definitions(STD_FILESYSTEM_FORCE)

set_property(TARGET PfmViewerApp PROPERTY
             MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
