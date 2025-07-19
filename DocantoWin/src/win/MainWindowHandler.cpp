#include "MainWindowHandler.h"


Docanto::Geometry::Rectangle<float> rectangle;

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
	m_pdfhandler->request();

	m_render->begin_draw();
	m_render->clear();

	m_render->set_current_transform_active();
	m_pdfhandler->draw();

	m_render->set_identity_transform_active();
	m_uicaption->draw();
	m_render->draw_rect(rectangle, { 255,0,0,255});
	m_render->end_draw();
}
void DocantoWin::MainWindowHandler::size(Docanto::Geometry::Dimension<long> d) {
	m_render->resize(d);
}

void DocantoWin::MainWindowHandler::key(Window::VK key, bool pressed) {
	if (pressed == false) {
		return;
	}

	using enum Window::VK;

	switch (key) {
	case F4:
	{
		if (m_mainwindow->is_key_pressed(ALT)) {
			m_mainwindow->send_close_request();
		}
		break;
	}
	case F10:
	{
		m_pdfhandler->toggle_debug_draw();
		m_mainwindow->send_paint_request();
		break;
	}
	case O:
	{
		auto path = open_file_dialog(L"PDF\0 * .pdf\0\0", m_mainwindow->get_hwnd());
		Docanto::Logger::log("Got path ", path);
		if (path.has_value()) {
			Docanto::Timer t;
			m_pdfhandler = std::make_shared<PDFHandler>(path.value(), m_render);
			Docanto::Logger::log("Loaded PDF in ", t);
		}
		else {
			exit(0);
		}

		m_pdfhandler->render();
		m_gesture = std::make_shared<GestureHandler>(m_render, m_pdfhandler);
		break;
	}
	case R: 
	{
		AppVariables::TOUCH_ALLOW_ROTATION = !AppVariables::TOUCH_ALLOW_ROTATION;
		m_render->set_rotation_matrix(0, { 0, 0 });
		m_mainwindow->send_paint_request();
		break;
	}
	case E:
	{
		m_render->add_rotation_matrix(10, m_mainwindow->get_mouse_pos());
		m_pdfhandler->render();
		m_mainwindow->send_paint_request();
		break;
	}
	case Q:
	{
		m_render->add_rotation_matrix(-10, m_mainwindow->get_mouse_pos());
		m_pdfhandler->render();
		m_mainwindow->send_paint_request();
		break;
	}
	case DOWNARROW:
	{
		m_render->add_translation_matrix({ 0, -100 });
		m_pdfhandler->render();
		m_mainwindow->send_paint_request();
		break;
	}
	case UPARROW:
	{
		m_render->add_translation_matrix({ 0, 100 });
		m_pdfhandler->render();
		m_mainwindow->send_paint_request();
		break;
	}
	case LEFTARROW:
	{
		m_render->add_translation_matrix({ 100, 0 });
		m_pdfhandler->render();
		m_mainwindow->send_paint_request();
		break;
	}
	case RIGHTARROW:
	{
		m_render->add_translation_matrix({ -100, 0 });
		m_pdfhandler->render();
		m_mainwindow->send_paint_request();
		break;
	}
	case OEM_PLUS:
	{
		m_render->add_scale_matrix(1.05, m_mainwindow->get_mouse_pos());
		m_pdfhandler->render();
		m_mainwindow->send_paint_request();
		break;
	}
	case OEM_MINUS:
	{
		m_render->add_scale_matrix(0.95, m_mainwindow->get_mouse_pos());
		m_pdfhandler->render();
		m_mainwindow->send_paint_request();
		break;
	}
	case BACKSPACE:
	{
		m_render->set_rotation_matrix(D2D1::Matrix3x2F::Identity());
		m_render->set_scale_matrix(D2D1::Matrix3x2F::Identity());
		m_render->set_translation_matrix(D2D1::Matrix3x2F::Identity());
		[[fallthrough]];
	}
	case SPACE:
	{
		m_pdfhandler->render();
		m_mainwindow->send_paint_request();
	}

	}
}

void DocantoWin::MainWindowHandler::pointer_down(Window::PointerInfo p) {
	if (!m_gesture) {
		return;
	}
	// there will never be a touchpad pointer down event since we cant track them
	if (p.type == Window::POINTER_TYPE::TOUCH or p.type == Window::POINTER_TYPE::TOUCHPAD) {
		m_gesture->start_gesture(p);
	}

	m_mainwindow->send_paint_request();
}

void DocantoWin::MainWindowHandler::pointer_update(Window::PointerInfo p) {
	if (!m_gesture) {
		return;
	}

	if (p.type == Window::POINTER_TYPE::TOUCH or p.type == Window::POINTER_TYPE::TOUCHPAD) {
		m_gesture->update_gesture(p);
		m_mainwindow->send_paint_request();
	}
}

void DocantoWin::MainWindowHandler::pointer_up(Window::PointerInfo p) {
	if (!m_gesture) {
		return;
	}

	if (p.type == Window::POINTER_TYPE::TOUCH or p.type == Window::POINTER_TYPE::TOUCHPAD) {
		m_gesture->end_gesture(p);
	}

	m_pdfhandler->render();
	m_mainwindow->send_paint_request();
}

DocantoWin::MainWindowHandler::MainWindowHandler(HINSTANCE instance) {
	m_mainwindow = std::make_shared<Window>(instance);
	m_render = std::make_shared<Direct2DRender>(m_mainwindow);
	m_uicaption = std::make_shared<Caption>(m_render);

	m_mainwindow->set_callback_nchittest([&](Docanto::Geometry::Point<long> p) -> int {
		return m_uicaption->hittest(p);
	});

	m_mainwindow->set_callback_paint([&]() {
		this->paint();
	});

	m_mainwindow->set_callback_size([&](Docanto::Geometry::Dimension<long> d) {
		this->size(d);
	});

	m_mainwindow->set_callback_key([&](Window::VK key, bool presed) {
		this->key(key, presed);
	});

	m_mainwindow->set_callback_pointer_down([&](Window::PointerInfo d) {
		this->pointer_down(d);
	});
	m_mainwindow->set_callback_pointer_update([&](Window::PointerInfo d) {
		this->pointer_update(d);
	});

	m_mainwindow->set_callback_pointer_up([&](Window::PointerInfo d) {
		this->pointer_up(d);
	});

	auto path = open_file_dialog(L"PDF\0 * .pdf\0\0", m_mainwindow->get_hwnd());
	Docanto::Logger::log("Got path ", path);
	if (path.has_value()) {
		Docanto::Timer t;
		m_pdfhandler = std::make_shared<PDFHandler>(path.value(), m_render);
		Docanto::Logger::log("Loaded PDF in ", t);
	}
	else {
		exit(0);
	}

	m_pdfhandler->request();
	m_pdfhandler->render();
	m_gesture = std::make_shared<GestureHandler>(m_render, m_pdfhandler);

}

void DocantoWin::MainWindowHandler::run() {
	Docanto::Logger::log("Entering main loop");
	m_mainwindow->set_state(Window::WINDOW_STATE::NORMAL);

	while (!m_mainwindow->get_close_request()) {
		Window::get_window_messages(true);
	}

	Docanto::Logger::log("Closing down and cleaning up");
}
