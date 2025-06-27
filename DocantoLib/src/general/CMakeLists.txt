add_library(DocantoGeneralLib STATIC
    Logger.cpp
    Timer.cpp
    File.cpp
 "Image.cpp")

target_include_directories(DocantoGeneralLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include/general> 
)


