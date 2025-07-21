#include "helper/Geometry.h"
#include "Window.h"

#ifndef _DOCANTOWIN_DIRECT2D_H_
#define _DOCANTOWIN_DIRECT2D_H_


namespace DocantoWin {
	inline void SafeRelease(IUnknown* ptr) {
		if (ptr) {
			ptr->Release();
			ptr = nullptr;
		}
	}

	class Direct2DRender : public Docanto::BasicRender {
		std::shared_ptr<Window> m_window;
		ID2D1HwndRenderTarget* m_renderTarget = nullptr;

		D2D1::Matrix3x2F m_transformTranslationMatrix = D2D1::Matrix3x2F::Identity();
		D2D1::Matrix3x2F m_transformScaleMatrix = D2D1::Matrix3x2F::Identity();
		D2D1::Matrix3x2F m_transformRotationMatrix = D2D1::Matrix3x2F::Identity();

		std::atomic<UINT32> m_isRenderinProgress = 0;
		std::mutex draw_lock;

		bool createD2DResources();
		bool initWinrt();
		void createBlur();

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

			operator T* () const {
				return m_object;
			}

			T* operator->() const {
				return m_object;
			}
		};

		typedef RenderObject<ID2D1SolidColorBrush> BrushObject;
		typedef RenderObject<ID2D1StrokeStyle> StrokeStyle;
		typedef RenderObject<ID2D1Bitmap> BitmapObject;
		typedef RenderObject<IDWriteTextFormat> TextFormatObject;
		typedef RenderObject<IDWriteTextLayout> TextLayoutObject;
		typedef RenderObject<ID2D1PathGeometry> PathObject;
	private:
		// GPU resources
		BrushObject m_solid_brush;

	public:

		Direct2DRender(std::shared_ptr<Window> w);
		~Direct2DRender();

		void begin_draw();
		void end_draw();

		void clear(Docanto::Color c);
		void clear();

		void draw_text(const std::wstring& text, Docanto::Geometry::Point<float> pos, TextFormatObject& format, BrushObject& brush);
		void draw_text(const std::wstring& text, Docanto::Geometry::Point<float> pos, Docanto::Color c, float size);

		void draw_rect(Docanto::Geometry::Rectangle<float> r, Docanto::Color c);
		void draw_rect(Docanto::Geometry::Rectangle<float> r, Docanto::Color c, float thick);
		void draw_rect(Docanto::Geometry::Rectangle<float> r, BrushObject& brush, float thick = 1);

		void draw_line(Docanto::Geometry::Point<float> p1, Docanto::Geometry::Point<float> p2, BrushObject& brush, float thick = 1);
		void draw_line(Docanto::Geometry::Point<float> p1, Docanto::Geometry::Point<float> p2, Docanto::Color c, float thick = 1);

		void draw_rect_filled(Docanto::Geometry::Rectangle<float> r, BrushObject& brush);
		void draw_rect_filled(Docanto::Geometry::Rectangle<float> r, Docanto::Color c);

		void draw_bitmap(Docanto::Geometry::Point<float> where, BitmapObject& obj);
		void draw_bitmap(Docanto::Geometry::Rectangle<float> rec, BitmapObject& obj);
		void draw_bitmap(Docanto::Geometry::Point<float> where, BitmapObject& obj, float dpi);

		void set_current_transform_active();
		void set_identity_transform_active();

		void set_translation_matrix(Docanto::Geometry::Point<float> p);
		void set_translation_matrix(D2D1::Matrix3x2F m);
		void add_translation_matrix(Docanto::Geometry::Point<float> p);

		void set_scale_matrix(float scale, Docanto::Geometry::Point<float> center);
		void set_scale_matrix(D2D1::Matrix3x2F m);
		void add_scale_matrix(float scale, Docanto::Geometry::Point<float> center);

		void set_rotation_matrix(float angle, Docanto::Geometry::Point<float> center);
		void set_rotation_matrix(D2D1::Matrix3x2F m);
		void add_rotation_matrix(float angle, Docanto::Geometry::Point<float> center);

		Docanto::Geometry::Point<float> inv_transform(Docanto::Geometry::Point<float> p);
		Docanto::Geometry::Point<float> transform(Docanto::Geometry::Point<float> p);
		Docanto::Geometry::Rectangle<float> transform(Docanto::Geometry::Rectangle<float> r);
		Docanto::Geometry::Rectangle<float> inv_transform(Docanto::Geometry::Rectangle<float> r);

		float get_transform_scale() const;
		D2D1::Matrix3x2F get_scale_matrix() const;
		D2D1::Matrix3x2F get_rotation_matrix() const;
		D2D1::Matrix3x2F get_transformation_matrix() const;
		Docanto::Geometry::Point<float> get_zoom_center() const;
		Docanto::Geometry::Point<float> get_transform_pos() const;
		float get_angle() const;
		float get_dpi() override;

		std::shared_ptr<Window> get_attached_window() const;

		void resize(Docanto::Geometry::Dimension<long> r);

		BrushObject create_brush(Docanto::Color c);
		TextFormatObject create_text_format(std::wstring font, float size);
		TextLayoutObject create_text_layout(const std::wstring& text, TextFormatObject& format);
		BitmapObject create_bitmap(const Docanto::Image& i);
	};

}
#endif // !_DOCANTOWIN_DIRECT2D_H_
