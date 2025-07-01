#include "win/Direct2D.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <cstdlib>

std::shared_ptr<Window> g_window = nullptr;
std::shared_ptr<Direct2DRender> g_render = nullptr;

void paint_callback() {
}

void size_callback(Docanto::Geometry::Dimension<long> dims) {
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	Docanto::Logger::init();


	g_window = std::make_shared<Window>(hInstance);
	g_window->set_state(Window::WINDOW_STATE::NORMAL);
	g_window->set_window_size({ 50, 50, 500, 500 });
	g_window->set_callback_paint(paint_callback);
	g_window->set_callback_size(size_callback);

	g_render = std::make_shared<Direct2DRender>(g_window);

	while (!g_window->get_close_request()) {
		Window::get_window_messages(true);
		Docanto::Logger::print_to_debug();
	}

	g_window = nullptr;

	Docanto::Logger::print_to_debug();

	return 0;
}