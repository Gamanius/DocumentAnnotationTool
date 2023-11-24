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

void callback_pointer_down(WindowHandler::PointerInfo p) {
	Logger::log("Pointer at " + std::to_string(p.pos.x) + ", " + std::to_string(p.pos.y));
	Logger::log("Buttons " + std::to_string(p.button1pressed) +
		", " + std::to_string(p.button2pressed) + ", " + std::to_string(p.button3pressed) +
		", " + std::to_string(p.button4pressed) + ", " + std::to_string(p.button5pressed));
	Logger::log("Pressure " + std::to_string(p.pressure)); 
	Logger::print_to_console(); 
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
	g_main_window->set_callback_pointer_down(callback_pointer_down);
	g_main_window->set_callback_pointer_update(callback_pointer_down);

	g_main_window->set_state(WindowHandler::WINDOW_STATE::NORMAL);

	while(!g_main_window->close_request()) {
		g_main_window->get_window_messages(true);
	}
}