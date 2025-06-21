add_library(DocantoGeneralLib STATIC
    Logger.cpp
 "Timer.h" "Timer.cpp")

target_include_directories(DocantoGeneralLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> 
    $<INSTALL_INTERFACE:include>                   
)


