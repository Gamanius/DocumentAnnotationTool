add_executable(DocantoWin WIN32 src/main.cpp "src/include.h" "src/win/Window.h" "src/win/Window.cpp" "src/win/Direct2D.cpp" "src/win/Direct2D.h" "src/ui/Caption.cpp" "src/ui/Caption.h" "src/helper/Geometry.h" "src/win/MainWindowHandler.cpp" "src/pdf/PDFHandler.h" "src/pdf/PDFHandler.cpp" "src/ui/Gesture.cpp" "src/ui/Gesture.h")

target_include_directories(DocantoWin PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(DocantoWin PUBLIC DocantoLib)