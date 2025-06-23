add_subdirectory(src/general)
add_subdirectory(src/pdf)

add_library(DocantoLib INTERFACE)

target_include_directories(DocantoLib INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include> 
)


target_link_libraries(DocantoLib INTERFACE DocantoGeneralLib)
target_link_libraries(DocantoLib INTERFACE DocantoPDFLib)