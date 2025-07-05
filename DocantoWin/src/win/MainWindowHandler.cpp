#include "MainWindowHandler.h"


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


	m_pdfhandler = std::make_shared<PDFHandler>(L"C:/repos/Docanto/pdf_tests/1.pdf", m_render);
	m_pdfhandler->render();
	m_render->add_transform_matrix({ 100, 100 });
	
}

void DocantoWin::MainWindowHandler::run() {
	Docanto::Logger::log("Entering main loop");
	m_mainwindow->set_state(Window::WINDOW_STATE::NORMAL);

	while (!m_mainwindow->get_close_request()) {
		Window::get_window_messages(true);
	}

	Docanto::Logger::log("Closing down and cleaning up");
}
