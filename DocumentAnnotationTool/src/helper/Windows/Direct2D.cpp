#include "Direct2D.h"
#include <wincodec.h>
#include <d2d1.h>

ID2D1Factory* Direct2DRenderer::m_factory = nullptr;
IDWriteFactory3* Direct2DRenderer::m_writeFactory = nullptr;

Direct2DRenderer::Direct2DRenderer(const WindowHandler& w) {
	m_hdc = w.get_hdc();
	m_hwnd = w.get_hwnd();
	m_window_size = w.get_window_size();

	D2D1_FACTORY_OPTIONS option{};

#ifdef _DEBUG
	option.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#else
	option.debugLevel = D2D1_DEBUG_LEVEL_NONE;
#endif // _DEBUG

	HRESULT result = S_OK;
	if (m_factory == nullptr) {
		result = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, option, &m_factory);
		ASSERT_WIN(result == S_OK, "Could not create Direct2D factory!");
	}
	else {
		m_factory->AddRef(); 
	}

	// create some text rendering
	if (m_writeFactory == nullptr) {
		result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3), reinterpret_cast<IUnknown**>(&m_writeFactory));
		ASSERT_WIN(result == S_OK, "Could not create DirectWrite factory!");
	}
	else {
		m_writeFactory->AddRef(); 
	}

	auto props = D2D1::HwndRenderTargetProperties(m_hwnd, m_window_size);
	props.presentOptions = D2D1_PRESENT_OPTIONS_IMMEDIATELY;
	result = m_factory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		props,
		&m_renderTarget
	);

	m_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	resize(m_window_size);
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

void Direct2DRenderer::resize(Math::Rectangle<long> r) {
	m_renderTarget->Resize(r);

	// we should also check if the dpi changed
    UINT dpiX, dpiY;
    dpiX = dpiY = GetDpiForWindow(m_hwnd);
    m_renderTarget->SetDpi(static_cast<float>(dpiX), static_cast<float>(dpiY));

	m_window_size = r;
}

void Direct2DRenderer::draw_bitmap(BitmapObject& bitmap, Math::Point<float> pos, INTERPOLATION_MODE mode, float opacity) {
	begin_draw(); 
	m_renderTarget->DrawBitmap(bitmap.m_object, D2D1::RectF(pos.x, pos.y, pos.x + bitmap.m_object->GetSize().width, pos.y + bitmap.m_object->GetSize().height),
		opacity, mode == INTERPOLATION_MODE::NEAREST_NEIGHBOR ? D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR : D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
	end_draw();	
}

void Direct2DRenderer::draw_bitmap(BitmapObject& bitmap, Math::Rectangle<float> dest, INTERPOLATION_MODE mode, float opacity) {
	begin_draw();
	m_renderTarget->DrawBitmap(bitmap.m_object, dest, opacity, 
		mode == INTERPOLATION_MODE::NEAREST_NEIGHBOR ? D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR : D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
	end_draw();
}

void Direct2DRenderer::draw_text(const std::wstring& text, Math::Point<float> pos, TextFormatObject& format, BrushObject& brush) {
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

void Direct2DRenderer::draw_text(const std::wstring& text, Math::Point<float> pos, Renderer::Color c, float size) {
	auto format = create_text_format(L"Consolas", size);
	auto brush  = create_brush(c);
	draw_text(text, pos, format, brush);
}

void Direct2DRenderer::draw_line(Math::Point<float> p1, Math::Point<float> p2, BrushObject& brush, float thick) {
	begin_draw();
	m_renderTarget->DrawLine(p1, p2, brush.m_object, thick);
	end_draw();
}

void Direct2DRenderer::draw_line(Math::Point<float> p1, Math::Point<float> p2, Renderer::Color c, float thick) {
	begin_draw();
	auto brush = create_brush(c);
	draw_line(p1, p2, brush, thick);
	end_draw();
}

void Direct2DRenderer::draw_line(Math::Point<float> p1, Math::Point<float> p2, Renderer::AlphaColor c, float thick) {
	begin_draw();
	auto brush = create_brush(c);
	draw_line(p1, p2, brush, thick);
	end_draw();
}

void Direct2DRenderer::draw_path(PathObject& obj, BrushObject& brush, float thick) {
	begin_draw();
	m_renderTarget->DrawGeometry(obj.m_object, brush.m_object, thick); 
	end_draw();
}

void Direct2DRenderer::draw_path(PathObject& obj, Renderer::Color c, float thick) {
	begin_draw();
	auto brush = create_brush(c); 
	draw_path(obj, brush, thick);
	end_draw();
}

void Direct2DRenderer::draw_path(PathObject& obj, Renderer::AlphaColor c, float thick) {
	begin_draw();
	auto brush = create_brush(c);
	draw_path(obj, brush, thick);
	end_draw();
}

void Direct2DRenderer::draw_rect(Math::Rectangle<float> rec, BrushObject& brush, float thick, std::vector<float> dashes) {
	begin_draw();

	if (dashes.empty()) {
		m_renderTarget->DrawRectangle(rec, brush.m_object, thick);
	}
	else {
		auto style = create_stroke_style(dashes); 
		m_renderTarget->DrawRectangle(rec, brush.m_object, thick, style);  
	}
	end_draw();
}

void Direct2DRenderer::draw_rect(Math::Rectangle<float> rec, BrushObject& brush, float thicc) {
	draw_rect(rec, brush, thicc, {});
}

void Direct2DRenderer::draw_rect(Math::Rectangle<float> rec, BrushObject& brush) {
	draw_rect(rec, brush, 1.0f);
}

void Direct2DRenderer::draw_rect_filled(Math::Rectangle<float> rec, BrushObject& brush) {
	begin_draw();
	m_renderTarget->FillRectangle(rec, brush.m_object);
	end_draw();
}

void Direct2DRenderer::draw_circle(Math::Point<float> center, float radius, BrushObject& brush, float thick) {
	begin_draw();
	m_renderTarget->DrawEllipse({ center, radius, radius }, brush.m_object, thick); 
	end_draw();
}

void Direct2DRenderer::draw_circle(Math::Point<float> center, float radius, BrushObject& brush) {
	draw_circle(center, radius, brush, 1.0f);
}

void Direct2DRenderer::draw_circle_filled(Math::Point<float> center, float radius, BrushObject& brush) {
	begin_draw();
	m_renderTarget->FillEllipse({ center, radius, radius }, brush.m_object); 
	end_draw();
}

void Direct2DRenderer::draw_rect(Math::Rectangle<float> rec, Renderer::Color c, float thick, std::vector<float> dashes) {
	auto temp_brush = create_brush(c);
	draw_rect(rec, temp_brush, thick, dashes);
}

void Direct2DRenderer::draw_rect(Math::Rectangle<float> rec, Renderer::Color c, float thick) {
	// create the brush object
	auto temp_brush = create_brush(c);
	draw_rect(rec, temp_brush, thick);
}

void Direct2DRenderer::draw_rect(Math::Rectangle<float> rec, Renderer::Color c) {
	draw_rect(rec, c, 1.0f);
}

void Direct2DRenderer::draw_rect_filled(Math::Rectangle<float> rec, Renderer::Color c) {
	// create brush object
	auto temp_brush = create_brush(c);
	draw_rect_filled(rec, temp_brush);
}

void Direct2DRenderer::draw_circle(Math::Point<float> center, float radius, Renderer::Color c, float thick) {
	auto temp_brush = create_brush(c);
	draw_circle(center, radius, temp_brush, thick);
}

void Direct2DRenderer::draw_circle(Math::Point<float> center, float radius, Renderer::Color c) {
	draw_circle(center, radius, c, 1.0f);
}

void Direct2DRenderer::draw_circle_filled(Math::Point<float> center, float radius, Renderer::Color c) {
	auto temp_brush = create_brush(c);
	draw_circle_filled(center, radius, temp_brush); 
}

void Direct2DRenderer::set_current_transform_active() {
	m_renderTarget->SetTransform(m_transformPosMatrix * m_transformScaleMatrix);
}

void Direct2DRenderer::set_identity_transform_active() {
	m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}

void Direct2DRenderer::set_transform_matrix(Math::Point<float> p) {
	m_transformPosMatrix = D2D1::Matrix3x2F::Translation(p.x, p.y);
}

void Direct2DRenderer::set_transform_matrix(D2D1::Matrix3x2F m) {
	m_transformPosMatrix = m;
}

void Direct2DRenderer::add_transform_matrix(Math::Point<float> p) {
	m_transformPosMatrix = D2D1::Matrix3x2F::Translation(p.x, p.y) * m_transformPosMatrix;
}

void Direct2DRenderer::set_scale_matrix(float scale, Math::Point<float> center) {
	m_transformScaleMatrix = D2D1::Matrix3x2F::Scale({ scale, scale }, center);
}

void Direct2DRenderer::set_scale_matrix(D2D1::Matrix3x2F m) {
	m_transformScaleMatrix = m;
}

void Direct2DRenderer::add_scale_matrix(float scale, Math::Point<float> center) {
	// calc the new scale matrix
	// boi i wish i would know how this works again...
	auto mat = D2D1::Matrix3x2F(m_transformScaleMatrix);
	mat.Invert();
	mat = D2D1::Matrix3x2F::Scale({ scale, scale }, mat.TransformPoint(center));
	m_transformScaleMatrix = mat * m_transformScaleMatrix;
}

float Direct2DRenderer::get_transform_scale() const {
	return m_transformScaleMatrix._11;
}

D2D1::Matrix3x2F Direct2DRenderer::get_scale_matrix() const {
	return m_transformScaleMatrix;
}

Math::Point<float> Direct2DRenderer::get_zoom_center() const {
	if (FLOAT_EQUAL(m_transformScaleMatrix._11, 1)) {
		return { m_transformScaleMatrix._31, m_transformScaleMatrix._32 };
	}
	return { m_transformScaleMatrix._31 / (1 - m_transformScaleMatrix._11),
		m_transformScaleMatrix._32 / (1 - m_transformScaleMatrix._11), };
}

Math::Point<float> Direct2DRenderer::get_transform_pos() const {
	return { m_transformPosMatrix._31, m_transformPosMatrix._32 };
}

Math::Rectangle<long> Direct2DRenderer::get_window_size() const {
	return m_window_size;
}

Math::Rectangle<double> Direct2DRenderer::get_window_size_normalized() const {
	auto dpi = get_dpi();
	return Math::Rectangle<double>(0, 0, m_window_size.width * (96.0f / dpi), m_window_size.height * (96.0f / dpi));
}

Math::Rectangle<float> Direct2DRenderer::transform_rect(const Math::Rectangle<float> rec) const {
	auto transform = m_transformPosMatrix * m_transformScaleMatrix;
	auto p1 = transform.TransformPoint({ rec.x, rec.y });
	auto p2 = transform.TransformPoint({ rec.right(), rec.bottom()});
	return { p1, p2 };
}

Math::Rectangle<float> Direct2DRenderer::inv_transform_rect(const Math::Rectangle<float> rec) const {
	auto transform = m_transformPosMatrix * m_transformScaleMatrix;
	transform.Invert();
	auto p1 = transform.TransformPoint({ rec.x, rec.y });
	auto p2 = transform.TransformPoint({ rec.right(), rec.bottom() });
	return { p1, p2 };
}

Math::Point<float> Direct2DRenderer::transform_point(const Math::Point<float> p) const {
	auto transform = m_transformPosMatrix * m_transformScaleMatrix; 
	return transform.TransformPoint(p); 
}

Math::Point<float> Direct2DRenderer::inv_transform_point(const Math::Point<float> p) const {
	auto transform = m_transformPosMatrix * m_transformScaleMatrix;
	transform.Invert();
	return transform.TransformPoint(p);
}

UINT Direct2DRenderer::get_dpi() const {
	return GetDpiForWindow(m_hwnd);
}

float Direct2DRenderer::get_dpi_scale() const {
	return 96.0f/get_dpi(); 
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
	ASSERT_WIN(result == S_OK, "Could not create brush!");
	obj.m_object = brush;

	return std::move(obj);
}

Direct2DRenderer::StrokeStyle Direct2DRenderer::create_stroke_style(std::vector<float> dashes) {
	StrokeStyle obj;
	ID2D1StrokeStyle* style = nullptr;

	auto props = D2D1::StrokeStyleProperties(
		D2D1_CAP_STYLE_FLAT,
		D2D1_CAP_STYLE_FLAT,
		D2D1_CAP_STYLE_ROUND,
		D2D1_LINE_JOIN_ROUND,
		10.0f,
		D2D1_DASH_STYLE_CUSTOM,
		0.0f);
	m_factory->CreateStrokeStyle(props, dashes.data(), static_cast<UINT32>(dashes.size()), &style);
	obj.m_object = style; 

	return std::move(obj); 
}

Direct2DRenderer::BrushObject Direct2DRenderer::create_brush(Renderer::AlphaColor c) {
	BrushObject obj;
	ID2D1SolidColorBrush* brush = nullptr;
	auto result = m_renderTarget->CreateSolidColorBrush(c, &brush);
	ASSERT_WIN(result == S_OK, "Could not create brush!");
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
	ASSERT_WITH_STATEMENT(hr == S_OK, goto REALEASE;, "Could not create WIC factory!");

	// Load the bitmap using WIC
	hr = pWICFactory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
	ASSERT_WITH_STATEMENT(hr == S_OK, goto REALEASE;, "Could not create WIC decoder!");

	hr = pDecoder->GetFrame(0, &pFrame); 
	ASSERT_WITH_STATEMENT(hr == S_OK, pDecoder->Release(); pWICFactory->Release(); goto REALEASE;, "Could not get frame from WIC decoder!");

	// Create converter 
	hr = pWICFactory->CreateFormatConverter(&pConverter);
	ASSERT_WITH_STATEMENT(hr == S_OK, goto REALEASE;, "Could not create WIC format converter!");
	hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeCustom);
	ASSERT_WITH_STATEMENT(hr == S_OK, goto REALEASE;, "Could not initialize WIC format converter!");

	// Convert the WIC bitmap to Direct2D bitmap
	hr = m_renderTarget->CreateBitmapFromWicBitmap(pConverter, nullptr, &pBitmap); 
	ASSERT_WITH_STATEMENT(hr == S_OK, goto REALEASE;, "Could not create Direct2D bitmap!");
	obj.m_object = pBitmap;

REALEASE:
	SafeRelease(pWICFactory); 
	SafeRelease(pDecoder);  
	SafeRelease(pFrame);  
	SafeRelease(pConverter); 

	return std::move(obj); 
}

Direct2DRenderer::BitmapObject Direct2DRenderer::create_bitmap(const byte* const data, Math::Rectangle<unsigned int> size, unsigned int stride, float dpi) {
	ID2D1Bitmap* pBitmap = nullptr;
	D2D1_BITMAP_PROPERTIES prop;

	prop.dpiX = dpi;
	prop.dpiY = dpi;
	prop.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	prop.pixelFormat.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	HRESULT res = m_renderTarget->CreateBitmap({ size.width, size.height }, data, stride, prop, &pBitmap);
	ASSERT_WIN(res == S_OK, "Could not create bitmap!"); 

	BitmapObject obj;
	obj.m_object = pBitmap;

	return std::move(obj);
}



Direct2DRenderer::PathObject Direct2DRenderer::create_bezier_path(const Renderer::CubicBezierGeometry& cub) {
	ID2D1PathGeometry* path = nullptr;
	ID2D1GeometrySink* sink = nullptr;
	auto res = m_factory->CreatePathGeometry(&path);
	ASSERT_WIN(res == S_OK, "Could not create path geometry!");
	path->Open(&sink);

	sink->BeginFigure(cub.points[0], D2D1_FIGURE_BEGIN_FILLED); 
	D2D1_BEZIER_SEGMENT segment;
	for (size_t i = 0; i < cub.points.size() - 1; i++) {
		segment.point1 = cub.control_points[0].at(i);
		segment.point2 = cub.control_points[1].at(i);
		segment.point3 = cub.points.at(i + 1);
		sink->AddBezier(segment); 
	}
	sink->EndFigure(D2D1_FIGURE_END_OPEN);
	sink->Close();

	SafeRelease(sink);
	return std::move(PathObject(path)); 
}

Direct2DRenderer::PathObject Direct2DRenderer::create_line_path(const std::vector<Math::Point<float>>& line) {
	ID2D1PathGeometry* path = nullptr;
	ID2D1GeometrySink* sink = nullptr;
	auto res = m_factory->CreatePathGeometry(&path);
	ASSERT_WIN(res == S_OK, "Could not create path geometry!");
	path->Open(&sink);

	sink->BeginFigure(line[0], D2D1_FIGURE_BEGIN_FILLED);
	// the cast is possible since the d2d1_point has the same memory layout as the line
	sink->AddLines(reinterpret_cast<const D2D1_POINT_2F*>(&line[1]), static_cast<UINT>(line.size() - 1));
	
	sink->EndFigure(D2D1_FIGURE_END_OPEN);
	sink->Close();

	SafeRelease(sink);
	return std::move(PathObject(path));
}
