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
std::vector<MuPDFHandler::PDF> g_template_pdfs;
PDFRenderHandler* g_pdfrenderhandler = nullptr;
StrokeHandler* g_strokehandler = nullptr;

GestureHandler g_gesturehandler;

bool g_draw_annots = true;

UINT toolbar_last_draw = 0;

void crash_callback() {
	auto path = FileHandler::get_appdata_path() / L"crash" / SessionVariables::FILE_PATH.filename(); 
	Logger::log("Trying to save PDF at ", path);
	

	if (g_pdf != nullptr) {
		g_pdf->save_pdf(path);
		Logger::success("Saved PDF"); 
	}
	else {
		Logger::error("Could not save PDF");
	}

	// create message box asking to go to the log folder
	auto msg = std::wstring(L"An fatal error occured which prevents further program execution. A save of the PDF has been made. Open log folder?");
	auto res = MessageBox(NULL, msg.c_str(), L"Error", MB_YESNO | MB_ICONERROR);
	if (res == IDYES) {
		ShellExecute(NULL, L"open", path.parent_path().c_str(), NULL, NULL, SW_SHOWDEFAULT);
	}

}

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
		g_main_renderer->clear(AppVariables::COLOR_BACKGROUND); 
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
		g_pdfrenderhandler->send_bitmaps(instructions); 
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
			}
	
		}
		g_main_renderer->end_draw();
		return;
	}
	case WindowHandler::PDF_BITMAP_READY:
	{
		g_pdfrenderhandler->send_bitmaps(instructions);
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

	if (g_template_pdfs.empty() == false) {
		g_ui_handler->draw_new_page_button(); 
	}
	g_ui_handler->draw_pen_selection();
	g_ui_handler->draw_caption(toolbar_last_draw);

	g_main_renderer->end_draw();
}

void save_dialoge() {
	auto path = FileHandler::save_file_dialog(L"PDF\0*.pdf\0\0", *g_main_window);
	if (path == std::nullopt)
		return;
	if (std::filesystem::path(path.value()).extension() != ".pdf") {
		path.value().append(L".pdf");
	}
	
	SessionVariables::FILE_PATH = path.value();
	SessionVariables::WINDOW_TITLE = SessionVariables::FILE_PATH.filename();

	g_pdf->save_pdf(path.value());
	Logger::success("Saved PDF to ", path.value());
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
			if (SessionVariables::FILE_PATH.empty()) {
				SessionVariables::WINDOW_TITLE = L"New PDF";
				g_main_window->set_window_title(L"New PDF");
			}
			else {
				g_main_window->set_window_title(SessionVariables::FILE_PATH.filename());
				SessionVariables::WINDOW_TITLE = SessionVariables::FILE_PATH.filename();
			}

		}
		else {
			g_main_window->set_window_title(APPLICATION_NAME + std::wstring(L" | Parsing pages: ") + std::to_wstring(progress * 100) + L" % ");
			SessionVariables::WINDOW_TITLE = getLoadingBar(progress) + std::format( + L" {:.1f} %", progress * 100);
			
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
	case PDF_HANDLER_PAGE_CHANGE:
	{
		g_pdfrenderhandler->update_displaylist(static_cast<MuPDFHandler::DisplaylistChangeInfo*>(data));
		return;
	}
	}
}

void callback_size(Math::Rectangle<long> r) {
	g_main_renderer->resize(r);
	g_main_window->invalidate_drawing_area();
	g_gesturehandler.check_bounds();
}

void callback_key_down(WindowHandler::VK key) {
	using VK = WindowHandler::VK;
	switch (key) {
	case VK::S:
	{
		if (not WindowHandler::is_key_pressed(VK::LEFT_CONTROL)) { 
			break;
		}
		if (WindowHandler::is_key_pressed(VK::LEFT_SHIFT) or SessionVariables::FILE_PATH.empty()) {
			save_dialoge();
		}
		else {
			g_pdf->save_pdf(SessionVariables::FILE_PATH);
			Logger::success("Saved PDF to ", SessionVariables::FILE_PATH);
		}
		break;
	}
	case VK::OEM_PLUS:
	{
		auto mouse_pos = g_main_window->get_mouse_pos();
		g_gesturehandler.update_page_pos(true, true, mouse_pos);

		g_main_window->invalidate_drawing_area();
		break;
	}
	case VK::OEM_MINUS:
	{
		auto mouse_pos = g_main_window->get_mouse_pos();
		g_gesturehandler.update_page_pos(false, true, mouse_pos);

		g_main_window->invalidate_drawing_area();
		break;
	}
	case VK::UPARROW:
	{
		g_gesturehandler.update_page_pos(true, false);
		g_main_window->invalidate_drawing_area();
		break;
	}
	case VK::DOWNARROW:
	{
		g_gesturehandler.update_page_pos(false, false);
		g_main_window->invalidate_drawing_area();
		break;
	}
	case VK::LEFTARROW:
	{
		g_gesturehandler.update_page_pos(true, true);
		g_main_window->invalidate_drawing_area();
		break;
	}
	case VK::RIGHTARROW:
	{
		g_gesturehandler.update_page_pos(false, true);
		g_main_window->invalidate_drawing_area();
		break;
	}
	case VK::F1:
	{
		// opens the roaming folder
		Logger::log("Opening appdata folder");
		ShellExecute(NULL, L"open", L"explorer.exe", FileHandler::get_appdata_path().c_str(), NULL, SW_SHOWDEFAULT);
		break;
	}
	case VK::F3:
	{
		g_draw_annots = not g_draw_annots;
		g_main_window->invalidate_drawing_area();
		break;
	}
	case VK::F4:
	{
		if (not WindowHandler::is_key_pressed(VK::ALT)) {
			break;
		} 
		[[fallthrough]]; 
	}
	case VK::ESCAPE:
	{
		g_main_window->send_close_request();
		break;
	}
	case VK::F5:
	{
		g_pdfrenderhandler->clear_render_cache();
		g_main_window->invalidate_drawing_area();
		break;
	}
	case VK::NUMPAD_0:
	case VK::KEY_0:
	{
		SessionVariables::PENSELECTION_SELECTED_PEN = ~0;
		g_main_window->invalidate_drawing_area();
		break;
	}
	case VK::KEY_1:
	case VK::KEY_2:
	case VK::KEY_3:
	case VK::KEY_4:
	case VK::KEY_5:
	case VK::KEY_6:
	case VK::KEY_7:
	case VK::KEY_8:
	case VK::KEY_9:
	{
		auto amount_of_pens = g_strokehandler->get_pen_handler().get_all_pens().size();
		SessionVariables::PENSELECTION_SELECTED_PEN = (key - VK::KEY_1) % amount_of_pens;
		g_main_window->invalidate_drawing_area();
		break;
	}
	case VK::NUMPAD_1:
	case VK::NUMPAD_2:
	case VK::NUMPAD_3:
	case VK::NUMPAD_4:
	case VK::NUMPAD_5:
	case VK::NUMPAD_6:
	case VK::NUMPAD_7:
	case VK::NUMPAD_8:
	case VK::NUMPAD_9:
	{
		auto amount_of_pens = g_strokehandler->get_pen_handler().get_all_pens().size();
		SessionVariables::PENSELECTION_SELECTED_PEN = (key - VK::NUMPAD_1) % amount_of_pens;
		g_main_window->invalidate_drawing_area();
		break;
	}
	}
}

void callback_key_up(WindowHandler::VK k) {
	// its now all in key down
}

void callback_pointer_down(WindowHandler::PointerInfo p) {
	//if (p.type == WindowHandler::POINTER_TYPE::MOUSE and p.button3pressed) {
	//	g_gesturehandler.start_select_page(p);
	//}
	using UI_HIT = UIHandler::UI_HIT;
	auto ui_hit = g_ui_handler->check_ui_hit(p.pos);
	switch (ui_hit.type) {
	case UI_HIT::PEN_SELECTION:
	{
		SessionVariables::PENSELECTION_SELECTED_PEN = ui_hit.data; 
		g_main_window->invalidate_drawing_area(); 
		return;
	}
	case UI_HIT::SAVE_BUTTON:
	{
		if ((p.type == WindowHandler::POINTER_TYPE::MOUSE  and p.button2pressed)
		 or (p.type == WindowHandler::POINTER_TYPE::STYLUS and p.button1pressed)
		 or  SessionVariables::FILE_PATH.empty()) {
			save_dialoge();
		}
		else {
			g_pdf->save_pdf(SessionVariables::FILE_PATH);
		}
		return;
	}

	default:
		break;
	}
	

	// handle hand
	if (p.type == WindowHandler::POINTER_TYPE::TOUCH) {
		g_gesturehandler.start_gesture(p);
	}

	// handle everything related to the pen
	if (p.type == WindowHandler::POINTER_TYPE::STYLUS and p.pressure > 0) {
		
		if (p.button2pressed or SessionVariables::PENSELECTION_SELECTED_PEN == ~0) {
			g_strokehandler->earsing_stroke(p);
		} 
		else {
			g_strokehandler->start_stroke(p);
		}
	}

	g_main_window->invalidate_drawing_area();
}

void callback_pointer_update(WindowHandler::PointerInfo p) {
	if (g_ui_handler->check_ui_hit(p.pos).type != UIHandler::UI_HIT::NONE
		and (g_strokehandler->is_stroke_active() == false and g_gesturehandler.is_gesture_active() == false)) {
		return;
	}

	if (p.type == WindowHandler::POINTER_TYPE::TOUCH) {
		g_gesturehandler.update_gesture(p);
		g_main_window->invalidate_drawing_area();
	}

	// handle everything related to the pen
	if (p.type == WindowHandler::POINTER_TYPE::STYLUS and p.pressure > 0) {
		if (p.button2pressed or SessionVariables::PENSELECTION_SELECTED_PEN == ~0) {
			g_strokehandler->earsing_stroke(p);
		} 
		else {
			g_strokehandler->update_stroke(p);
		}

		g_main_window->invalidate_drawing_area();
	}
}

void callback_pointer_up(WindowHandler::PointerInfo p) {
	using UI_HIT = UIHandler::UI_HIT;
	auto ui_hit = g_ui_handler->check_ui_hit(p.pos);
	switch (ui_hit.type) {
	case UI_HIT::NEW_PAGE:
	{
		if (g_gesturehandler.is_moving() or g_template_pdfs.empty()) {
			break;
		}
		g_pdf->add_page(g_template_pdfs[0]); 
		g_pdfrenderhandler->clear_render_cache(); 
		g_main_window->invalidate_drawing_area(); 
	}
	}

	if (p.type == WindowHandler::POINTER_TYPE::TOUCH) {
		g_gesturehandler.end_gesture(p);
	}

	// handle everything related to the pen
	if (p.type == WindowHandler::POINTER_TYPE::STYLUS) {
		if (p.button2pressed or SessionVariables::PENSELECTION_SELECTED_PEN == ~0) {
			g_strokehandler->end_earsing_stroke(p);
		}
		else {
			g_strokehandler->end_stroke(p);
		}
	}

	g_main_window->invalidate_drawing_area();
}

void callback_mousewheel(short delta, bool hwheel, Math::Point<int> center) {
	std::optional<Math::Point<int>> zoom_center = std::nullopt;
	if (WindowHandler::is_key_pressed(WindowHandler::VK::LEFT_CONTROL)) {
		zoom_center = center;
	}

	g_gesturehandler.update_page_pos(delta > 0, hwheel or WindowHandler::is_key_pressed(WindowHandler::VK::LEFT_SHIFT), zoom_center);
	g_main_window->invalidate_drawing_area();
}

std::filesystem::path load_template_pdfs() {
	auto path = FileHandler::get_appdata_path() / AppVariables::PDF_TEMPLATE_FOLDER_NAME;
	std::filesystem::path first_file;

	if (std::filesystem::exists(path) == false) {
		Logger::log("No template folder found. Creating one...");
		std::filesystem::create_directory(path);
		return first_file;
	}

    for (const auto& entry : std::filesystem::directory_iterator(path)) { 
		if (entry.is_regular_file() == false or entry.path().extension() != ".pdf") {
			continue;
		}
		first_file = entry.path(); 
        // Load the PDF file here
        std::wstring pdf_path = entry.path().wstring();
        // Call the function to load the PDF file
		auto pdf = g_mupdfcontext->load_pdf(pdf_path);
		if (pdf.has_value() == false) {
			continue;
		}

		g_template_pdfs.push_back(std::move(pdf.value())); 
    }

	return first_file;
}

void main_window_loop_run(HINSTANCE h, std::filesystem::path p) {
	// add callback to possible crash
	ABNORMAL_PROGRAM_EXIT_CALLBACK = crash_callback;

	auto update_caption = [](const std::wstring& input) {
		SessionVariables::WINDOW_TITLE = input;
		g_main_renderer->begin_draw();
		g_main_renderer->clear(AppVariables::COLOR_BACKGROUND);
		g_ui_handler->draw_caption(0);
		g_main_renderer->end_draw();
		};


	Logger::log("Initializing main window loop");
	g_main_window = std::make_unique<WindowHandler>(APPLICATION_NAME, h);
	g_main_window->set_state(WindowHandler::WINDOW_STATE::NORMAL);

	g_main_renderer = std::make_unique<Direct2DRenderer>(*g_main_window.get());
	g_ui_handler = std::make_unique<UIHandler>(g_main_renderer.get()); 

	g_mupdfcontext = std::shared_ptr<MuPDFHandler>(new MuPDFHandler); 

	auto alternate_path = load_template_pdfs(); 

	if (p.empty()) {
	INVALID_PATH:
		update_caption(L"Choosing .pdf");
		auto op_path = FileHandler::open_file_dialog(L"PDF\0*.pdf\0\0", *g_main_window); 
		if (op_path.has_value()) {
			p = op_path.value();

			SessionVariables::FILE_PATH = p;
		}
		else if (!alternate_path.empty()) {
			p = alternate_path; 
		}
		else {
			return;
		}
	}
	else {
		SessionVariables::FILE_PATH = p;
	}

	Logger::log(L"Trying to open ", p);
	update_caption(L"Opening " + p.filename().wstring());
	Timer time;
	auto pdf = g_mupdfcontext->load_pdf(p);
	if (!pdf.has_value()) {
		goto INVALID_PATH;
	}

	g_pdf = &pdf.value(); 
	{
		// center the pdf in the middle
		auto rec = g_pdf->get_pagerec()->get_read();
		auto page = rec->at(0);
		g_main_renderer->add_transform_matrix({ 
			(static_cast<float>(g_main_renderer->get_window_size_normalized().width) - page.width) / 2.0f,
			AppVariables::WINDOWLAYOUT_TOOLBAR_HEIGHT
			});
	}

	g_gesturehandler = GestureHandler(g_main_renderer.get(), &pdf.value());
	auto strok_handler = StrokeHandler(g_pdf, g_main_renderer.get(), g_main_window.get());
	g_strokehandler = &strok_handler;

	g_ui_handler->add_penhandler(&strok_handler.get_pen_handler()); 
	g_ui_handler->add_pdf(g_pdf);  

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

	WINDOW_LOOP:
	while(!g_main_window->close_request()) {
		g_main_window->get_window_messages(true);
	}

	if (SessionVariables::PDF_UNSAVED_CHANGES) {
		auto ans = MessageBox(NULL, L"Unsaved changes. Do you want to quit?", L"Unsaved changes", MB_ICONWARNING | MB_YESNO); 
		if (ans == IDNO) { 
			g_main_window->rest_close_request() ;
			goto WINDOW_LOOP;
		}
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