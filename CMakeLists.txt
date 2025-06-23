cmake_minimum_required(VERSION 3.10)

project(Docanto)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

message(STATUS "Build type is: " ${CMAKE_BUILD_TYPE})
message(STATUS "Using: " ${CMAKE_CXX_COMPILER_ID})
message(STATUS "System: " ${CMAKE_HOST_SYSTEM_NAME})


if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
	message(STATUS "Running setup.cmd to compile mupdf")
	execute_process(
		COMMAND cmd.exe /c setup.cmd
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		RESULT_VARIABLE _setup_result
		COMMAND_ECHO STDOUT
	)
	if (NOT _setup_result EQUAL 0)
        message(FATAL_ERROR "setup.cmd failed with exit code ${_setup_result}")
	endif()
    set(MUPDF_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mupdf/include)
    set(MUPDF_DEBUG_BUILD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mupdf/platform/win32/x64/Debug)
    set(MUPDF_RELEASE_BUILD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mupdf/platform/win32/x64/Debug)
endif()

message(STATUS MuPDF lib @ ${MUPDF_DEBUG_BUILD_PATH})

add_subdirectory(DocantoCLI)
add_subdirectory(DocantoLib)
