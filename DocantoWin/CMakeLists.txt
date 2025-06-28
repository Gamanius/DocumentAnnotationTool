add_executable(DocantoWin WIN32 src/main.cpp "src/win/include.h" "src/win/Window.h" "src/win/Window.cpp")

target_link_libraries(DocantoWin PUBLIC DocantoLib)