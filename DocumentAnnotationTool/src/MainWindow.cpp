#include "helper/include.h"
#include "MainWindow.h"

std::unique_ptr<WindowHandler> g_main_window;
std::unique_ptr<Direct2DRenderer> g_main_renderer;

// will be automatically destroyed after main_window_loop_run
Direct2DRenderer::TextFormatObject* g_text_format;
Direct2DRenderer::BrushObject* g_brush;

Direct2DRenderer::BrushObject* color_red;
Direct2DRenderer::BrushObject* color_blue;

Direct2DRenderer::BrushObject* color_green;
Direct2DRenderer::BrushObject* color_yellow;
Direct2DRenderer::BrushObject* color_cyan;
Direct2DRenderer::BrushObject* color_magenta;

Direct2DRenderer::BitmapObject* g_bitmap = nullptr;

MuPDFHandler* g_mupdfcontext;
PDFHandler* g_pdfhandler = nullptr;

std::vector<Renderer::Rectangle<float>> new_rect;
Renderer::Rectangle<float> main_rec = {0, 0, 1000, 1000};
std::vector<Renderer::Rectangle<float>> rects;



void callback_draw(std::optional<std::vector<CachedBitmap*>*> highres_bitmaps, std::mutex* lock) {
	// draw scaled elements
	g_main_renderer->set_current_transform_active();
	g_main_renderer->begin_draw();
	g_main_renderer->clear(Renderer::Color(50, 50, 50));

	// when the highres_bitmaps arg is nullopt then it is a normal draw call from windows
	if (highres_bitmaps.has_value() == false)
		g_pdfhandler->render(g_main_window->get_hwnd());
	//else there are new bitmaps to render that have been created by other threads
	else if (lock != nullptr) {
		// thread safe
		std::lock_guard l(*lock); 
		auto bitmaps = highres_bitmaps.value();
		for (auto& i : *bitmaps) {
			if (g_main_renderer->inv_transform_rect(g_main_renderer->get_window_size()).intersects(i->dest))
				g_main_renderer->draw_bitmap(i->bitmap, i->dest, Direct2DRenderer::INTERPOLATION_MODE::LINEAR);
		}
	}
	else {
		auto bitmaps = highres_bitmaps.value(); 
		for (auto& i : *bitmaps) {
			if (g_main_renderer->inv_transform_rect(g_main_renderer->get_window_size()).intersects(i->dest))
				g_main_renderer->draw_bitmap(i->bitmap, i->dest, Direct2DRenderer::INTERPOLATION_MODE::LINEAR);
		}
	}
	//g_pdfhandler->debug_render();
	 
	// draw the rectangles

	//int size_before = 0;
	//auto arr = splice_rect(main_rec, rects);
	//size_before = arr.size();
	//merge_rects(arr);
	//Logger::log((int)(arr.size() - size_before));
	//
	//Logger::print_to_debug();
	//for (auto& i : arr) {
	//	g_main_renderer->draw_rect(i, *color_cyan, 5);
	//}
	//g_main_renderer->draw_rect(main_rec, *color_red);
	//for (size_t i = 0; i < rects.size(); i++) {
	//	g_main_renderer->draw_rect(rects[i], *color_yellow);
	//}
	
	// draw ui elements
	g_main_renderer->set_identity_transform_active();
	g_main_renderer->draw_text(L"DOCANTO ALPHA VERSION 0", Renderer::Point<float>(0, 0), *g_text_format, *g_brush);
	g_main_renderer->end_draw();
}

void callback_size(Renderer::Rectangle<long> r) {
	g_main_renderer->resize(r);
}


void callback_key_up(WindowHandler::VK k) {
	if (WindowHandler::is_key_pressed(WindowHandler::LEFT_CONTROL)) {
		auto mouse_pos = g_main_window->get_mouse_pos();
		if (k == WindowHandler::VK::OEM_PLUS ) {
			g_main_renderer->add_scale_matrix(1.25, mouse_pos);
		}
		else if (k == WindowHandler::VK::OEM_MINUS) {
			g_main_renderer->add_scale_matrix(0.8, mouse_pos);

		}
	}

	g_main_window->invalidate_drawing_area();
}

void callback_pointer_down(WindowHandler::PointerInfo p) {
/*	Logger::log("Pointer at " + std::to_string(p.pos.x) + ", " + std::to_string(p.pos.y));
	Logger::log("Buttons " + std::to_string(p.button1pressed) +
		", " + std::to_string(p.button2pressed) + ", " + std::to_string(p.button3pressed) +
		", " + std::to_string(p.button4pressed) + ", " + std::to_string(p.button5pressed));
	Logger::log("Pressure " + std::to_string(p.pressure)); 
	Logger::print_to_console(false); */
	p.pos = g_main_renderer->inv_transform_point(p.pos);
	static int index = 0;
	static Renderer::Point<float> p1 = {0, 0};
	if (index == 0) {
		p1 = p.pos;
		index++;
	}
	else {
		rects.push_back({ p1, p.pos });
		rects.at(rects.size() - 1).validate();
		index = 0;
	}
	g_main_window->invalidate_drawing_area();
}

void callback_mousewheel(short delta, bool hwheel, Renderer::Point<int> center) {
	delta *= 1/g_main_renderer->get_transform_scale();
	if (WindowHandler::is_key_pressed(WindowHandler::LEFT_CONTROL)) {
		g_main_renderer->add_scale_matrix(delta > 0 ? 1.25 : 0.8, center);
	}
	else if (WindowHandler::is_key_pressed(WindowHandler::LEFT_SHIFT) or hwheel)
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

	auto temp = g_main_renderer->create_brush(Renderer::Color(255, 0, 0));
	color_red = &temp;
	auto temp2 = g_main_renderer->create_brush(Renderer::Color(0, 0, 255));
	color_blue = &temp2;
	auto temp3 = g_main_renderer->create_brush(Renderer::Color(0, 255, 0));
	color_green = &temp3;
	auto temp4 = g_main_renderer->create_brush(Renderer::Color(255, 255, 0));
	color_yellow = &temp4;
	auto temp5 = g_main_renderer->create_brush(Renderer::Color(0, 255, 255));
	color_cyan = &temp5;
	auto temp6 = g_main_renderer->create_brush(Renderer::Color(255, 0, 255));
	color_magenta = &temp6;

	MuPDFHandler context;
	PDFHandler pdf_handler; 
	g_mupdfcontext = &context;
	Direct2DRenderer::BitmapObject bitmap;

	auto path = FileHandler::open_file_dialog(L"PDF\0*.pdf\0\0", *g_main_window);
	if (path.has_value()) {
		pdf_handler = PDFHandler(g_main_renderer.get(), *g_mupdfcontext, path.value());
		g_pdfhandler = &pdf_handler; 
	}
	else {
		return;
	}

	// do the callbacks
	g_main_window->set_callback_paint(callback_draw);
	g_main_window->set_callback_size(callback_size);
	g_main_window->set_callback_pointer_down(callback_pointer_down);
	//g_main_window->set_callback_pointer_update(callback_pointer_down);
	g_main_window->set_callback_mousewheel(callback_mousewheel);
	//g_main_window->set_callback_key_down(callback_key_down);
	g_main_window->set_callback_key_up(callback_key_up);

	g_main_window->set_state(WindowHandler::WINDOW_STATE::NORMAL);

	while(!g_main_window->close_request()) {
		g_main_window->get_window_messages(true);
	}
}