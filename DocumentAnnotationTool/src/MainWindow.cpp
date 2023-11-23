#include "helper/include.h"
#include "MainWindow.h"

std::unique_ptr<WindowHandler> g_main_window;
std::unique_ptr<Direct2DRenderer> g_main_renderer;

// will be automatically destroyed after main_window_loop_run
Direct2DRenderer::TextFormatObject* g_text_format;
Direct2DRenderer::BrushObject* g_brush;

void callback_draw() {
	g_main_renderer->clear(Renderer::Color(50, 50, 50));
	g_main_renderer->draw_text(L"Hello World", Renderer::Point<float>(100, 500), *g_text_format, *g_brush);
}

void callback_size(Renderer::Rectangle<long> r) {
	g_main_renderer->resize(r);
}

void main_window_loop_run(HINSTANCE h) {
	g_main_window = std::make_unique<WindowHandler>(APPLICATION_NAME, h);

	g_main_renderer = std::make_unique<Direct2DRenderer>(*g_main_window.get());
	auto default_brush = g_main_renderer->create_brush(Renderer::Color(255, 0, 0));
	g_brush = &default_brush;
	auto default_text_format = g_main_renderer->create_text_format(L"Consolas", 500);
	g_text_format = &default_text_format;


	// do the callbacks
	g_main_window->set_callback_paint(callback_draw);
	g_main_window->set_callback_size(callback_size);

	g_main_window->set_state(WindowHandler::WINDOW_STATE::NORMAL);

	while(!g_main_window->close_request()) {
		g_main_window->get_window_messages(true);
	}
}