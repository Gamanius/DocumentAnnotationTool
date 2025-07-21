#include "Direct2D.h"
#include <wincodec.h>
#include <d2d1.h>

#include <winrt\Windows.UI.Composition.Desktop.h>
#include <winrt\Windows.Graphics.Effects.h>
#include <winrt\Windows.UI.Composition.h>
#include <windows.ui.composition.interop.h>

#include "winrt\Microsoft.Graphics.Canvas.Effects.h"

#include <DispatcherQueue.h>
#include <d2d1_3.h>
#include <d3d11_3.h>
#include <wrl.h>

#include "helper/AppVariables.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#undef min
#undef max

winrt::com_ptr<ID2D1Device>         m_d2dDevice  = nullptr;
winrt::com_ptr<ID2D1Factory1>       m_d2dFactory = nullptr;
winrt::com_ptr<IDWriteFactory3>     m_d2dwriteFactory = nullptr;
winrt::com_ptr<ID2D1Bitmap1>        m_d2dBitmap  = nullptr;
winrt::com_ptr<IWICImagingFactory2> m_wicFactory = nullptr;

winrt::com_ptr<ID3D11Device>        m_d3dDevice  = nullptr;


winrt::com_ptr<ID2D1DeviceContext> temp_d2dDeviceContext;
winrt::com_ptr<ID2D1DeviceContext> d2dContext;

winrt::Windows::System::DispatcherQueueController m_dispatcherQueueController = nullptr;

winrt::com_ptr<winrt::Windows::UI::Composition::ICompositionGraphicsDevice> m_graphicsDevice;
winrt::com_ptr<ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop> m_surfaceInterop;
winrt::Windows::UI::Composition::CompositionVirtualDrawingSurface m_virtualSurface = nullptr;
winrt::Windows::UI::Composition::CompositionSurfaceBrush m_surfaceBrush = nullptr;

winrt::Windows::UI::Composition::Compositor m_compositor = nullptr;

winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget m_target = nullptr;

bool DocantoWin::Direct2DRender::createD2DResources() {
	D2D1_FACTORY_OPTIONS option{};

#ifdef _DEBUG
	option.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#else
	option.debugLevel = D2D1_DEBUG_LEVEL_NONE;
#endif // _DEBUG

	HRESULT result = S_OK;
	if (m_d2dFactory == nullptr) {
		result = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, option, m_d2dFactory.put());

		if (!WIN_ASSERT_OK(result, "Could not initialize Direct2D")) {
			return false;
		}
	}


	// create some text rendering
	if (m_d2dwriteFactory == nullptr) {
		result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3), (IUnknown**)m_d2dwriteFactory.put_void());
		if (result != S_OK) {
			Docanto::Logger::error("Could not initilaize text rendering");
			return false;
		}
	}

	if (m_d3dDevice == nullptr) {
		result = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			D3D11_CREATE_DEVICE_BGRA_SUPPORT,
			nullptr, 0,
			D3D11_SDK_VERSION,
			m_d3dDevice.put(),
			nullptr,
			nullptr);

		if (result != S_OK) {
			return false;
		}
	}

	namespace abi = ABI::Windows::UI::Composition;

	winrt::com_ptr<IDXGIDevice> const dxdevice = m_d3dDevice.as<IDXGIDevice>();
	winrt::com_ptr<abi::ICompositorInterop> interopCompositor = m_compositor.as<abi::ICompositorInterop>();


	m_d2dFactory->CreateDevice(dxdevice.get(), m_d2dDevice.put());
	m_d2dDevice->CreateDeviceContext(
		D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
		d2dContext.put());

	interopCompositor->CreateGraphicsDevice(m_d2dDevice.get(), reinterpret_cast<abi::ICompositionGraphicsDevice**>(winrt::put_abi(m_graphicsDevice)));
	result = CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, __uuidof(m_wicFactory), m_wicFactory.put_void());
	WIN_ASSERT_OK(result, "CoCreateInstance");

	return true;

}

bool DocantoWin::Direct2DRender::initWinrt() {
	if (m_dispatcherQueueController == nullptr) {
		DispatcherQueueOptions options
		{
			sizeof(DispatcherQueueOptions),
			DQTYPE_THREAD_CURRENT,
			DQTAT_COM_ASTA
		};

		winrt::Windows::System::DispatcherQueueController controller{ nullptr };
		auto result = CreateDispatcherQueueController(options, reinterpret_cast<ABI::Windows::System::IDispatcherQueueController**>(winrt::put_abi(controller)));
		m_dispatcherQueueController = controller;

		if (result != S_OK) {
			return false;
		}
	}
	return true;
}

void DocantoWin::Direct2DRender::createBlur() {
	using namespace winrt;
	using namespace winrt::Windows::UI;
	using namespace winrt::Windows::UI::Composition;
	using namespace winrt::Windows::UI::Composition::Desktop;

	auto root = m_target.Root().as<SpriteVisual>();
	if (root) {
		using namespace winrt::Microsoft::Graphics::Canvas;

		// GaussianBlurEffect only
		auto blur = Effects::GaussianBlurEffect();
		blur.Name(L"Blur");
		blur.BorderMode(Effects::EffectBorderMode::Hard);
		blur.BlurAmount(5.f); // Adjust blur intensity
		blur.Source(CompositionEffectSourceParameter(L"Backdrop"));

		// Create blur brush
		auto blurBrush = m_compositor.CreateEffectFactory(blur).CreateBrush();
		blurBrush.SetSourceParameter(L"Backdrop", m_compositor.CreateBackdropBrush());

		// Apply blur brush directly to the root visual
		root.Brush(blurBrush);

	}
}

DocantoWin::Direct2DRender::Direct2DRender(std::shared_ptr<Window> w) : m_window(w) {

	namespace abi = ABI::Windows::UI::Composition::Desktop;

	using namespace winrt;
	using namespace winrt::Windows::UI;
	using namespace winrt::Windows::UI::Composition;
	using namespace winrt::Windows::UI::Composition::Desktop;
	using namespace winrt::Microsoft::Graphics::Canvas;
	

	this->initWinrt();
	m_compositor = Compositor();
	auto interop = m_compositor.as<ABI::Windows::UI::Composition::Desktop::ICompositorDesktopInterop >();
	winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget target{ nullptr };
	check_hresult(interop->CreateDesktopWindowTarget(m_window->get_hwnd(), false, reinterpret_cast<ABI::Windows::UI::Composition::Desktop::IDesktopWindowTarget**>(winrt::put_abi(target))));
	m_target = target;

	this->createD2DResources();

	auto graphicsDevice2 = m_graphicsDevice.as<ICompositionGraphicsDevice2>();

	m_virtualSurface = graphicsDevice2.CreateVirtualDrawingSurface(
		{ 256, 256 },
		winrt::Windows::Graphics::DirectX::DirectXPixelFormat::R16G16B16A16Float,
		winrt::Windows::Graphics::DirectX::DirectXAlphaMode::Premultiplied);

	m_surfaceInterop = m_virtualSurface.as<ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop>();

	ICompositionSurface surface = m_surfaceInterop.as<ICompositionSurface>();

	m_surfaceBrush = m_compositor.CreateSurfaceBrush(surface);
	auto backdropVisual = m_compositor.CreateSpriteVisual();
	backdropVisual.RelativeSizeAdjustment({ 1.0f, 1.0f }); // Fill parent

	// GaussianBlurEffect
	auto blur = Effects::GaussianBlurEffect();
	blur.Name(L"Blur");
	blur.BorderMode(Effects::EffectBorderMode::Hard);
	blur.BlurAmount(5.f); // Adjust blur intensity
	blur.Source(CompositionEffectSourceParameter(L"Backdrop")); // Sample what's behind this visual

	// Create blur brush
	auto blurEffectFactory = m_compositor.CreateEffectFactory(blur);
	auto blurBrush = blurEffectFactory.CreateBrush();
	blurBrush.SetSourceParameter(L"Backdrop", m_compositor.CreateBackdropBrush()); // Use backdrop from the window

	// Apply blur brush to the backdrop visual
	backdropVisual.Brush(blurBrush);

	// --- 2. Create the visual for your Direct2D content ---
	// This is typically what your `m_target.Root()` refers to,
	// or you can create a new visual heirarchy here.
	// Let's assume m_rootVisual is your main content visual (which uses m_surfaceBrush)
	auto m_rootVisual = m_compositor.CreateSpriteVisual();
	m_rootVisual.RelativeSizeAdjustment({ 1.0f, 1.0f });

	// Assign your Direct2D surface brush to this visual
	m_rootVisual.Brush(m_surfaceBrush);

	// --- 3. Manage the composition tree ---
	// The m_target.Root() will now be a container visual for these two.
	// Or, you can set backdropVisual directly as m_target.Root()
	// and then add m_rootVisual as a child of backdropVisual.

	// Option A: Use the existing root, add backdrop as a child behind D2D content
	// This assumes m_target.Root() is already a ContainerVisual or you set it up to be.
	// For simplicity, let's create a new root that holds both:
	auto mainContainer = m_compositor.CreateContainerVisual();
	mainContainer.RelativeSizeAdjustment({ 1.0f, 1.0f });
	mainContainer.Children().InsertAtTop(m_rootVisual);    // D2D content is on top
	mainContainer.Children().InsertAtTop(backdropVisual); // Blur is at the bottom

	m_target.Root(mainContainer);

	// create gpu rsc
	m_solid_brush = create_brush({});

	// set dpi and size of the target
	resize(m_window->get_client_size());
}

DocantoWin::Direct2DRender::~Direct2DRender() {
}

void DocantoWin::Direct2DRender::begin_draw() {
	std::lock_guard lock(draw_lock);
	if (m_isRenderinProgress == 0) {
			POINT offset = { 0,0 };
			m_surfaceInterop->BeginDraw(nullptr, __uuidof(ID2D1DeviceContext), (void**)temp_d2dDeviceContext.put(), &offset);
	}
	m_isRenderinProgress++;
}

void DocantoWin::Direct2DRender::end_draw() {
	std::lock_guard lock(draw_lock);
	m_isRenderinProgress--;
	if (m_isRenderinProgress == 0)
		m_surfaceInterop->EndDraw();
}

void DocantoWin::Direct2DRender::clear(Docanto::Color c) {
	begin_draw();
	temp_d2dDeviceContext->Clear(ColorToD2D1(c));
	end_draw();
}

void DocantoWin::Direct2DRender::clear() {
	clear(AppVariables::Colors::get(AppVariables::Colors::TYPE::BACKGROUND_COLOR));
}

std::shared_ptr<DocantoWin::Window> DocantoWin::Direct2DRender::get_attached_window() const {
	return m_window;
}

void DocantoWin::Direct2DRender::resize(Docanto::Geometry::Dimension<long> r) {
	//m_renderTarget->Resize(DimensionToD2D1(r));

	// we should also check if the dpi changed
	UINT dpiX, dpiY;
	dpiX = dpiY = GetDpiForWindow(m_window->m_hwnd);
	//m_d2dDevice->SetDpi(static_cast<float>(dpiX), static_cast<float>(dpiY));
}

void DocantoWin::Direct2DRender::draw_text(const std::wstring& text, Docanto::Geometry::Point<float> pos, TextFormatObject& format, BrushObject& brush) {
	auto layout = create_text_layout(text, format);

	begin_draw();

	temp_d2dDeviceContext->DrawTextLayout(
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
	temp_d2dDeviceContext->DrawRectangle(RectToD2D1(r), brush.m_object, thic);
}
void DocantoWin::Direct2DRender::draw_rect_filled(Docanto::Geometry::Rectangle<float> r, BrushObject& brush) {
	temp_d2dDeviceContext->FillRectangle(RectToD2D1(r), brush.m_object);
}

void DocantoWin::Direct2DRender::draw_rect_filled(Docanto::Geometry::Rectangle<float> r, Docanto::Color c) {
	m_solid_brush->SetColor(ColorToD2D1(c));
	temp_d2dDeviceContext->FillRectangle(RectToD2D1(r), m_solid_brush);
}

void DocantoWin::Direct2DRender::draw_bitmap(Docanto::Geometry::Point<float> where, BitmapObject& obj) {
	begin_draw();
	auto size = obj.m_object->GetSize();
	temp_d2dDeviceContext->DrawBitmap(obj.m_object, D2D1::RectF(where.x, where.y, size.width + where.x , size.height + where.y));
	end_draw();
}

void DocantoWin::Direct2DRender::draw_bitmap(Docanto::Geometry::Rectangle<float> rec, BitmapObject& obj) {
	begin_draw();
	temp_d2dDeviceContext->DrawBitmap(obj.m_object, RectToD2D1(rec));
	end_draw();
}

void DocantoWin::Direct2DRender::draw_bitmap(Docanto::Geometry::Point<float> where, BitmapObject& obj, float dpi) {
	float x = 0, y = 0;
	obj.m_object->GetDpi(&x, &y);
	auto scale = x / dpi;

	begin_draw();
	auto size = obj.m_object->GetSize();
	temp_d2dDeviceContext->DrawBitmap(obj.m_object, D2D1::RectF(where.x, where.y, size.width * scale + where.x, size.height * scale + where.y));
	end_draw();
}

void DocantoWin::Direct2DRender::draw_line(Docanto::Geometry::Point<float> p1, Docanto::Geometry::Point<float> p2, BrushObject& brush, float thick) {
	begin_draw();
	temp_d2dDeviceContext->DrawLine(PointToD2D1(p1), PointToD2D1(p2), brush.m_object, thick);
	end_draw();
}

void DocantoWin::Direct2DRender::draw_line(Docanto::Geometry::Point<float> p1, Docanto::Geometry::Point<float> p2, Docanto::Color c, float thick) {
	begin_draw();
	m_solid_brush->SetColor(ColorToD2D1(c));
	draw_line(p1, p2, m_solid_brush, thick);
	end_draw();
}


void DocantoWin::Direct2DRender::set_current_transform_active() {
	temp_d2dDeviceContext->SetTransform(m_transformTranslationMatrix * m_transformRotationMatrix * m_transformScaleMatrix);
}

void DocantoWin::Direct2DRender::set_identity_transform_active() {
	temp_d2dDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
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

	HRESULT result = m_d2dwriteFactory->CreateTextFormat(
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
	auto result = m_d2dwriteFactory->CreateTextLayout(
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
	HRESULT res = d2dContext->CreateBitmap(DimensionToD2D1(i.dims), i.data.get(), i.stride, prop, &pBitmap);
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
	auto result = d2dContext->CreateSolidColorBrush(ColorToD2D1(c), &brush);
	if (result != S_OK) {
		Docanto::Logger::error("Could not create D2D1 brush");
	}
	obj.m_object = brush;

	return std::move(obj);
}
