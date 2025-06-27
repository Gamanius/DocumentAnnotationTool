add_executable(DocantoCLI src/main.cpp "src/stb_image_write.h")

target_link_libraries(DocantoCLI PUBLIC DocantoLib)