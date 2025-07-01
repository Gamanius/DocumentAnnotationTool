add_executable(DocantoWin WIN32 src/main.cpp "src/include.h" "src/win/Window.h" "src/win/Window.cpp" "src/win/Direct2D.cpp" "src/win/Direct2D.h" "src/ui/Caption.cpp" "src/ui/Caption.h")

target_link_libraries(DocantoWin PUBLIC DocantoLib)