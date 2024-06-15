#include "include.h"
#include <wincodec.h>
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
		result = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, option, &m_factory);
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
	m_window_size = r;
}

void Direct2DRenderer::draw_bitmap(BitmapObject& bitmap, Renderer::Point<float> pos, INTERPOLATION_MODE mode, float opacity) {
	begin_draw(); 
	m_renderTarget->DrawBitmap(bitmap.m_object, D2D1::RectF(pos.x, pos.y, pos.x + bitmap.m_object->GetSize().width, pos.y + bitmap.m_object->GetSize().height),
		opacity, mode == INTERPOLATION_MODE::NEAREST_NEIGHBOR ? D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR : D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
	end_draw();	
}

void Direct2DRenderer::draw_bitmap(BitmapObject& bitmap, Renderer::Rectangle<float> dest, INTERPOLATION_MODE mode, float opacity) {
	begin_draw();
	m_renderTarget->DrawBitmap(bitmap.m_object, dest, opacity, 
		mode == INTERPOLATION_MODE::NEAREST_NEIGHBOR ? D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR : D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
	end_draw();
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

void Direct2DRenderer::draw_text(const std::wstring& text, Renderer::Point<float> pos, Renderer::Color c, float size) {
	auto format = create_text_format(L"Consolas", size);
	auto brush  = create_brush(c);
	draw_text(text, pos, format, brush);
}

void Direct2DRenderer::draw_rect(Renderer::Rectangle<float> rec, BrushObject& brush, float thicc) {
	begin_draw();
	m_renderTarget->DrawRectangle(rec, brush.m_object, thicc);
	end_draw();
}

void Direct2DRenderer::draw_rect(Renderer::Rectangle<float> rec, BrushObject& brush) {
	draw_rect(rec, brush, 1.0f);
}

void Direct2DRenderer::draw_rect_filled(Renderer::Rectangle<float> rec, BrushObject& brush) {
	begin_draw();
	m_renderTarget->FillRectangle(rec, brush.m_object);
	end_draw();
}

void Direct2DRenderer::draw_rect(Renderer::Rectangle<float> rec, Renderer::Color c, float thick) {
	// create the brush object
	auto temp_brush = create_brush(c);
	draw_rect(rec, temp_brush, thick);
}

void Direct2DRenderer::draw_rect(Renderer::Rectangle<float> rec, Renderer::Color c) {
	draw_rect(rec, c, 1.0f);
}

void Direct2DRenderer::draw_rect_filled(Renderer::Rectangle<float> rec, Renderer::Color c) {
	// create brush object
	auto temp_brush = create_brush(c);
	draw_rect_filled(rec, temp_brush);
}

void Direct2DRenderer::set_current_transform_active() {
	m_renderTarget->SetTransform(m_transformPosMatrix * m_transformScaleMatrix);
}

void Direct2DRenderer::set_identity_transform_active() {
	m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}

void Direct2DRenderer::set_transform_matrix(Renderer::Point<float> p) {
	m_transformPos = p;
	m_transformPosMatrix = D2D1::Matrix3x2F::Translation(p.x, p.y);
}

void Direct2DRenderer::add_transform_matrix(Renderer::Point<float> p) {
	m_transformPos += p;
	m_transformPosMatrix = D2D1::Matrix3x2F::Translation(p.x, p.y) * m_transformPosMatrix;
}

void Direct2DRenderer::set_scale_matrix(float scale, Renderer::Point<float> center) {
	m_transformScale = scale;
	m_transformScaleCenter = center;

	m_transformScaleMatrix = D2D1::Matrix3x2F::Scale({ scale, scale }, center);
}

void Direct2DRenderer::add_scale_matrix(float scale, Renderer::Point<float> center) {
	m_transformScale *= scale;
	m_transformScaleCenter = center;

	// calc the new scale matrix
	// boi i wish i would know how this works again...
	auto mat = D2D1::Matrix3x2F(m_transformScaleMatrix);
	mat.Invert();
	mat = D2D1::Matrix3x2F::Scale({ scale, scale }, mat.TransformPoint(center));
	m_transformScaleMatrix = mat * m_transformScaleMatrix;
}

float Direct2DRenderer::get_transform_scale() const {
	return m_transformScale;
}

Renderer::Point<float> Direct2DRenderer::get_transform_pos() const {
	return m_transformPos;
}

Renderer::Rectangle<long> Direct2DRenderer::get_window_size() const {
	return m_window_size;
}

Renderer::Rectangle<double> Direct2DRenderer::get_window_size_normalized() const {
	auto dpi = get_dpi();
	return Renderer::Rectangle<double>(0, 0, m_window_size.width * (96.0f / dpi), m_window_size.height * (96.0f / dpi));
}

Renderer::Rectangle<float> Direct2DRenderer::transform_rect(const Renderer::Rectangle<float> rec) const {
	auto transform = m_transformPosMatrix * m_transformScaleMatrix;
	auto p1 = transform.TransformPoint({ rec.x, rec.y });
	auto p2 = transform.TransformPoint({ rec.right(), rec.bottom()});
	return { p1, p2 };
}

Renderer::Rectangle<float> Direct2DRenderer::inv_transform_rect(const Renderer::Rectangle<float> rec) const {
	auto transform = m_transformPosMatrix * m_transformScaleMatrix;
	transform.Invert();
	auto p1 = transform.TransformPoint({ rec.x, rec.y });
	auto p2 = transform.TransformPoint({ rec.right(), rec.bottom() });
	return { p1, p2 };
}

Renderer::Point<float> Direct2DRenderer::transform_point(const Renderer::Point<float> p) const {
	auto transform = m_transformPosMatrix * m_transformScaleMatrix; 
	return transform.TransformPoint(p); 
}

Renderer::Point<float> Direct2DRenderer::inv_transform_point(const Renderer::Point<float> p) const {
	auto transform = m_transformPosMatrix * m_transformScaleMatrix;
	transform.Invert();
	return transform.TransformPoint(p);
}

UINT Direct2DRenderer::get_dpi() const {
	return GetDpiForWindow(m_hwnd);
}

void Direct2DRenderer::begin_draw() {
	std::lock_guard lock(draw_lock);
	if (m_isRenderinProgress == 0)
		m_renderTarget->BeginDraw();
	m_isRenderinProgress++;
}

void Direct2DRenderer::end_draw() {
	std::lock_guard lock(draw_lock);
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

Direct2DRenderer::BitmapObject Direct2DRenderer::create_bitmap(const std::wstring& path) {
	// might be slow 
	BitmapObject obj; 
	IWICImagingFactory* pWICFactory = nullptr;
	IWICBitmapFrameDecode* pFrame = nullptr; 
	IWICBitmapDecoder* pDecoder = nullptr;
	IWICFormatConverter* pConverter = nullptr;
	ID2D1Bitmap* pBitmap = nullptr; 

	auto hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<void**>(&pWICFactory)); 
	ASSERT_WITH_STATEMENT(hr == S_OK, "Could not create WIC factory!", goto REALEASE; );

	// Load the bitmap using WIC
	hr = pWICFactory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
	ASSERT_WITH_STATEMENT(hr == S_OK, "Could not create WIC decoder!", goto REALEASE; );

	hr = pDecoder->GetFrame(0, &pFrame); 
	ASSERT_WITH_STATEMENT(hr == S_OK, "Could not get frame from WIC decoder!", pDecoder->Release(); pWICFactory->Release(); goto REALEASE; );

	// Create converter 
	hr = pWICFactory->CreateFormatConverter(&pConverter);
	ASSERT_WITH_STATEMENT(hr == S_OK, "Could not create WIC format converter!",  goto REALEASE; );
	hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeCustom);
	ASSERT_WITH_STATEMENT(hr == S_OK, "Could not initialize WIC format converter!", goto REALEASE; );

	// Convert the WIC bitmap to Direct2D bitmap
	hr = m_renderTarget->CreateBitmapFromWicBitmap(pConverter, nullptr, &pBitmap); 
	ASSERT_WITH_STATEMENT(hr == S_OK, "Could not create Direct2D bitmap!", goto REALEASE; );
	obj.m_object = pBitmap;

REALEASE:
	SafeRelease(pWICFactory); 
	SafeRelease(pDecoder);  
	SafeRelease(pFrame);  
	SafeRelease(pConverter); 

	return std::move(obj); 
}

Direct2DRenderer::BitmapObject Direct2DRenderer::create_bitmap(const byte* const data, Renderer::Rectangle<unsigned int> size, unsigned int stride, float dpi) {
	ID2D1Bitmap* pBitmap = nullptr;
	D2D1_BITMAP_PROPERTIES prop;

	prop.dpiX = dpi;
	prop.dpiY = dpi;
	prop.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	prop.pixelFormat.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	HRESULT res = m_renderTarget->CreateBitmap({ size.width, size.height }, data, stride, prop, &pBitmap);
	ASSERT_WIN(res == S_OK, "Could not create bitmap!"); 

	BitmapObject obj;
	obj.m_object = pBitmap;

	return std::move(obj);
}
