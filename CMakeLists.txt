cmake_minimum_required(VERSION 3.10)

project(Docanto)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

message(STATUS "Build type is: " ${CMAKE_BUILD_TYPE})
message(STATUS "Using: " ${CMAKE_CXX_COMPILER_ID})

add_subdirectory(DocantoCLI)
add_subdirectory(DocantoLib)
