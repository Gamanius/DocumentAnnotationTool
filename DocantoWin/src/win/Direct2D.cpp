#include "Direct2D.h"
#include <wincodec.h>

#include "helper/AppVariables.h"

#undef min
#undef max

ID2D1Factory6* DocantoWin::Direct2DRender::m_factory = nullptr;
IDWriteFactory3* DocantoWin::Direct2DRender::m_writeFactory = nullptr;

void DocantoWin::Direct2DRender::createD2DResources() {
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
}

DocantoWin::Direct2DRender::Direct2DRender(std::shared_ptr<Window> w) : m_window(w) {
	createD2DResources();
	
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// Create the DX11 API device object, and get a corresponding context.
	ComPtr<ID3D11DeviceContext> d3d11context; 
	ComPtr<IDXGIDevice> dxdevice;

	auto res = D3D11CreateDevice(
		nullptr,                    // specify null to use the default adapter
		D3D_DRIVER_TYPE_HARDWARE,
		0,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,              // optionally set debug and Direct2D compatibility flags
		featureLevels,              // list of feature levels this app can support
		ARRAYSIZE(featureLevels),   // number of possible feature levels
		D3D11_SDK_VERSION,
		&m_d3d11device,                    // returns the Direct3D device created
		nullptr,            // returns feature level of device created
		&d3d11context                    // returns the device immediate context
	);

	if (res != S_OK) {
		Docanto::Logger::error("Couldn't init D3D11");
		return;
	}

	res = m_d3d11device.As(&dxdevice);
	if (res != S_OK) {
		Docanto::Logger::error("Couldn't get DX Adapter");
		return;
	}

	res = m_factory->CreateDevice(dxdevice.Get(), &m_device);
	if (res != S_OK) {
		Docanto::Logger::error("Couldn't get Direct2D Device");
		return;
	}

	res = m_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, &m_devicecontext);
	if (res != S_OK) {
		Docanto::Logger::error("Couldn't get Direct2D device context");
		return;
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	swapChainDesc.Width = 0;                           // dont need to set these since they are computed automatically
	swapChainDesc.Height = 0;
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // this is the most common swapchain format
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;                // don't use multi-sampling
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;                     // use double buffering to enable flip
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // all apps must use this SwapEffect
	swapChainDesc.Flags = 0;

	ComPtr<IDXGIAdapter> dxgiAdapter;
	dxdevice->GetAdapter(&dxgiAdapter);
	ComPtr<IDXGIFactory2> dxgiFactory;
	dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

	res = dxgiFactory->CreateSwapChainForHwnd(
		m_d3d11device.Get(), m_window->get_hwnd(), &swapChainDesc, nullptr, nullptr, &m_swapChain);
	if (res != S_OK) {
		Docanto::Logger::error("Couldn't create swap chain");
		return;
	}

	auto dpi = m_window->get_dpi();
	D2D1_BITMAP_PROPERTIES1 bmpProps = {};
	bmpProps.pixelFormat = { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED };
	bmpProps.dpiX = dpi;
	bmpProps.dpiY = dpi;
	bmpProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

	ComPtr<IDXGISurface> backBuffer;
	res = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	if (res != S_OK) {
		Docanto::Logger::error("Couldn't get buffer");
		return;
	}
	res = m_devicecontext->CreateBitmapFromDxgiSurface(backBuffer.Get(), &bmpProps, &m_targetBitmap	);
	if (res != S_OK) {
		Docanto::Logger::error("Couldn't create Direct2D drawing surface");
		return;
	}

	m_devicecontext->SetTarget(m_targetBitmap.Get());

	// create gpu rsc
	m_solid_brush = create_brush({});

	// set dpi and size of the target
	//resize(m_window->get_client_size());
	m_devicecontext->SetDpi(static_cast<float>(dpi), static_cast<float>(dpi));
}

DocantoWin::Direct2DRender::~Direct2DRender() {
	SafeRelease(m_factory);
	SafeRelease(m_writeFactory);
}

void DocantoWin::Direct2DRender::begin_draw() {
	std::lock_guard lock(draw_lock);
	if (m_isRenderinProgress == 0)
		m_devicecontext->BeginDraw();
	m_isRenderinProgress++;
}

void DocantoWin::Direct2DRender::end_draw() {
	std::lock_guard lock(draw_lock);
	m_isRenderinProgress--;
	if (m_isRenderinProgress == 0) {
		m_devicecontext->EndDraw();

		DXGI_PRESENT_PARAMETERS params = { 0 };
		m_swapChain->Present1(1, 0, &params);
	}
}

void DocantoWin::Direct2DRender::clear(Docanto::Color c) {
	begin_draw();
	m_devicecontext->Clear(ColorToD2D1(c));
	end_draw();
}

void DocantoWin::Direct2DRender::clear() {
	clear(AppVariables::Colors::get(AppVariables::Colors::TYPE::BACKGROUND_COLOR));
}

std::shared_ptr<DocantoWin::Window> DocantoWin::Direct2DRender::get_attached_window() const {
	return m_window;
}

void DocantoWin::Direct2DRender::resize(Docanto::Geometry::Dimension<long> r) {
	m_devicecontext->SetTarget(nullptr);
	m_targetBitmap.Reset();
	m_swapChain->ResizeBuffers(0, r.width, r.height, DXGI_FORMAT_UNKNOWN, 0);
	ComPtr<IDXGISurface> dxgiSurface;
	m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiSurface));


	auto dpi = m_window->get_dpi();
	D2D1_BITMAP_PROPERTIES1 props = {
	{ DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
	dpi, dpi,
	D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW
	};

	m_devicecontext->CreateBitmapFromDxgiSurface(dxgiSurface.Get(), &props, &m_targetBitmap);

	// 5. Set the new target
	m_devicecontext->SetTarget(m_targetBitmap.Get());
	//m_device->Resize(DimensionToD2D1(r));

	// we should also check if the dpi changed
	m_devicecontext->SetDpi(static_cast<float>(dpi), static_cast<float>(dpi));
}

void DocantoWin::Direct2DRender::draw_text(const std::wstring& text, Docanto::Geometry::Point<float> pos, TextFormatObject& format, BrushObject& brush) {
	auto layout = create_text_layout(text, format);

	begin_draw();

	m_devicecontext->DrawTextLayout(
		PointToD2D1(pos),
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
	m_devicecontext->DrawRectangle(RectToD2D1(r), brush.m_object, thic);
}
void DocantoWin::Direct2DRender::draw_rect_filled(Docanto::Geometry::Rectangle<float> r, BrushObject& brush) {
	m_devicecontext->FillRectangle(RectToD2D1(r), brush.m_object);
}

void DocantoWin::Direct2DRender::draw_rect_filled(Docanto::Geometry::Rectangle<float> r, Docanto::Color c) {
	m_solid_brush->SetColor(ColorToD2D1(c));
	m_devicecontext->FillRectangle(RectToD2D1(r), m_solid_brush);
}

void DocantoWin::Direct2DRender::draw_bitmap(Docanto::Geometry::Point<float> where, BitmapObject& obj) {
	begin_draw();
	auto size = obj.m_object->GetSize();
	m_devicecontext->DrawBitmap(obj.m_object, D2D1::RectF(where.x, where.y, size.width + where.x , size.height + where.y));
	end_draw();
}

void DocantoWin::Direct2DRender::draw_bitmap(Docanto::Geometry::Rectangle<float> rec, BitmapObject& obj) {
	begin_draw();
	m_devicecontext->DrawBitmap(obj.m_object, RectToD2D1(rec));
	end_draw();
}

void DocantoWin::Direct2DRender::draw_bitmap(Docanto::Geometry::Point<float> where, BitmapObject& obj, float dpi) {
	float x = 0, y = 0;
	obj.m_object->GetDpi(&x, &y);
	auto scale = x / dpi;

	begin_draw();
	auto size = obj.m_object->GetSize();
	m_devicecontext->DrawBitmap(obj.m_object, D2D1::RectF(where.x, where.y, size.width * scale + where.x, size.height * scale + where.y));
	end_draw();
}

void DocantoWin::Direct2DRender::draw_svg(int id, Docanto::Geometry::Point<float> where, Docanto::Color c, Docanto::Geometry::Dimension<float> size) {
	const size_t svg_default_size = 800; //px
	if (!m_cached_svgs.contains(id)) {
		m_cached_svgs[id] = create_svg(id);
	}

	auto& svg = m_cached_svgs[id];
	float scale_x = size.width / svg_default_size;
	float scale_y = size.height / svg_default_size;

	ComPtr<ID2D1SvgElement> root;
	svg->GetRoot(&root);
	auto col = ColorToD2D1(c);
	root->SetAttributeValue(L"stroke", col);

	begin_draw();
	D2D1_MATRIX_3X2_F last_transform;
	m_devicecontext->GetTransform(&last_transform);

	// this code will break so damn fast if the last transform isnt guaranteed to be a transformation
	// i'm keeping it this way since drawing svg is only for UI elements which do not get scaled (for now)
	D2D1_MATRIX_3X2_F transform =
		D2D1::Matrix3x2F::Scale(scale_x, scale_y) *
		last_transform *
		D2D1::Matrix3x2F::Translation(where.x, where.y);
	m_devicecontext->SetTransform(transform);

	m_devicecontext->DrawSvgDocument(svg);

	m_devicecontext->SetTransform(last_transform);

	end_draw();
}

void DocantoWin::Direct2DRender::draw_line(Docanto::Geometry::Point<float> p1, Docanto::Geometry::Point<float> p2, BrushObject& brush, float thick) {
	begin_draw();
	m_devicecontext->DrawLine(PointToD2D1(p1), PointToD2D1(p2), brush.m_object, thick);
	end_draw();
}

void DocantoWin::Direct2DRender::draw_line(Docanto::Geometry::Point<float> p1, Docanto::Geometry::Point<float> p2, Docanto::Color c, float thick) {
	begin_draw();
	m_solid_brush->SetColor(ColorToD2D1(c));
	draw_line(p1, p2, m_solid_brush, thick);
	end_draw();
}


void DocantoWin::Direct2DRender::set_current_transform_active() {
	m_devicecontext->SetTransform(m_transformTranslationMatrix * m_transformRotationMatrix * m_transformScaleMatrix);
}

void DocantoWin::Direct2DRender::set_identity_transform_active() {
	m_devicecontext->SetTransform(D2D1::Matrix3x2F::Identity());
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

void DocantoWin::Direct2DRender::set_clipping_rect(Docanto::Geometry::Rectangle<float> clip) {
	m_devicecontext->PushAxisAlignedClip(RectToD2D1(clip), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
}

void DocantoWin::Direct2DRender::push_clipping_rect_to_origin(Docanto::Geometry::Rectangle<float> clip) {
	m_devicecontext->SetTransform(D2D1::Matrix3x2F::Translation({ clip.x, clip.y }));
	m_devicecontext->PushAxisAlignedClip({0,0, clip.width, clip.height}, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
}

void DocantoWin::Direct2DRender::pop_clipping_rect() {
	m_devicecontext->PopAxisAlignedClip();
}

void DocantoWin::Direct2DRender::pop_clipping_rect_to_origin() {
	set_identity_transform_active();
	m_devicecontext->PopAxisAlignedClip();
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
	HRESULT res = m_devicecontext->CreateBitmap(DimensionToD2D1(i.dims), i.data.get(), i.stride, prop, &pBitmap);
	if (res != S_OK) {
		Docanto::Logger::error("Could not create bitmap");
	}

	BitmapObject obj;
	obj.m_object = pBitmap;

	return std::move(obj);
}

DocantoWin::Direct2DRender::SVGDocument DocantoWin::Direct2DRender::create_svg(int id, const std::wstring& type) {
	HRSRC res = FindResource(NULL, MAKEINTRESOURCE(id), type.c_str());
	if (res == nullptr) {
		Docanto::Logger::error("Couln't find SVG with id ", id); 
		return SVGDocument();
	}
	HGLOBAL resData = LoadResource(NULL, res);
	if (resData == nullptr) {
		Docanto::Logger::error("Couldn't load resource with id ", id); 
		return SVGDocument();
	}

	DWORD size = SizeofResource(NULL, res);
	void* ptr = LockResource(resData);

	ComPtr<IStream> stream;
	HRESULT hr = CreateStreamOnHGlobal(nullptr, TRUE, &stream);
	if (FAILED(hr)) {
		Docanto::Logger::error("Failed to create stream for SVG id ", id);
		return SVGDocument();
	}

	// Write the resource data to the stream
	ULONG written;
	hr = stream->Write(ptr, size, &written);
	if (FAILED(hr) || written != size) {
		Docanto::Logger::error("Failed to write SVG data to stream for id ", id);
		return SVGDocument();
	}

	// Reset stream position
	LARGE_INTEGER li = {};
	stream->Seek(li, STREAM_SEEK_SET, nullptr);

	SVGDocument svg;
	hr = m_devicecontext->CreateSvgDocument(stream.Get(), D2D1::SizeF(1, 1), &svg.m_object);
	if (FAILED(hr)) {
		Docanto::Logger::error("Failed to create SVG document for id ", id);
		return SVGDocument();
	}

	return svg;
}

DocantoWin::Direct2DRender::BrushObject DocantoWin::Direct2DRender::create_brush(Docanto::Color c) {
	BrushObject obj;
	ID2D1SolidColorBrush* brush = nullptr;
	auto result = m_devicecontext->CreateSolidColorBrush(ColorToD2D1(c), &brush);
	if (result != S_OK) {
		Docanto::Logger::error("Could not create D2D1 brush");
	}
	obj.m_object = brush;

	return std::move(obj);
}
