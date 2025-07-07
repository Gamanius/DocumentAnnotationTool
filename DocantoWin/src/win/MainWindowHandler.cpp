#include "MainWindowHandler.h"


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
	m_render->begin_draw();
	m_render->clear({ 50, 50, 50 });

	m_render->set_current_transform_active();
	m_pdfhandler->draw();

	m_render->set_identity_transform_active();
	m_uicaption->draw();
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
	case O:
	{

		m_pdfhandler = std::make_shared<PDFHandler>(L"C:/repos/Docanto/pdf_tests/Annotation pdf.pdf", m_render);
		m_pdfhandler->render();
		m_mainwindow->send_paint_request();
		break;
	}
	case DOWNARROW:
	{
		m_render->add_transform_matrix({ 0, -100 });
		m_mainwindow->send_paint_request();
		break;
	}
	case UPARROW:
	{
		m_render->add_transform_matrix({ 0, 100 });
		m_mainwindow->send_paint_request();
		break;
	}
	case LEFTARROW:
	{
		m_render->add_transform_matrix({ 100, 0 });
		m_mainwindow->send_paint_request();
		break;
	}
	case RIGHTARROW:
	{
		m_render->add_transform_matrix({ -100, 0 });
		m_mainwindow->send_paint_request();
		break;
	}
	case OEM_PLUS:
	{
		m_render->add_scale_matrix(1.05, m_mainwindow->get_mouse_pos());
		m_mainwindow->send_paint_request();
		break;
	}
	case OEM_MINUS:
	{
		m_render->add_scale_matrix(0.95, m_mainwindow->get_mouse_pos());
		m_mainwindow->send_paint_request();
		break;
	}
	}
}

void DocantoWin::MainWindowHandler::pointer_down(Window::PointerInfo p) {
}

void DocantoWin::MainWindowHandler::pointer_update(Window::PointerInfo p) {

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


	m_render->add_transform_matrix({ 100, 100 });
	auto path = open_file_dialog(L"PDF\0 * .pdf\0\0", m_mainwindow->get_hwnd());
	Docanto::Logger::log("Got path ", path);
	if (path.has_value()) {
		m_pdfhandler = std::make_shared<PDFHandler>(path.value(), m_render);
		m_pdfhandler->render();
	}
	else {
		exit(0);
	}

}

void DocantoWin::MainWindowHandler::run() {
	Docanto::Logger::log("Entering main loop");
	m_mainwindow->set_state(Window::WINDOW_STATE::NORMAL);

	while (!m_mainwindow->get_close_request()) {
		Window::get_window_messages(true);
	}

	Docanto::Logger::log("Closing down and cleaning up");
}
