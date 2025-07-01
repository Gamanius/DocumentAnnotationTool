#include "include.h"
#include "Window.h"

inline void SafeRelease(IUnknown* ptr) {
	if (ptr) {
		ptr->Release();
		ptr = nullptr;
	}
}

class Direct2DRender : public Docanto::BasicRender {
	std::shared_ptr<Window> m_window;

	static ID2D1Factory* m_factory;
	static IDWriteFactory3* m_writeFactory;

	ID2D1HwndRenderTarget* m_renderTarget = nullptr;

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

		operator T* () const {
			return m_object;
		}
	};

	typedef RenderObject<ID2D1SolidColorBrush> BrushObject;
	typedef RenderObject<ID2D1StrokeStyle> StrokeStyle;
	typedef RenderObject<ID2D1Bitmap> BitmapObject;
	typedef RenderObject<IDWriteTextFormat> TextFormatObject;
	typedef RenderObject<ID2D1PathGeometry> PathObject;

	Direct2DRender(std::shared_ptr<Window> w);
	~Direct2DRender();

	void begin_draw();
	void end_draw();

	void draw_rect(Docanto::Geometry::Rectangle<float> r, Docanto::Color c);


	void resize(Docanto::Geometry::Dimension<long> r);

	BrushObject create_brush(Docanto::Color c);
};