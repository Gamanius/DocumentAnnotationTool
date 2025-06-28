#include "win/Window.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	Docanto::Logger::init();

	Window w(hInstance);
	w.set_state(Window::WINDOW_STATE::NORMAL);

	while (true) {
		Window::get_window_messages(true);
	}

	Docanto::Logger::print_to_debug();
}