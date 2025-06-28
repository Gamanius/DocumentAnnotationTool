#include "win/Window.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <cstdlib>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	Docanto::Logger::init();

	Window w(hInstance);
	w.set_state(Window::WINDOW_STATE::NORMAL);
	w.set_window_size({ 50, 50, 500, 500 });

	while (!w.get_close_request()) {
		Window::get_window_messages(true);
		Docanto::Logger::print_to_debug();
	}

	Docanto::Logger::print_to_debug();
}