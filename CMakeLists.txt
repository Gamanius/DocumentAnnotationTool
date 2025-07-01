cmake_minimum_required(VERSION 3.31)

project(Docanto)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

message(STATUS "Build type is: " ${CMAKE_BUILD_TYPE})
message(STATUS "Using: " ${CMAKE_CXX_COMPILER_ID})
message(STATUS "System: " ${CMAKE_HOST_SYSTEM_NAME})


set(MUPDF_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mupdf/include)

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    add_link_options($<$<CONFIG:Debug>:/INCREMENTAL>)
    add_compile_options($<$<CONFIG:Debug>:/Zi>)
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        set(MUPDF_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mupdf/platform/win32/x64/Release)
    else()
        set(MUPDF_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mupdf/platform/win32/x64/Debug)
    endif()
endif()

find_library(
    MUPDF_LIB
    NAMES mupdf libmupdf
    PATHS ${MUPDF_LIB_PATH}
    NO_DEFAULT_PATH
)

if (MUPDF_LIB)
    message(STATUS "Found libmupdf at " ${MUPDF_LIB_PATH})
else()
    message(FATAL_ERROR "MuPDF library not found!")
endif()


add_subdirectory(DocantoLib)
add_subdirectory(DocantoCLI)
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    add_subdirectory(DocantoWin)
endif()
