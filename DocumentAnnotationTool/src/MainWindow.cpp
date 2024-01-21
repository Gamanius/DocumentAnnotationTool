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
Renderer::Rectangle<float> r1 = {0, 0, 100, 100};
Renderer::Rectangle<float> r2 = {50, 50, 25, 25};

template <typename T>
std::array<std::optional<Renderer::Rectangle<T>>, 4> splice_rect(Renderer::Rectangle<T> r1, Renderer::Rectangle<T> r2) {
	// validate the rectangles just in case
	r1.validate();
	r2.validate();

	std::array<std::optional<Renderer::Rectangle<T>>, 4> return_arr = { std::nullopt, std::nullopt, std::nullopt, std::nullopt };
	// first we check if they overlap
	if (r1.intersects(r2) == false)
		return return_arr;
	// now we check for every point
	std::array<bool, 4> corner_check = {false, false, false, false};
	// upperleft
	if (r1.intersects(r2.upperleft()))
		corner_check[0] = true;
	// upperright
	if (r1.intersects(r2.upperright()))
		corner_check[1] = true;
	// bottomleft
	if (r1.intersects(r2.lowerleft())) 
		corner_check[2] = true;
	// bottomright
	if (r1.intersects(r2.lowerright())) 
		corner_check[3] = true;

	// count the number of corners that are inside
	byte num_corner_inside = 0;
	for (size_t i = 0; i < corner_check.size(); i++) {
		if (corner_check[i])
			num_corner_inside++;
	}

	if (num_corner_inside == 3) {
		// well we shouldn't be here
		Logger::log("This should not happen. Rectangles are weird...");
		return return_arr;
	}

	// case for 1 corner inside
	if (num_corner_inside == 1) {
		// upperleft is inside https://www.desmos.com/geometry/x17ptqj7yk
		if (corner_check[0] == true) {
			return_arr[0] = { {r1.x, r2.y}, {r2.x, r1.bottom() }};
			return_arr[1] = { r1.upperleft(), r2.upperleft()};
			return_arr[2] = { {r2.x, r1.y}, {r1.right(), r2.y} };
		}
		// upperright is inside https://www.desmos.com/geometry/clide4boo4
		else if (corner_check[1] == true) {
			return_arr[0] = { r2.upperright(), r1.lowerright()};  
			return_arr[1] = { r1.upperright(), r2.upperright() };
			return_arr[2] = { {r1.x, r2.y}, {r2.right(), r1.y} };
		}
		// bottomleft is inside https://www.desmos.com/geometry/slaar3gg52
		else if (corner_check[2] == true) {
			return_arr[0] = { r2.lowerleft(), r1.lowerright() };
			return_arr[1] = { r2.lowerleft(), r1.lowerleft() }; 
			return_arr[2] = { {r1.x, r2.bottom()}, {r2.x, r1.y} }; 
		}
		// bottomright is inside https://www.desmos.com/geometry/w8a8tnweec
		else if (corner_check[3] == true) {
			return_arr[0] = { r2.lowerright() , r1.lowerright() }; 
			return_arr[1] = { { r1.x, r2.bottom()}, { r2.right(), r1.bottom() } }; 
			return_arr[2] = { { r2.right(), r1.y }, { r1.right(), r2.bottom() } };  
		}
		return return_arr; 
	}

	if (num_corner_inside == 2) {
		// upperleft and upperright https://www.desmos.com/geometry/p8by0k5swk
		if (corner_check[0] == true and corner_check[1] == true) { 
			return_arr[0] = { r1.upperleft(), {r2.x, r1.bottom()}};
			return_arr[1] = { {r2.x, r1.y}, r2.upperright() };
			return_arr[2] = {{r2.right(), r1.y}, r1.lowerright() };
		}
		// upperleft and bottomleft https://www.desmos.com/geometry/e18umhhck7
		else if (corner_check[0] == true and corner_check[2] == true) {
			return_arr[0] = { r1.upperleft(), {r1.right(), r2.y} };
			return_arr[1] = { {r1.x, r2.y}, r2.lowerleft() };
			return_arr[2] = { {r1.x, r2.bottom()}, r1.lowerright() }; 
		}
		// bottomleft and bottomright https://www.desmos.com/geometry/g2fplsakd6
		else if (corner_check[2] == true and corner_check[3] == true) {
			return_arr[0] = { r1.upperleft(), {r2.x, r1.bottom()} };
			return_arr[1] = { r2.lowerleft(), {r2.right(), r1.bottom()}};
			return_arr[2] = { {r2.right(), r1.y}, r1.lowerright() };
		}
		// bottomright and upperright https://www.desmos.com/geometry/vwe0vjjtsm
		else if (corner_check[1] == true and corner_check[3] == true) {
			return_arr[0] = { r1.upperleft(), {r1.right(), r2.y} };
			return_arr[1] = { {r2.right(), r2.y}, {r1.right(), r2.bottom() }};
			return_arr[2] = { {r1.x, r2.bottom()}, r1.lowerright() };
		}
		return return_arr;
	}

	// the rectangle has been eaten o_o https://www.desmos.com/geometry/ziteq1xk2a
	if (num_corner_inside == 4) { 
		return_arr[0] = { r1.upperleft(), {r2.x, r1.bottom()} };
		return_arr[1] = { {r2.x, r1.y}, r2.upperright() };
		return_arr[2] = { r2.lowerleft(), {r2.right(), r1.bottom() } };
		return_arr[3] = { {r2.right(), r1.y}, r1.lowerright() };
		return return_arr;
	}

	// if the rectangle intersects but none of the corners are inside 
	auto overlapped_rec = r1.calculate_overlap(r2); 
	// Now we need to check which points are inside r2
	// upperleft and upperright
	if (r2.intersects(r1.upperleft()) and r2.intersects(r1.upperright())) {
		return_arr[0] = { overlapped_rec.lowerleft(), r1.lowerright() };
	}
	// upperleft and lowerleft
	else if (r2.intersects(r1.upperleft()) and r2.intersects(r1.lowerleft())) { 
		return_arr[0] = { overlapped_rec.upperright(), r1.lowerright() };
	}
	// lowerleft and lowerright
	else if (r2.intersects(r1.lowerleft()) and r2.intersects(r1.lowerright())) {
		return_arr[0] = { r1.upperleft(), overlapped_rec.upperright() };
	}
	// upperright and lowerright
	else if (r2.intersects(r1.upperright()) and r2.intersects(r1.lowerright())) {
		return_arr[0] = { r1.upperleft(), overlapped_rec.lowerleft() };
	}
	return return_arr;
}

void callback_draw(std::optional<std::deque<PDFHandler::CachedBitmap>*> highres_bitmaps, std::mutex* lock) {
	// draw scaled elements
	g_main_renderer->set_current_transform_active();
	g_main_renderer->begin_draw();
	g_main_renderer->clear(Renderer::Color(50, 50, 50));

	//// when the highres_bitmaps arg is nullopt then it is a normal draw call from windows
	//if (highres_bitmaps.has_value() == false)
	//	g_pdfhandler->render(g_main_window->get_hwnd());
	////else there are new bitmaps to render that have been created by other threads
	//else {
	//	auto* bitmaps = highres_bitmaps.value();
	//	for (auto& i : *bitmaps) {
	//		g_main_renderer->draw_bitmap(i.bitmap, i.dest, Direct2DRenderer::INTERPOLATION_MODE::LINEAR);
	//	}
	//}
	 
	// draw the rectangles
	auto arr = splice_rect(r1, r2);
	for (auto& i : arr) {
		if (i.has_value())
			g_main_renderer->draw_rect(i.value(), *color_cyan, 5);
	}
	g_main_renderer->draw_rect(r1, *color_red);
	g_main_renderer->draw_rect(r2, *color_yellow);
	

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
	switch (index) {
	case 0:
		r1.x = p.pos.x;
		r1.y = p.pos.y;
		break;
	case 1:
		r1.width = p.pos.x - r1.x;
		r1.height = p.pos.y - r1.y;
		break;
	case 2:
		r2.x = p.pos.x;
		r2.y = p.pos.y;
		break;
	case 3:
		r2.width = p.pos.x - r2.x;
		r2.height = p.pos.y - r2.y;
		index = -1;
		break;
	}
	index++;
	r1.validate();
	r2.validate();
	g_main_window->invalidate_drawing_area();
}

void callback_mousewheel(short delta, bool hwheel, Renderer::Point<int> center) {
	delta *= 1/g_main_renderer->get_transform_scale();
	if (WindowHandler::is_key_pressed(WindowHandler::LEFT_CONTROL)) {
		g_main_renderer->add_scale_matrix(delta > 0 ? 1.25 : 0.8, center);
		Logger::log(g_main_renderer->get_transform_scale());
		Logger::print_to_debug();
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

	//auto path = FileHandler::open_file_dialog(L"PDF\0*.pdf\0\0", *g_main_window);
	//if (path.has_value()) {
	//	pdf_handler = PDFHandler(g_main_renderer.get(), *g_mupdfcontext, path.value());
	//	g_pdfhandler = &pdf_handler; 
	//}
	//else {
	//	return;
	//}

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