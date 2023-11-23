#include "include.h"
#include <d2d1.h>

ID2D1Factory* Direct2DRenderer::m_factory = nullptr;
IDWriteFactory3* Direct2DRenderer::m_writeFactory = nullptr;


Direct2DRenderer::Direct2DRenderer(const WindowHandler& w) {
	m_hdc = w.get_hdc();
	m_hwnd = w.get_hwnd();
	m_window_size = w.get_size();

	D2D1_FACTORY_OPTIONS option{};

#ifdef _DEBUG
		option.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#else
	option.debugLevel = D2D1_DEBUG_LEVEL_NONE;
#endif // _DEBUG

	HRESULT result = S_OK;
	if (m_factory == nullptr) {
		result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, option, &m_factory);
		ASSERT(result == S_OK, "Could not create Direct2D factory!");
	}
	else {
		m_factory->AddRef(); 
	}

	// create some text rendering
	if (m_writeFactory == nullptr) {
		result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3), reinterpret_cast<IUnknown**>(&m_writeFactory));
		ASSERT(result == S_OK, "Could not create DirectWrite factory!");
	}
	else {
		m_writeFactory->AddRef(); 
	}

	result = m_factory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(m_hwnd, m_window_size),
		&m_renderTarget
	);
}

Direct2DRenderer::~Direct2DRenderer() {
	SafeRelease(m_factory);
	SafeRelease(m_renderTarget);
	SafeRelease(m_writeFactory);
}

void Direct2DRenderer::clear(Renderer::Color c) {
	begin_draw();
	m_renderTarget->Clear(c);
	end_draw();
}

void Direct2DRenderer::resize(Renderer::Rectangle<long> r) {
	m_renderTarget->Resize(r);
}

void Direct2DRenderer::draw_text(const std::wstring& text, Renderer::Point<float> pos, TextFormatObject& format, BrushObject& brush) {
	IDWriteTextLayout* textLayout;
	auto result = m_writeFactory->CreateTextLayout(
		text.c_str(),
		(uint32_t)text.length(),
		format.m_object,
		FLT_MAX,
		FLT_MAX,
		&textLayout
	);
	ASSERT(result == S_OK, "Could not create text layout!");

	begin_draw();

	m_renderTarget->DrawTextLayout(
		pos,
		textLayout,
		brush.m_object,
		D2D1_DRAW_TEXT_OPTIONS_NONE);

	end_draw();

	SafeRelease(textLayout);

}

void Direct2DRenderer::begin_draw() {
	if (m_isRenderinProgress == 0)
		m_renderTarget->BeginDraw();
	m_isRenderinProgress++;
}

void Direct2DRenderer::end_draw() {
	m_isRenderinProgress--;
	if (m_isRenderinProgress == 0)
		m_renderTarget->EndDraw();
}

Direct2DRenderer::TextFormatObject Direct2DRenderer::create_text_format(std::wstring font, float size) {
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

	ASSERT(result == S_OK, "Could not create text format!");

	text.m_object = textformat;

	return std::move(text);
}


Direct2DRenderer::BrushObject Direct2DRenderer::create_brush(Renderer::Color c) {
	BrushObject obj;
	ID2D1SolidColorBrush* brush = nullptr; 
	auto result = m_renderTarget->CreateSolidColorBrush(c, &brush);

	ASSERT(result == S_OK, "Could not create brush!");
	obj.m_object = brush;

	return std::move(obj);
}