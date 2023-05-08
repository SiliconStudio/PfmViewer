cmake_minimum_required(VERSION 3.12)

set (PROJECT_NAME "pfmviewer")
project(${PROJECT_NAME} C CXX)

if (NOT DEFINED ISPC_EXECUTABLE)
    find_program (ISPC_EXECUTABLE ispc)
endif()

if (NOT ISPC_EXECUTABLE OR (${ISPC_EXECUTABLE} STREQUAL ISPC_EXECUTABLE-NOTFOUND))
	set (ISPC_EXECUTABLE "$ENV{ProgramFiles}/ISPC/ispc-v1.18.0-windows/bin/ispc.exe")
endif()

if (NOT ISPC_EXECUTABLE OR (${ISPC_EXECUTABLE} STREQUAL ISPC_EXECUTABLE-NOTFOUND))
	message(FATAL_ERROR "Failed to find ispc" )
endif()

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/simple.o ${PROJECT_BINARY_DIR}/include/simple.h
					COMMAND ${ISPC_EXECUTABLE}
					ARGS ${CMAKE_CURRENT_SOURCE_DIR}/simple.ispc --target=avx512skx-x64 --arch=x86-64 --header-outfile=${PROJECT_BINARY_DIR}/include/simple.h -o ${PROJECT_BINARY_DIR}/simple.o
					DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/simple.ispc
)

add_custom_target(simpleGen DEPENDS ${PROJECT_BINARY_DIR}/include/simple.h)
add_library(ispcLibrary STATIC ${PROJECT_BINARY_DIR}/simple.o)
add_dependencies(ispcLibrary simpleGen)
SET_TARGET_PROPERTIES(
    ispcLibrary
    PROPERTIES
    LINKER_LANGUAGE C
)

add_executable(PfmViewerApp ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
add_dependencies(PfmViewerApp ispcLibrary)
target_include_directories(PfmViewerApp PRIVATE ${PROJECT_BINARY_DIR}/include)

target_link_libraries(PfmViewerApp ispcLibrary)