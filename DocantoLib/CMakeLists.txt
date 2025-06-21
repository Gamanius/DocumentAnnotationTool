add_subdirectory(src/general)

add_library(DocantoLib INTERFACE)

target_include_directories(DocantoLib INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:src> 
)

target_link_libraries(DocantoLib INTERFACE DocantoGeneralLib)