#include "helper/include.h"
#include "MainWindow.h"

std::unique_ptr<WindowHandler> g_main_window;
std::unique_ptr<Direct2DRenderer> g_main_renderer;

// will be automatically destroyed after main_window_loop_run
Direct2DRenderer::TextFormatObject* g_text_format;
Direct2DRenderer::BrushObject* g_brush;

Direct2DRenderer::BitmapObject* g_bitmap = nullptr;

MuPDFHandler* g_mupdfcontext;
PDFHandler* g_pdfhandler = nullptr;

void callback_draw() {
	// draw scaled elements
	g_main_renderer->set_current_transform_active();
	g_main_renderer->begin_draw();
	g_main_renderer->clear(Renderer::Color(50, 50, 50));

	g_pdfhandler->render();
	// draw ui elements
	g_main_renderer->set_identity_transform_active();
	g_main_renderer->draw_text(L"DOCANTO ALPHA VERSION 0", Renderer::Point<float>(0, 0), *g_text_format, *g_brush);
	g_main_renderer->end_draw();
}

void callback_size(Renderer::Rectangle<long> r) {
	g_main_renderer->resize(r);
}

void callback_pointer_down(WindowHandler::PointerInfo p) {
/*	Logger::log("Pointer at " + std::to_string(p.pos.x) + ", " + std::to_string(p.pos.y));
	Logger::log("Buttons " + std::to_string(p.button1pressed) +
		", " + std::to_string(p.button2pressed) + ", " + std::to_string(p.button3pressed) +
		", " + std::to_string(p.button4pressed) + ", " + std::to_string(p.button5pressed));
	Logger::log("Pressure " + std::to_string(p.pressure)); 
	Logger::print_to_console(false); */
}

void callback_mousewheel(short delta, bool hwheel, Renderer::Point<int> center) {
	delta *= 1/g_main_renderer->get_transform_scale();
	if (WindowHandler::is_key_pressed(WindowHandler::LEFT_CONTROL)) {
		g_main_renderer->add_scale_matrix(delta > 0 ? 1.25 : 0.8, g_main_window->get_mouse_pos());
		Logger::log(g_main_renderer->get_transform_scale());
		Logger::print_to_debug();
	}
	else if (WindowHandler::is_key_pressed(WindowHandler::LEFT_SHIFT))
		g_main_renderer->add_transform_matrix({ (float)delta, 0});
	else
		g_main_renderer->add_transform_matrix({ 0, (float)delta }); 
	g_main_window->invalidate_drawing_area();
}

void main_window_loop_run(HINSTANCE h) {
	g_main_window = std::make_unique<WindowHandler>(APPLICATION_NAME, h);

	g_main_renderer = std::make_unique<Direct2DRenderer>(*g_main_window.get());
	auto default_brush = g_main_renderer->create_brush(Renderer::Color(255, 0, 0));
	g_brush = &default_brush;
	auto default_text_format = g_main_renderer->create_text_format(L"Consolas", 20);
	g_text_format = &default_text_format;

	MuPDFHandler context;
	PDFHandler pdf_handler; 
	g_mupdfcontext = &context;
	Direct2DRenderer::BitmapObject bitmap;

	auto path = FileHandler::open_file_dialog(L"PDF\0*.pdf\0\0", *g_main_window);
	if (path.has_value()) {
		pdf_handler = PDFHandler(g_main_renderer.get(), *g_mupdfcontext, path.value());
		g_pdfhandler = &pdf_handler; 
	}

	// do the callbacks
	g_main_window->set_callback_paint(callback_draw);
	g_main_window->set_callback_size(callback_size);
	g_main_window->set_callback_pointer_down(callback_pointer_down);
	g_main_window->set_callback_pointer_update(callback_pointer_down);
	g_main_window->set_callback_mousewheel(callback_mousewheel);

	g_main_window->set_state(WindowHandler::WINDOW_STATE::NORMAL);

	while(!g_main_window->close_request()) {
		g_main_window->get_window_messages(true);
	}
}