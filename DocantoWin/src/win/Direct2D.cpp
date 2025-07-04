#include "Direct2D.h"
#include <wincodec.h>
#include <d2d1.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

ID2D1Factory* DocantoWin::Direct2DRender::m_factory = nullptr;
IDWriteFactory3* DocantoWin::Direct2DRender::m_writeFactory = nullptr;

DocantoWin::Direct2DRender::Direct2DRender(std::shared_ptr<Window> w) : m_window(w) {
	D2D1_FACTORY_OPTIONS option{};

#ifdef _DEBUG
	option.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#else
	option.debugLevel = D2D1_DEBUG_LEVEL_NONE;
#endif // _DEBUG

	HRESULT result = S_OK;
	if (m_factory == nullptr) {
		result = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, option, &m_factory);

		if (result != S_OK) {
			Docanto::Logger::error("Could not initialize Direct2D");
			return;
		}
	}
	else {
		m_factory->AddRef();
	}

	// create some text rendering
	if (m_writeFactory == nullptr) {
		result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3), reinterpret_cast<IUnknown**>(&m_writeFactory));
		if (result != S_OK) {
			Docanto::Logger::error("Could not initilaize text rendering");
		}
	}
	else {
		m_writeFactory->AddRef();
	}

	auto props = D2D1::HwndRenderTargetProperties(m_window->m_hwnd, DimensionToD2D1(m_window->get_window_size()));
	props.presentOptions = D2D1_PRESENT_OPTIONS_IMMEDIATELY;
	result = m_factory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		props,
		&m_renderTarget
	);

	m_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

	if (result != S_OK) {
		Docanto::Logger::error("Could not initilaize HWND rendering");
	}	
}

DocantoWin::Direct2DRender::~Direct2DRender() {
	SafeRelease(m_factory);
	SafeRelease(m_renderTarget);
	SafeRelease(m_writeFactory);
}

void DocantoWin::Direct2DRender::begin_draw() {
	std::lock_guard lock(draw_lock);
	if (m_isRenderinProgress == 0)
		m_renderTarget->BeginDraw();
	m_isRenderinProgress++;
}

void DocantoWin::Direct2DRender::end_draw() {
	std::lock_guard lock(draw_lock);
	m_isRenderinProgress--;
	if (m_isRenderinProgress == 0)
		m_renderTarget->EndDraw();
}

void DocantoWin::Direct2DRender::clear(Docanto::Color c) {
	begin_draw();
	m_renderTarget->Clear(ColorToD2D1(c));
	end_draw();
}

std::shared_ptr<DocantoWin::Window> DocantoWin::Direct2DRender::get_attached_window() const {
	return m_window;
}

void DocantoWin::Direct2DRender::resize(Docanto::Geometry::Dimension<long> r) {
	m_renderTarget->Resize(DimensionToD2D1(r));

	// we should also check if the dpi changed
	UINT dpiX, dpiY;
	dpiX = dpiY = GetDpiForWindow(m_window->m_hwnd);
	m_renderTarget->SetDpi(static_cast<float>(dpiX), static_cast<float>(dpiY));
}

void DocantoWin::Direct2DRender::draw_text(const std::wstring& text, Docanto::Geometry::Point<float> pos, TextFormatObject& format, BrushObject& brush) {
	auto layout = create_text_layout(text, format);

	begin_draw();

	m_renderTarget->DrawTextLayout(
		m_window->PxToDp(pos),
		layout.m_object,
		brush.m_object,
		D2D1_DRAW_TEXT_OPTIONS_NONE);

	end_draw();
}

void DocantoWin::Direct2DRender::draw_text(const std::wstring& text, Docanto::Geometry::Point<float> pos, Docanto::Color c, float size) {
	auto format = create_text_format(L"Consolas", size);
	auto brush = create_brush(c);
	draw_text(text, pos, format, brush);
}

void DocantoWin::Direct2DRender::draw_rect(Docanto::Geometry::Rectangle<float> r, Docanto::Color c, float thic) {
	auto brush = create_brush(c);
	draw_rect(r, brush, thic);
}
void DocantoWin::Direct2DRender::draw_rect(Docanto::Geometry::Rectangle<float> r, Docanto::Color c) {
	auto brush = create_brush(c);
	draw_rect(r, brush);
}

void DocantoWin::Direct2DRender::draw_rect(Docanto::Geometry::Rectangle<float> r, BrushObject& brush, float thic) {
	m_renderTarget->DrawRectangle(m_window->PxToDp(r), brush.m_object, thic);
}
void DocantoWin::Direct2DRender::draw_rect_filled(Docanto::Geometry::Rectangle<float> r, BrushObject& brush) {
	m_renderTarget->FillRectangle(m_window->PxToDp(r), brush.m_object);
}

void DocantoWin::Direct2DRender::draw_line(Docanto::Geometry::Point<float> p1, Docanto::Geometry::Point<float> p2, BrushObject& brush, float thick) {
	begin_draw();
	m_renderTarget->DrawLine(m_window->PxToDp(p1), m_window->PxToDp(p2), brush.m_object, thick);
	end_draw();
}

void DocantoWin::Direct2DRender::draw_line(Docanto::Geometry::Point<float> p1, Docanto::Geometry::Point<float> p2, Docanto::Color c, float thick) {
	begin_draw();
	auto brush = create_brush(c);
	draw_line(p1, p2, brush, thick);
	end_draw();
}

DocantoWin::Direct2DRender::TextFormatObject DocantoWin::Direct2DRender::create_text_format(std::wstring font, float size) {
	TextFormatObject text;
	IDWriteTextFormat* textformat = nullptr;

	HRESULT result = m_writeFactory->CreateTextFormat(
		font.c_str(),
		nullptr,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		size,
		L"en-us",
		&textformat
	);

	if (result != S_OK) {
		Docanto::Logger::error("Could not create text format");
	}

	text.m_object = textformat;

	return std::move(text);
}

DocantoWin::Direct2DRender::TextLayoutObject DocantoWin::Direct2DRender::create_text_layout(const std::wstring &text, TextFormatObject& format) {
	TextLayoutObject obj;
	IDWriteTextLayout* textLayout;
	auto result = m_writeFactory->CreateTextLayout(
		text.c_str(),
		(uint32_t)text.length(),
		format.m_object,
		FLT_MAX,
		FLT_MAX,
		&textLayout
	); 

	if (result != S_OK) {
		Docanto::Logger::error("Could not create D2D1 text layout");
	}
	obj.m_object = textLayout;

	return std::move(obj);
}

DocantoWin::Direct2DRender::BrushObject DocantoWin::Direct2DRender::create_brush(Docanto::Color c) {
	BrushObject obj;
	ID2D1SolidColorBrush* brush = nullptr;
	auto result = m_renderTarget->CreateSolidColorBrush(ColorToD2D1(c), &brush);
	if (result != S_OK) {
		Docanto::Logger::error("Could not create D2D1 brush");
	}
	obj.m_object = brush;

	return std::move(obj);
}
