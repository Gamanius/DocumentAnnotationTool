#pragma once
#include "../General/General.h" 
#include "WindowHandler.h"
#include "../PDF/MuPDF.h"
#include "json.hpp"

// Direct2D
#include <d2d1.h>
#include <dwrite_3.h>
#include <Windows.h>

#ifndef _DIRECT_2D_H_
#define _DIRECT_2D_H_

inline void SafeRelease(IUnknown* ptr) {
	if (ptr) {
		ptr->Release();
		ptr = nullptr;
	}
}

class Direct2DRenderer {
	static ID2D1Factory* m_factory;
	static IDWriteFactory3* m_writeFactory;

	HDC m_hdc = nullptr;
	HWND m_hwnd = nullptr;
	Math::Rectangle<long> m_window_size;
	ID2D1HwndRenderTarget* m_renderTarget = nullptr;

	D2D1::Matrix3x2F m_transformPosMatrix = D2D1::Matrix3x2F::Identity(), m_transformScaleMatrix = D2D1::Matrix3x2F::Identity();

	std::atomic<UINT32> m_isRenderinProgress = 0;
	std::mutex draw_lock;

public:
	template <typename T>
	struct RenderObject {
		T* m_object = nullptr;

		RenderObject() = default;
		RenderObject(T* object) : m_object(object) {}
		RenderObject(RenderObject&& t) noexcept {
			m_object = t.m_object;
			t.m_object = nullptr;
		}
		RenderObject(const RenderObject& t) {
			m_object = t.m_object;
			m_object->AddRef();
		}

		RenderObject& operator=(const RenderObject& t) {
			if (this != &t) {
				m_object = t.m_object;
				m_object->AddRef();
			}
			return *this;
		}

		RenderObject& operator=(RenderObject&& t) noexcept {
			if (this != &t) {
				m_object = t.m_object;
				t.m_object = nullptr;
			}
			return *this;
		}

		~RenderObject() {
			SafeRelease(m_object);
		}
	};

	typedef RenderObject<ID2D1SolidColorBrush> BrushObject;
	typedef RenderObject<ID2D1Bitmap> BitmapObject;
	typedef RenderObject<IDWriteTextFormat> TextFormatObject;
	typedef RenderObject<ID2D1PathGeometry> PathObject;

	enum INTERPOLATION_MODE {
		NEAREST_NEIGHBOR,
		LINEAR
	};

	Direct2DRenderer(const WindowHandler& window);
	~Direct2DRenderer();

	void begin_draw();
	void end_draw();
	void clear(Renderer::Color c);

	void resize(Math::Rectangle<long> r);

	void draw_bitmap(BitmapObject& bitmap, Math::Point<float> pos, INTERPOLATION_MODE mode = INTERPOLATION_MODE::NEAREST_NEIGHBOR, float opacity = 1.0f);
	void draw_bitmap(BitmapObject& bitmap, Math::Rectangle<float> dest, INTERPOLATION_MODE mode = INTERPOLATION_MODE::NEAREST_NEIGHBOR, float opacity = 1.0f);

	void draw_text(const std::wstring& text, Math::Point<float> pos, TextFormatObject& format, BrushObject& brush);
	void draw_text(const std::wstring& text, Math::Point<float> pos, Renderer::Color c, float size);

	void draw_line(Math::Point<float> p1, Math::Point<float> p2, BrushObject& brush, float thick);
	void draw_line(Math::Point<float> p1, Math::Point<float> p2, Renderer::Color c, float thick);
	void draw_line(Math::Point<float> p1, Math::Point<float> p2, Renderer::AlphaColor c, float thick);

	void draw_path(PathObject& obj, BrushObject& brush, float thick);
	void draw_path(PathObject& obj, Renderer::Color c, float thick);
	void draw_path(PathObject& obj, Renderer::AlphaColor c, float thick);

	void draw_rect(Math::Rectangle<float> rec, BrushObject& brush, float thick);
	void draw_rect(Math::Rectangle<float> rec, BrushObject& brush);
	void draw_rect_filled(Math::Rectangle<float> rec, BrushObject& brush);

	void draw_circle(Math::Point<float> center, float radius, BrushObject& brush, float thick);
	void draw_circle(Math::Point<float> center, float radius, BrushObject& brush);
	void draw_circle_filled(Math::Point<float> center, float radius, BrushObject& brush);

	// draw calls that create the brushobjects. Is slower than the rest i guess?
	void draw_rect(Math::Rectangle<float> rec, Renderer::Color c, float thick);
	void draw_rect(Math::Rectangle<float> rec, Renderer::Color c);
	void draw_rect_filled(Math::Rectangle<float> rec, Renderer::Color c);

	void draw_circle(Math::Point<float> center, float radius, Renderer::Color c, float thick);
	void draw_circle(Math::Point<float> center, float radius, Renderer::Color c);
	void draw_circle_filled(Math::Point<float> center, float radius, Renderer::Color c);

	void set_current_transform_active();
	void set_identity_transform_active();

	void set_transform_matrix(Math::Point<float> p);
	void set_transform_matrix(D2D1::Matrix3x2F m);
	void add_transform_matrix(Math::Point<float> p);
	void set_scale_matrix(float scale, Math::Point<float> center);
	void set_scale_matrix(D2D1::Matrix3x2F m);
	void add_scale_matrix(float scale, Math::Point<float> center);

	float get_transform_scale() const;
	D2D1::Matrix3x2F get_scale_matrix() const;
	Math::Point<float> get_zoom_center() const;
	Math::Point<float> get_transform_pos() const;
	/// <summary>
	/// Will return the actual window size
	/// </summary>
	/// <returns></returns>
	Math::Rectangle<long> get_window_size() const;
	/// <summary>
	/// Will return the window size at 96 DPI
	/// </summary>
	/// <returns></returns>
	Math::Rectangle<double> get_window_size_normalized() const;
	/// <summary>
	/// Transforms the rectangle using the current transformation matrices
	/// </summary>
	/// <param name="rec">The rectangle to be transformed</param>
	/// <returns>The new transformed rectangle</returns>
	Math::Rectangle<float> transform_rect(const Math::Rectangle<float> rec) const;

	Math::Rectangle<float> inv_transform_rect(const Math::Rectangle<float> rec) const;

	Math::Point<float> transform_point(const Math::Point<float> p) const;

	Math::Point<float> inv_transform_point(const Math::Point<float> p) const;

	UINT get_dpi() const;
	float get_dpi_scale() const;

	/// <summary>
	/// Converts the given pixel to DIPs (device independent pixels)
	/// </summary>
	/// <param name="px">The pixels</param>
	/// <returns>DIP</returns>
	template <typename T>
	T PxToDp(T px) const {
		return px / (get_dpi() / 96.0f);
	}

	/// <summary>
	/// Converts the DIP to on screen pixels
	/// </summary>
	/// <param name="dip">the dip</param>
	/// <returns>Pixels in screen coordinates</returns>
	template <typename T>
	T DptoPx(T dip) const {
		return dip * (get_dpi() / 96.0f);
	}


	template<typename T>
	Math::Point<T> PxToDp(const Math::Point<T>& pxPoint) const {
		Math::Point<T> dpPoint;
		dpPoint.x = pxPoint.x / (get_dpi() / 96.0f);
		dpPoint.y = pxPoint.y / (get_dpi() / 96.0f);
		return dpPoint;
	}

	template<typename T>
	Math::Point<T> DpToPx(const Math::Point<T>& dpPoint) const {
		Math::Point<T> pxPoint;
		pxPoint.x = dpPoint.x * (get_dpi() / 96.0f);
		pxPoint.y = dpPoint.y * (get_dpi() / 96.0f);
		return pxPoint;
	}

	template<typename T>
	Math::Rectangle<T> PxToDp(const Math::Rectangle<T>& pxRect) const {
		Math::Rectangle<T> dpRect;
		dpRect.x = pxRect.x / (get_dpi() / 96.0f);
		dpRect.y = pxRect.y / (get_dpi() / 96.0f);
		dpRect.width = pxRect.width / (get_dpi() / 96.0f);
		dpRect.height = pxRect.height / (get_dpi() / 96.0f);
		return dpRect;
	}

	template<typename T>
	Math::Rectangle<T> DpToPx(const Math::Rectangle<T>& dpRect) const {
		Math::Rectangle<T> pxRect;
		pxRect.x = dpRect.x * (get_dpi() / 96.0f);
		pxRect.y = dpRect.y * (get_dpi() / 96.0f);
		pxRect.width = dpRect.width * (get_dpi() / 96.0f);
		pxRect.height = dpRect.height * (get_dpi() / 96.0f);
		return pxRect;
	}

	TextFormatObject create_text_format(std::wstring font, float size);
	BrushObject create_brush(Renderer::Color c);
	BrushObject create_brush(Renderer::AlphaColor c);
	BitmapObject create_bitmap(const std::wstring& path);
	BitmapObject create_bitmap(const byte* const data, Math::Rectangle<unsigned int> size, unsigned int stride, float dpi);
	PathObject create_bezier_path(const Renderer::CubicBezierGeometry& cub);
	PathObject create_line_path(const std::vector<Math::Point<float>>& line);
};

struct CachedBitmap {
	Direct2DRenderer::BitmapObject bitmap;
	// coordinates in doc space
	Math::Rectangle<float> doc_coords;
	float dpi = MUPDF_DEFAULT_DPI;
	size_t page = 0;

	// Currently not used
	size_t used = false;

	CachedBitmap() = default;
	CachedBitmap(Direct2DRenderer::BitmapObject bitmap, Math::Rectangle<float> dest, float dpi = MUPDF_DEFAULT_DPI) : bitmap(bitmap), doc_coords(dest), dpi(dpi) {}
	CachedBitmap(Direct2DRenderer::BitmapObject bitmap, Math::Rectangle<float> dest, float dpi = MUPDF_DEFAULT_DPI, size_t used = 0) : bitmap(bitmap), doc_coords(dest), dpi(dpi), used(used) {}
	CachedBitmap(CachedBitmap&& cachedbitmap) noexcept {
		bitmap = std::move(cachedbitmap.bitmap);
		doc_coords = cachedbitmap.doc_coords;
		dpi = cachedbitmap.dpi;
		used = cachedbitmap.used;
		page = cachedbitmap.page;
	}
	CachedBitmap& operator=(CachedBitmap&& t) noexcept {
		// new c++ stuff?
		this->~CachedBitmap();
		new(this) CachedBitmap(std::move(t));
		return *this;
	}
};

#endif // !_DIRECT_2D_H_
