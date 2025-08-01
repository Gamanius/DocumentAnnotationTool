#include "MainWindowHandler.h"

#include "ui/UIDebug.h"
#include "ui/UIToolbar.h"

static std::optional<std::wstring> open_file_dialog(const wchar_t* filter, HWND windowhandle = nullptr) {
	OPENFILENAME ofn;
	WCHAR szFile[MAX_PATH] = { 0 };

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = windowhandle; // If you have a window handle, specify it here.
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ENABLESIZING;

	if (GetOpenFileName(&ofn)) {
		return std::wstring(ofn.lpstrFile);
	}
	auto error = CommDlgExtendedError();
	WIN_ASSERT_OK(error, "Unknown file dialog error with comm error ", error);
	return std::nullopt;
}

void DocantoWin::MainWindowHandler::paint() {
	m_ctx->tabs->get_active_tab()->pdfhandler->request();

	m_ctx->render->begin_draw();
	m_ctx->render->clear();

	m_ctx->render->set_current_transform_active();
	m_ctx->tabs->get_active_tab()->pdfhandler->draw();

	m_ctx->render->set_identity_transform_active();
	m_ctx->uihandler->draw();
	m_ctx->caption->draw();
	m_ctx->render->end_draw();
}
void DocantoWin::MainWindowHandler::size(Docanto::Geometry::Dimension<long> d) {
	m_ctx->render->resize(d);
}

void DocantoWin::MainWindowHandler::key(Window::VK key, bool pressed) {
	if (pressed == false) {
		return;
	}

	using enum Window::VK;

	switch (key) {
	case F4:
	{
		if (m_ctx->window->is_key_pressed(ALT)) {
			m_ctx->window->send_close_request();
		}
		break;
	}
	case F10:
	{
		m_ctx->tabs->get_active_tab()->pdfhandler->toggle_debug_draw();
		m_ctx->window->send_paint_request();
		break;
	}
	case O:
	{
		auto path = open_file_dialog(L"PDF\0 * .pdf\0\0", m_ctx->window->get_hwnd());
		Docanto::Logger::log("Got path ", path);
		if (path.has_value()) {
			Docanto::Timer t;
			m_ctx->tabs->get_active_tab()->pdfhandler->add_pdf(path.value());
			Docanto::Logger::log("Loaded PDF in ", t);
		}
		break;
	}
	case P:
	{
		auto path = open_file_dialog(L"PDF\0 * .pdf\0\0", m_ctx->window->get_hwnd());
		Docanto::Logger::log("Got path ", path);
		if (path.has_value()) {
			Docanto::Timer t;
			auto pdf = std::make_shared<PDFHandler>(path.value(), m_ctx->render);
			auto tool = std::make_shared<ToolHandler>(pdf);

			m_ctx->tabs->add(std::make_shared<TabContext>(pdf, tool));
			Docanto::Logger::log("Loaded PDF in ", t);
		}
		break;
	}
	case F1:
	{
		auto& last = m_ctx->uihandler->get_all_uiobjects_ref().back();
		last->set_resizable(!last->is_resizable());
	}
	case KEY_1:
	{
		m_ctx->tabs->set_active_tab(0);
		m_ctx->window->send_paint_request();
		break;
	}
	case KEY_2:
	{
		m_ctx->tabs->set_active_tab(1);
		m_ctx->window->send_paint_request();
		break;
	}
	case KEY_3:
	{
		m_ctx->tabs->set_active_tab(2);
		m_ctx->window->send_paint_request();
		break;
	}
	case R: 
	{
		AppVariables::TOUCH_ALLOW_ROTATION = !AppVariables::TOUCH_ALLOW_ROTATION;
		m_ctx->render->set_rotation_matrix(0, { 0, 0 });
		m_ctx->window->send_paint_request();
		break;
	}
	case E:
	{
		m_ctx->render->add_rotation_matrix(10, m_ctx->window->get_mouse_pos());
		m_ctx->window->send_paint_request();
		break;
	}
	case Q:
	{
		m_ctx->render->add_rotation_matrix(-10, m_ctx->window->get_mouse_pos());
		m_ctx->window->send_paint_request();
		break;
	}
	case DOWNARROW:
	{
		m_ctx->render->add_translation_matrix({ 0, -100 });
		m_ctx->window->send_paint_request();
		break;
	}
	case UPARROW:
	{
		m_ctx->render->add_translation_matrix({ 0, 100 });
		m_ctx->window->send_paint_request();
		break;
	}
	case LEFTARROW:
	{
		m_ctx->render->add_translation_matrix({ 100, 0 });
		m_ctx->window->send_paint_request();
		break;
	}
	case RIGHTARROW:
	{
		m_ctx->render->add_translation_matrix({ -100, 0 });
		m_ctx->window->send_paint_request();
		break;
	}
	case OEM_PLUS:
	{
		m_ctx->render->add_scale_matrix(1.05, m_ctx->window->get_mouse_pos());
		m_ctx->window->send_paint_request();
		break;
	}
	case OEM_MINUS:
	{
		m_ctx->render->add_scale_matrix(0.95, m_ctx->window->get_mouse_pos());
		m_ctx->window->send_paint_request();
		break;
	}
	case BACKSPACE:
	{
		m_ctx->render->set_rotation_matrix(D2D1::Matrix3x2F::Identity());
		m_ctx->render->set_scale_matrix(D2D1::Matrix3x2F::Identity());
		m_ctx->render->set_translation_matrix(D2D1::Matrix3x2F::Identity());
		[[fallthrough]];
	}
	case SPACE:
	{
		m_ctx->window->send_paint_request();
	}

	}
}

void DocantoWin::MainWindowHandler::pointer_down(Window::PointerInfo p) {
	if (!m_gesture) {
		return;
	}

	if (m_ctx->uihandler->pointer_down(p.pos)) {
		m_ctx->window->send_paint_request();
		return;
	}

	auto tool_hand_is_on = m_ctx->tabs->get_active_tab()->toolhandler->get_current_tool().type == ToolHandler::ToolType::HAND_MOVEMENT;
	if (tool_hand_is_on) {
		m_ctx->window->set_global_cursor(Window::CURSOR_TYPE::HAND_GRABBING);
	}

	// there will never be a touchpad pointer down event since we cant track them
	if (p.type == Window::POINTER_TYPE::TOUCH or p.type == Window::POINTER_TYPE::TOUCHPAD or tool_hand_is_on) {
		m_gesture->start_gesture(p);
	}

	m_ctx->window->send_paint_request();
}

void DocantoWin::MainWindowHandler::pointer_update(Window::PointerInfo p) {
	if (!m_gesture) {
		return;
	}

	if (m_ctx->uihandler->pointer_update(p.pos)) {
		m_ctx->window->send_paint_request();
		return;
	}

	if (p.type == Window::POINTER_TYPE::TOUCH or p.type == Window::POINTER_TYPE::TOUCHPAD or
		m_ctx->tabs->get_active_tab()->toolhandler->get_current_tool().type == ToolHandler::ToolType::HAND_MOVEMENT) {
		m_gesture->update_gesture(p);
		m_ctx->window->send_paint_request();
	}


	m_ctx->window->send_paint_request();
}

void DocantoWin::MainWindowHandler::pointer_up(Window::PointerInfo p) {
	if (!m_gesture) {
		return;
	}

	if (m_ctx->uihandler->pointer_up(p.pos)) {
		m_ctx->window->send_paint_request();
		return;
	}

	auto tool_hand_is_on = m_ctx->tabs->get_active_tab()->toolhandler->get_current_tool().type == ToolHandler::ToolType::HAND_MOVEMENT;
	if (tool_hand_is_on) {
		m_ctx->window->set_global_cursor(Window::CURSOR_TYPE::HAND);
	}

	if (p.type == Window::POINTER_TYPE::TOUCH or p.type == Window::POINTER_TYPE::TOUCHPAD or tool_hand_is_on) {
		m_gesture->end_gesture(p);
	}

	m_ctx->window->send_paint_request();
}

DocantoWin::MainWindowHandler::MainWindowHandler(HINSTANCE instance) {
	m_ctx = std::make_shared<Context>();
	m_ctx->window = std::make_shared<Window>(instance);
	m_ctx->render = std::make_shared<Direct2DRender>(m_ctx->window);
	m_ctx->caption = std::make_shared<Caption>(m_ctx->render);
	m_ctx->uihandler = std::make_shared<UIHandler>(m_ctx);

	m_ctx->uihandler->add(std::make_shared<UIDebugElement>(L"test"));
	m_ctx->uihandler->add(std::make_shared<UIToolbar>());

	m_ctx->tabs = std::make_shared<TabHandler>();

	m_ctx->window->set_callback_nchittest([&](Docanto::Geometry::Point<long> p) -> int {
		return m_ctx->caption->hittest(p);
	});

	m_ctx->window->set_callback_paint([&]() {
		this->paint();
	});

	m_ctx->window->set_callback_size([&](Docanto::Geometry::Dimension<long> d) {
		this->size(d);
		m_ctx->uihandler->resize(d);
	});

	m_ctx->window->set_callback_key([&](Window::VK key, bool presed) {
		this->key(key, presed);
	});

	m_ctx->window->set_callback_pointer_down([&](Window::PointerInfo d) {
		this->pointer_down(d);
	});
	m_ctx->window->set_callback_pointer_update([&](Window::PointerInfo d) {
		this->pointer_update(d);
	});

	m_ctx->window->set_callback_pointer_up([&](Window::PointerInfo d) {
		this->pointer_up(d);
	});

	auto path = open_file_dialog(L"PDF\0 * .pdf\0\0", m_ctx->window->get_hwnd());
	Docanto::Logger::log("Got path ", path);
	if (path.has_value()) {
		Docanto::Timer t;
		auto pdf = std::make_shared<PDFHandler>(path.value(), m_ctx->render);
		auto tool = std::make_shared<ToolHandler>(pdf);
		
		m_ctx->tabs->add(std::make_shared<TabContext>(pdf, tool));
		Docanto::Logger::log("Loaded PDF in ", t);
	}
	else {
		exit(0);
	}

	m_ctx->tabs->get_active_tab()->pdfhandler->request();
	m_gesture = std::make_shared<GestureHandler>(m_ctx->render, m_ctx->tabs);

}

void DocantoWin::MainWindowHandler::run() {
	Docanto::Logger::log("Entering main loop");
	m_ctx->window->set_state(Window::WINDOW_STATE::NORMAL);

	while (!m_ctx->window->get_close_request()) {
		Window::get_window_messages(true);
	}

	Docanto::Logger::log("Closing down and cleaning up");
}
