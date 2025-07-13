#include "Direct2D.h"
#include <wincodec.h>
#include <d2d1.h>

#include "helper/AppVariables.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#undef min
#undef max

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
		
		if (!WIN_ASSERT_OK(result, "Could not initialize Direct2D")) {
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

	// create gpu rsc
	m_solid_brush = create_brush({});

	// set dpi and size of the target
	resize(m_window->get_client_size());
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

void DocantoWin::Direct2DRender::clear() {
	clear(AppVariables::Colors::get(AppVariables::Colors::TYPE::BACKGROUND_COLOR));
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
	m_solid_brush->SetColor(ColorToD2D1(c));
	draw_text(text, pos, format, m_solid_brush);
}

void DocantoWin::Direct2DRender::draw_rect(Docanto::Geometry::Rectangle<float> r, Docanto::Color c, float thic) {
	m_solid_brush->SetColor(ColorToD2D1(c));
	draw_rect(r, m_solid_brush, thic);
}
void DocantoWin::Direct2DRender::draw_rect(Docanto::Geometry::Rectangle<float> r, Docanto::Color c) {
	m_solid_brush->SetColor(ColorToD2D1(c));
	draw_rect(r, m_solid_brush, 3);
}

void DocantoWin::Direct2DRender::draw_rect(Docanto::Geometry::Rectangle<float> r, BrushObject& brush, float thic) {
	m_renderTarget->DrawRectangle(RectToD2D1(r), brush.m_object, thic);
}
void DocantoWin::Direct2DRender::draw_rect_filled(Docanto::Geometry::Rectangle<float> r, BrushObject& brush) {
	m_renderTarget->FillRectangle(RectToD2D1(r), brush.m_object);
}

void DocantoWin::Direct2DRender::draw_rect_filled(Docanto::Geometry::Rectangle<float> r, Docanto::Color c) {
	m_solid_brush->SetColor(ColorToD2D1(c));
	m_renderTarget->FillRectangle(RectToD2D1(r), m_solid_brush);
}

void DocantoWin::Direct2DRender::draw_bitmap(Docanto::Geometry::Point<float> where, BitmapObject& obj) {
	begin_draw();
	auto size = obj.m_object->GetSize();
	m_renderTarget->DrawBitmap(obj.m_object, D2D1::RectF(where.x, where.y, size.width + where.x , size.height + where.y));
	end_draw();
}

void DocantoWin::Direct2DRender::draw_bitmap(Docanto::Geometry::Rectangle<float> rec, BitmapObject& obj) {
	begin_draw();
	m_renderTarget->DrawBitmap(obj.m_object, RectToD2D1(rec));
	end_draw();
}

void DocantoWin::Direct2DRender::draw_bitmap(Docanto::Geometry::Point<float> where, BitmapObject& obj, float dpi) {
	float x = 0, y = 0;
	obj.m_object->GetDpi(&x, &y);
	auto scale = x / dpi;

	begin_draw();
	auto size = obj.m_object->GetSize();
	m_renderTarget->DrawBitmap(obj.m_object, D2D1::RectF(where.x, where.y, size.width * scale + where.x, size.height * scale + where.y));
	end_draw();
}

void DocantoWin::Direct2DRender::draw_line(Docanto::Geometry::Point<float> p1, Docanto::Geometry::Point<float> p2, BrushObject& brush, float thick) {
	begin_draw();
	m_renderTarget->DrawLine(PointToD2D1(p1), PointToD2D1(p2), brush.m_object, thick);
	end_draw();
}

void DocantoWin::Direct2DRender::draw_line(Docanto::Geometry::Point<float> p1, Docanto::Geometry::Point<float> p2, Docanto::Color c, float thick) {
	begin_draw();
	m_solid_brush->SetColor(ColorToD2D1(c));
	draw_line(p1, p2, m_solid_brush, thick);
	end_draw();
}


void DocantoWin::Direct2DRender::set_current_transform_active() {
	m_renderTarget->SetTransform(m_transformTranslationMatrix * m_transformRotationMatrix * m_transformScaleMatrix);
}

void DocantoWin::Direct2DRender::set_identity_transform_active() {
	m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}

void DocantoWin::Direct2DRender::set_translation_matrix(Docanto::Geometry::Point<float> p) {
	m_transformTranslationMatrix = D2D1::Matrix3x2F::Translation(p.x, p.y);
}

void DocantoWin::Direct2DRender::set_translation_matrix(D2D1::Matrix3x2F m) {
	m_transformTranslationMatrix = m;
}

void DocantoWin::Direct2DRender::add_translation_matrix(Docanto::Geometry::Point<float> p) {
	// in rad
	auto angle = get_angle();

	Docanto::Geometry::Point<float> new_pos;

	new_pos.x = p.x * std::cos(angle) + p.y * std::sin(angle);
	new_pos.y = -p.x * std::sin(angle) + p.y * std::cos(angle);

	new_pos = new_pos * 1/get_transform_scale();

	set_translation_matrix(D2D1::Matrix3x2F::Translation(new_pos.x, new_pos.y) * m_transformTranslationMatrix);
}

void DocantoWin::Direct2DRender::set_scale_matrix(float scale, Docanto::Geometry::Point<float> center) {
	m_transformScaleMatrix = D2D1::Matrix3x2F::Scale({ scale, scale }, PointToD2D1(center));
}

void DocantoWin::Direct2DRender::set_scale_matrix(D2D1::Matrix3x2F m) {
	m_transformScaleMatrix = m;
}

void DocantoWin::Direct2DRender::add_scale_matrix(float scale, Docanto::Geometry::Point<float> center) {
	auto full = m_transformScaleMatrix;
	full.Invert();

	auto pivot = full.TransformPoint(PointToD2D1(center));


	auto new_scale_mat = D2D1::Matrix3x2F::Scale(scale, scale, pivot);

	set_scale_matrix(new_scale_mat * m_transformScaleMatrix);
}

void DocantoWin::Direct2DRender::set_rotation_matrix(float angle, Docanto::Geometry::Point<float> center) {
	m_transformRotationMatrix = D2D1::Matrix3x2F::Rotation(angle, PointToD2D1(center));
}

void DocantoWin::Direct2DRender::set_rotation_matrix(D2D1::Matrix3x2F m) {
	m_transformRotationMatrix = m;
}

void DocantoWin::Direct2DRender::add_rotation_matrix(float angle, Docanto::Geometry::Point<float> center) {    
	auto full = m_transformRotationMatrix * m_transformScaleMatrix;
	full.Invert();

	auto localPivot = full.TransformPoint(PointToD2D1(center));

	auto rotAbout = D2D1::Matrix3x2F::Rotation(
		angle,
		localPivot);

	set_rotation_matrix(rotAbout * m_transformRotationMatrix);
}

Docanto::Geometry::Point<float> DocantoWin::Direct2DRender::inv_transform(Docanto::Geometry::Point<float> p) {
	auto full = m_transformTranslationMatrix
		* m_transformRotationMatrix
		* m_transformScaleMatrix;
	full.Invert();

	auto localPivot = full.TransformPoint(PointToD2D1(p));

	return { localPivot.x, localPivot.y };
}

Docanto::Geometry::Point<float> DocantoWin::Direct2DRender::transform(Docanto::Geometry::Point<float> p) {
	auto local = (m_transformTranslationMatrix 
		* m_transformRotationMatrix 
		* m_transformScaleMatrix).TransformPoint(PointToD2D1(p));

	return { local.x, local.y };
}

Docanto::Geometry::Rectangle<float> DocantoWin::Direct2DRender::transform(Docanto::Geometry::Rectangle<float> r) {
	auto a = r.upperleft();
	auto b = r.lowerright();

	a = transform(a);
	b = transform(b);

	return Docanto::Geometry::Rectangle<float>(a, b);
}

Docanto::Geometry::Rectangle<float> DocantoWin::Direct2DRender::inv_transform(Docanto::Geometry::Rectangle<float> r) {
	auto a = r.upperleft();
	auto b = r.upperright();
	auto c = r.lowerleft();
	auto d = r.lowerright();

	a = inv_transform(a);
	b = inv_transform(b);
	c = inv_transform(c);
	d = inv_transform(d);

	Docanto::Geometry::Point<float> lefttop;
	lefttop.x = std::min({ a.x,b.x,c.x,d.x });
	lefttop.y = std::min({ a.y,b.y,c.y,d.y });

	Docanto::Geometry::Point<float> rightbottom;
	rightbottom.x = std::max({ a.x,b.x,c.x,d.x });
	rightbottom.y = std::max({ a.y,b.y,c.y,d.y });

	return Docanto::Geometry::Rectangle<float>(lefttop, rightbottom);
}

float DocantoWin::Direct2DRender::get_transform_scale() const {
	return m_transformScaleMatrix._11;
}

D2D1::Matrix3x2F DocantoWin::Direct2DRender::get_scale_matrix() const {
	return m_transformScaleMatrix;
}

D2D1::Matrix3x2F DocantoWin::Direct2DRender::get_rotation_matrix() const {
	return m_transformRotationMatrix;
}

D2D1::Matrix3x2F DocantoWin::Direct2DRender::get_transformation_matrix() const {
	return m_transformTranslationMatrix;
}

Docanto::Geometry::Point<float> DocantoWin::Direct2DRender::get_zoom_center() const {
	if (FLOAT_EQUAL(m_transformScaleMatrix._11, 1)) {
		return { m_transformScaleMatrix._31, m_transformScaleMatrix._32 };
	}
	return { m_transformScaleMatrix._31 / (1 - m_transformScaleMatrix._11),
		m_transformScaleMatrix._32 / (1 - m_transformScaleMatrix._11), };
}

Docanto::Geometry::Point<float> DocantoWin::Direct2DRender::get_transform_pos() const {
	return { m_transformTranslationMatrix._31, m_transformTranslationMatrix._32 };
}

float DocantoWin::Direct2DRender::get_angle() const {
	return -std::atan2(m_transformRotationMatrix._21, m_transformRotationMatrix._11);
}

float DocantoWin::Direct2DRender::get_dpi() {
	return get_attached_window()->get_dpi();
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

DocantoWin::Direct2DRender::BitmapObject DocantoWin::Direct2DRender::create_bitmap(const Docanto::Image& i) {
	ID2D1Bitmap* pBitmap = nullptr;
	D2D1_BITMAP_PROPERTIES prop;

	prop.dpiX = static_cast<float>(i.dpi);
	prop.dpiY = static_cast<float>(i.dpi);
	prop.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	prop.pixelFormat.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	HRESULT res = m_renderTarget->CreateBitmap(DimensionToD2D1(i.dims), i.data.get(), i.stride, prop, &pBitmap);
	if (res != S_OK) {
		Docanto::Logger::error("Could not create bitmap");
	}

	BitmapObject obj;
	obj.m_object = pBitmap;

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
