#include "helper/include.h"
#include <filesystem>

std::unique_ptr<WindowHandler> g_main_window;
std::unique_ptr<Direct2DRenderer> g_main_renderer;
std::unique_ptr<UIHandler> g_ui_handler;

// will be automatically destroyed after main_window_loop_run
Direct2DRenderer::TextFormatObject* g_text_format;
Direct2DRenderer::BrushObject* g_brush;

MuPDFHandler::PDF* g_pdf;
std::shared_ptr<MuPDFHandler> g_mupdfcontext;
PDFRenderHandler* g_pdfrenderhandler = nullptr;
StrokeHandler* g_strokehandler = nullptr;

GestureHandler g_gesturehandler;

bool g_draw_annots = true;

UINT toolbar_last_draw = 0;
std::wstring window_title_msg = L"";
std::wstring file_name;

void callback_draw(WindowHandler::DRAW_EVENT event, void* data) {
	// draw scaled elements
	g_main_renderer->set_current_transform_active();
	g_main_renderer->begin_draw();

	PDFRenderHandler::RenderInstructions instructions;
	instructions.draw_annots = g_draw_annots;
	instructions.render_annots = g_draw_annots;

	switch (event) {
	case WindowHandler::NORMAL_DRAW:
	{
		g_main_renderer->clear(Renderer::Color(50, 50, 50));
		// when the highres_bitmaps arg is nullopt then it is a normal draw call from windows
		// but only call when no gesture is in progress
		if (g_gesturehandler.is_gesture_active() == false) {
			g_pdfrenderhandler->render(instructions); 
		}
		else {
			instructions.render_highres = false; 
			instructions.render_annots = false; 
			g_pdfrenderhandler->render(instructions); 
		}
		break;
	}
	case WindowHandler::PDF_BITMAP_READ:
	{
		//else there are new bitmaps to render that have been created by other threads
		auto bitmaps = reinterpret_cast<std::vector<CachedBitmap*>*>(data); 
		for (size_t i = 0; i < bitmaps->size(); i++) {
			auto rec = g_pdf->get_pagerec()->get_read(); 
			auto pagerec = bitmaps->at(i)->doc_coords + rec->at(bitmaps->at(i)->page).upperleft();
			if (g_main_renderer->inv_transform_rect(g_main_renderer->get_window_size()).intersects(pagerec)) {
				g_main_renderer->draw_bitmap(bitmaps->at(i)->bitmap, pagerec, Direct2DRenderer::INTERPOLATION_MODE::LINEAR);
				//g_main_renderer->draw_rect(pagerec, { static_cast<byte>(i), static_cast<byte>(i + 40), static_cast<byte>(i + 80)}, 2);
			}
	
		}
		break;
	}
	case WindowHandler::TOOLBAR_DRAW:
		toolbar_last_draw = reinterpret_cast<UINT>(data);
		break;
	}
	g_strokehandler->render_strokes();
	// selected page
	g_main_renderer->set_current_transform_active();
	auto selected_page = g_gesturehandler.get_selected_page();
	if (selected_page != std::nullopt) {
		auto rec = g_pdf->get_pagerec()->get_read();
		g_main_renderer->draw_rect(rec->at(selected_page.value()), { 0, 0, 255 }, 5);
	}

	// draw ui elements
	g_main_renderer->set_identity_transform_active();

	g_ui_handler->draw_pen_selection(g_strokehandler->get_pen_handler().get_all_pens(), 
		g_main_window->get_toolbar_margin() + 10, g_strokehandler->get_pen_handler().get_pen_index(), 35);
	g_ui_handler->draw_caption(window_title_msg, g_main_window->get_toolbar_margin(), toolbar_last_draw); 
	
	g_main_renderer->end_draw();
}

std::wstring getLoadingBar(float progress) {
	int barWidth = 10; // size of the bar
	int pos = progress * barWidth;

	std::wstring bar = L"[";
	for (int i = 0; i < barWidth; ++i) {
		if (i < pos) bar += L"#";
		else bar += L" ";
	}
	bar += L"]";

	return bar;
}

void custom_msg(CUSTOM_WM_MESSAGE msg, void* data) {
	switch (msg) {
	case PDF_HANDLER_DISPLAY_LIST_UPDATE:
	{
		auto progress = g_pdfrenderhandler->get_display_list_progress();
		if (FLOAT_EQUAL(progress, 1)) {
			g_main_window->set_window_title(file_name);
			window_title_msg = file_name;

		}
		else {
			g_main_window->set_window_title(APPLICATION_NAME + std::wstring(L" | Parsing pages: ") + std::to_wstring(progress * 100) + L" % ");
			window_title_msg = getLoadingBar(progress) + std::format( + L" {:.1f} %", progress * 100);
			
			g_main_window->invalidate_drawing_area();
		}
		return;
	}
	case PDF_HANDLER_ANNOTAION_CHANGE:
	{
		// The data is in this case a size_t* 
		g_pdfrenderhandler->update_annotations(*static_cast<size_t*>(data));
		g_main_window->invalidate_drawing_area();
		return;
	}
	}
}

void callback_size(Math::Rectangle<long> r) {
	g_main_renderer->resize(r);
	g_gesturehandler.check_bounds();
}

void callback_key_down(WindowHandler::VK key) {
	if (WindowHandler::is_key_pressed(WindowHandler::LEFT_CONTROL)) {
		if (key == WindowHandler::VK::S) {
			auto path = FileHandler::save_file_dialog(L"PDF\0*.pdf\0\0", *g_main_window);
			if (path == std::nullopt)
				return;
			if (std::filesystem::path(path.value()).extension() != ".pdf") {
				path.value().append(L".pdf");
			}
			g_pdf->save_pdf(path.value()); 
			Logger::success("Saved PDF to ", path.value());
		}
	}
}

void callback_key_up(WindowHandler::VK k) {
	if (WindowHandler::is_key_pressed(WindowHandler::LEFT_CONTROL)) {
		auto mouse_pos = g_main_window->get_mouse_pos();
		if (k == WindowHandler::VK::OEM_PLUS ) {
			g_main_renderer->add_scale_matrix(1.25, mouse_pos);
		}
		else if (k == WindowHandler::VK::OEM_MINUS) {
			g_main_renderer->add_scale_matrix(0.8f, mouse_pos);

		}
	}

	if (k == WindowHandler::VK::F5) { 
		// refresh
		g_pdfrenderhandler->clear_render_cache();
		g_pdfrenderhandler->update_annotations(0);
	}

	if (k == WindowHandler::VK::F3) {
		g_draw_annots = not g_draw_annots;
	}

	if (k == WindowHandler::VK::F2) { 
		g_strokehandler->get_pen_handler().select_next_pen(); 
	}

	if (k == WindowHandler::VK::F1) {
		// opens the roaming folder
		ShellExecute(NULL, L"open", L"explorer.exe", FileHandler::get_appdata_path().c_str(), NULL, SW_SHOWDEFAULT);
	}

	g_main_window->invalidate_drawing_area();
}

Math::Point<float> g_init_touch_pos = { 0, 0 };
Math::Point<float> g_init_trans_pos = { 0, 0 };

void callback_pointer_down(WindowHandler::PointerInfo p) {
	if (p.type == WindowHandler::POINTER_TYPE::MOUSE and p.button3pressed) {
		g_gesturehandler.start_select_page(p);
	}
	// handle hand
	if (p.type == WindowHandler::POINTER_TYPE::TOUCH) {
		g_gesturehandler.start_gesture(p);
	}
	// handle everything related to the pen
	if (p.type == WindowHandler::POINTER_TYPE::STYLUS and p.pressure > 0) {
		if (p.button2pressed) {
			g_strokehandler->earsing_stroke(p);
		} 
		else {
			g_strokehandler->start_stroke(p);
		}
	}

	g_main_window->invalidate_drawing_area();
}

void callback_pointer_update(WindowHandler::PointerInfo p) {
	if (p.type == WindowHandler::POINTER_TYPE::MOUSE and p.button3pressed) {
		g_gesturehandler.update_select_page(p);
		g_main_window->invalidate_drawing_area();
	}

	if (p.type == WindowHandler::POINTER_TYPE::TOUCH) {
		g_gesturehandler.update_gesture(p);
		g_main_window->invalidate_drawing_area();
	}

	// handle everything related to the pen
	if (p.type == WindowHandler::POINTER_TYPE::STYLUS and p.pressure > 0) {
		if (p.button2pressed) {
			g_strokehandler->earsing_stroke(p);
		} 
		else {
			g_strokehandler->update_stroke(p);
		}

		g_main_window->invalidate_drawing_area();
	}

}


void callback_pointer_up(WindowHandler::PointerInfo p) {
	if (p.type == WindowHandler::POINTER_TYPE::MOUSE and not p.button3pressed) { 
		g_gesturehandler.stop_select_page(p); 
	}

	if (p.type == WindowHandler::POINTER_TYPE::TOUCH) {
		g_gesturehandler.end_gesture(p);
	}

	// handle everything related to the pen
	if (p.type == WindowHandler::POINTER_TYPE::STYLUS) {
		if (p.button2pressed) {
			g_strokehandler->end_earsing_stroke(p);
		}
		else {
			g_strokehandler->end_stroke(p);
		}
	}

	g_main_window->invalidate_drawing_area();
}

void callback_mousewheel(short delta, bool hwheel, Math::Point<int> center) {
	g_gesturehandler.update_mouse(static_cast<float>(delta)/2.0f, hwheel, center);
	g_main_window->invalidate_drawing_area();
}

void main_window_loop_run(HINSTANCE h, std::filesystem::path p) {
	auto update_caption = [](const std::wstring& input) {
		window_title_msg = input;
		g_main_renderer->begin_draw();
		g_main_renderer->clear(Renderer::Color(50, 50, 50));
		g_ui_handler->draw_caption(window_title_msg, g_main_window->get_toolbar_margin(), 0);
		g_main_renderer->end_draw();
		};


	Logger::log("Initializing main window loop");
	g_main_window = std::make_unique<WindowHandler>(APPLICATION_NAME, h);
	g_main_window->set_state(WindowHandler::WINDOW_STATE::NORMAL);

	g_main_renderer = std::make_unique<Direct2DRenderer>(*g_main_window.get());
	g_ui_handler = std::make_unique<UIHandler>(g_main_renderer.get()); 

	auto default_brush = g_main_renderer->create_brush(Renderer::Color(255, 255, 255));
	g_brush = &default_brush;
	auto default_text_format = g_main_renderer->create_text_format(L"Courier New", g_main_window->get_toolbar_margin() * 0.95);
	g_text_format = &default_text_format;
	g_mupdfcontext = std::shared_ptr<MuPDFHandler>(new MuPDFHandler); 

	if (p.empty()) {
	INVALID_PATH:
		update_caption(L"Choosing .pdf");
		auto op_path = FileHandler::open_file_dialog(L"PDF\0*.pdf\0\0", *g_main_window); 
		if (!op_path.has_value()) {
			return;
		}
		p = op_path.value();
	}
	Logger::log(L"Trying to open ", p);
	update_caption(L"Opening " + p.filename().wstring());
	Timer time;
	auto pdf = g_mupdfcontext->load_pdf(p);
	if (!pdf.has_value()) {
		goto INVALID_PATH;
	}
	file_name = p.filename();

	g_pdf = &pdf.value(); 
	{
		// center the pdf in the middle
		auto rec = g_pdf->get_pagerec()->get_read();
		auto page = rec->at(0);
		g_main_renderer->add_transform_matrix({ (static_cast<float>(g_main_renderer->get_window_size_normalized().width) - page.width) / 2.0f, static_cast<float>(g_main_window->get_toolbar_margin())});
	}

	g_gesturehandler = GestureHandler(g_main_renderer.get(), &pdf.value());
	auto strok_handler = StrokeHandler(g_pdf, g_main_renderer.get(), g_main_window.get());
	g_strokehandler = &strok_handler;

	auto pdf_handler = PDFRenderHandler(g_pdf, g_main_renderer.get(), g_main_window.get(), 2);
	g_pdfrenderhandler = &pdf_handler;

	Logger::success("Loaded PDF file in ", time);

	// why do i have to do this here??
	g_main_renderer->resize(g_main_window->get_client_size());

		
	// do the callbacks
	g_main_window->set_callback_paint(callback_draw);
	g_main_window->set_callback_size(callback_size);
	g_main_window->set_callback_custom_msg(custom_msg); 

	g_main_window->set_callback_pointer_down(callback_pointer_down);
	g_main_window->set_callback_pointer_update(callback_pointer_update);
	g_main_window->set_callback_pointer_up(callback_pointer_up);

	g_main_window->set_callback_mousewheel(callback_mousewheel);
	g_main_window->set_callback_key_down(callback_key_down);
	g_main_window->set_callback_key_up(callback_key_up);

	while(!g_main_window->close_request()) {
		g_main_window->get_window_messages(true);
	}
	Logger::log(L"Got close request. Cleaning up...");
	// this is creating a 1 byte memory leak
	std::thread t([] {
		std::this_thread::sleep_for(std::chrono::seconds(2));
		// abort if normal cleanup is not possible
		Logger::error("Could not cleanup in time. Aborting...");
		abort();
	});
	t.detach();

	//pdf_handler.stop_rendering(*g_main_window);
}