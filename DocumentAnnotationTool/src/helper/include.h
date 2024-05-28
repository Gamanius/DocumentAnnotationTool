#pragma once

#include <Windows.h>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include <d2d1.h>
#include <dwrite_3.h>
#include <optional>
#include <tuple>
#include <crtdbg.h>
#include <array>
#include "mupdf/fitz.h"
#include <mutex>
#include <thread>
#include <atomic>
#include <mupdf/fitz/crypt.h>
#include <deque>

/// <summary>
/// ID of the main thread. All other thread should use an ID > 1. ID = 0 is reserved 
/// </summary>
constexpr size_t MAIN_THREAD_ID = 1;

#define APPLICATION_NAME L"Docanto"
#define EPSILON 0.00001f
#define PDF_STITCH_THRESHOLD 0.1f
#define FLOAT_EQUAL(a, b) (abs(a - b) < EPSILON)

/// Custom WM_APP message to signal that the bitmap is ready to be drawn
#define WM_PDF_BITMAP_READY (WM_APP + 0x0BAD /*Magic number (rolled by fair dice)*/)

inline void SafeRelease(IUnknown* ptr) {
	if (ptr) {
		ptr->Release();
		ptr = nullptr;
	}
}

template <typename T>
class ThreadSafeClass {
	std::shared_ptr<T> obj;
	std::unique_lock<std::mutex> lock;

public:
	ThreadSafeClass(std::pair<std::mutex, std::shared_ptr<T>>& p) : lock(p.first) {
		obj = std::shared_ptr<T>(p.second);
	}

	ThreadSafeClass(std::mutex& m, std::shared_ptr<T> p) : lock(m) {
		obj = std::shared_ptr<T>(p);
	}

	ThreadSafeClass(ThreadSafeClass&& a) noexcept {
		obj = std::move(a.obj);
		a.lock.swap(lock);
	}

	ThreadSafeClass& operator=(ThreadSafeClass&& t) noexcept {
		this->~ThreadSafeClass();
		new(this) ThreadSafeClass(std::move(t));
		return *this;
	}

	T* operator->()const { return obj.get(); }
	T& operator*() const { return *obj; }

	~ThreadSafeClass() {
		lock.unlock();
	}
};


template <typename T, typename M>
class RawThreadSafeClass {
	T* obj = nullptr;
	std::unique_lock<M> lock;

public:
	RawThreadSafeClass(M& m, T* p) : lock(m) {
		obj = p;
	}

	RawThreadSafeClass(std::unique_lock<M>& l, T* p) {
		std::swap(l, lock);
		obj = p;
	}

	RawThreadSafeClass(RawThreadSafeClass&& a) noexcept {
		obj = a.obj;
		a.lock.swap(lock);
	}

	RawThreadSafeClass& operator=(RawThreadSafeClass&& t) noexcept {
		this->~RawThreadSafeClass();
		new(this) RawThreadSafeClass(std::move(t));
		return *this;
	}

	T* operator->()const { return obj; }
	T& operator*() const { return *obj; }

	~RawThreadSafeClass() {
		// will not delete pointer since it is not owning it
		lock.unlock(); 
	}
};

template<typename T, typename M = std::recursive_mutex, typename C = RawThreadSafeClass<T, M>>
struct ThreadSafeWrapper {
private:
	T m_item;
	M m_mutex;
public:
	ThreadSafeWrapper(T&& item) {
		m_item = std::move(item);
	}

	C get_item() {
		return C(m_mutex, &m_item);
	}

	std::optional<C> try_get_item() {
		// TODO DEBUG TO SEE IF IT WORKS
		std::unique_lock<M> lock(m_mutex, std::defer_lock);
		bool succ = lock.try_lock();
		if (!succ)
			return std::nullopt;
		return C(lock, &m_item);
	}

	ThreadSafeWrapper(const ThreadSafeWrapper& o) = delete;
	ThreadSafeWrapper(ThreadSafeWrapper&& o) = delete;
	ThreadSafeWrapper& operator=(const ThreadSafeWrapper& t) = delete;
	ThreadSafeWrapper& operator=(ThreadSafeWrapper&& t) = delete;


	~ThreadSafeWrapper() {
		// make sure mutex is not used
		get_item();
	}
};


namespace Logger {
	enum MsgLevel {
		INFO = 0,
		WARNING = 1,
		FATAL = 2
	};


	void log(const std::wstring& msg, MsgLevel lvl = MsgLevel::INFO);
	void log(const std::string& msg, MsgLevel lvl = MsgLevel::INFO);
	void log(const unsigned long msg, MsgLevel lvl = MsgLevel::INFO);
	void log(size_t msg, MsgLevel lvl = MsgLevel::INFO);
	void log(int msg, MsgLevel lvl = MsgLevel::INFO);
	void log(const double msg, MsgLevel lvl = MsgLevel::INFO);
	void warn(const std::string& msg);

	void assert_msg(const std::string& msg, const std::string& file, long line);
	void assert_msg_win(const std::string& msg, const std::string& file, long line);
	void assert_msg(const std::wstring& msg, const std::string& file, long line);
	void assert_msg_win(const std::wstring& msg, const std::string& file, long line);
	
	void set_console_handle(HANDLE handle);
	void print_to_console(bool clear = true);

	void print_to_debug(bool clear = true);

	void clear();

	const std::weak_ptr<std::vector<std::wstring>> get_all_msg();
}

namespace Renderer { 
	template <typename T>
	struct Point {
		T x= 0, y = 0;

		Point() = default;
		Point(POINT p) : x(p.x), y(p.y) {}
		template <typename W>
		Point(Point<W> p) : x((T)p.x), y((T)p.y) {}
		Point(D2D1_POINT_2F p) : x((T)p.x), y((T)p.y) {}
		Point(T x, T y) : x(x), y(y) {}

		operator D2D1_POINT_2F() const {
			return D2D1::Point2F(x, y);
		}

		operator std::wstring() const {
			return std::to_wstring(x) + L", " + std::to_wstring(y);
		}



		Point<T> operator +(const Point<T>& p) const { 
			return Point<T>(x + p.x, y + p.y); 
		}
		Point<T> operator -(const Point<T>& p) const {
			return Point<T>(x - p.x, y - p.y);
		}

		Point<T> operator / (const T& f) const {
			return Point<T>(x / f, y / f);
		}

		Point<T> operator *(const T& f) const {
			return Point<T>(x * f, y * f);
		}


		Point<T>& operator /= (const T& f) {
			x /= f;
			y /= f;
			return *this;
		}
		Point<T>& operator +=(const Point<T>& p) { 
			x += p.x;
			y += p.y;

			return *this;
		}
	};

	template <typename T>
	struct Rectangle {
		T x = 0, y = 0;
		T width = 0, height = 0;

		Rectangle() = default;
		Rectangle(T x, T y, T width, T height) : x(x), y(y), width(width), height(height) {}
		Rectangle(Point<T> p, T width, T height) : x(p.x), y(p.y), width(width), height(height) {}
		Rectangle(Point<T> p1, Point<T> p2) : x(p1.x), y(p1.y), width(p2.x - p1.x), height(p2.y - p1.y) { }
		Rectangle(fz_rect rect) : x(rect.x0), y(rect.y0), width(rect.x1 - rect.x0), height(rect.y1 - rect.y0) { }
		Rectangle(fz_irect rect) : x(rect.x0), y(rect.y0), width(rect.x1 - rect.x0), height(rect.y1 - rect.y0) {  }

		operator D2D1_SIZE_U() const {
			return D2D1::SizeU((UINT32)width, (UINT32)height);
		}

		T right() const { return x + width; }
		T bottom() const { return y + height; }
		Point<T> upperleft() const { return { x, y }; }
		Point<T> upperright() const { return { right(), y }; }
		Point<T> lowerleft() const { return { x, bottom() }; }
		Point<T> lowerright() const { return { right(), bottom() }; }

		operator RECT() const { 
			return { x, y, right(), bottom() };
		} 

		operator D2D1_RECT_F() const {
			return { x, y, right(), bottom() };
		}

		operator std::wstring() const {
			return std::to_wstring(x) + L", " + std::to_wstring(y) + L", " + std::to_wstring(width) + L", " + std::to_wstring(height); 
		}

		template <typename W>
		operator Rectangle<W>() const {
			return Rectangle<W>((W)x, (W)y, (W)width, (W)height);
		}

		operator fz_rect() const {
			return { x, y, right(), bottom() };
		}

		bool intersects(const Rectangle<T>& other) const {
			return (x < other.x + other.width && 
				x + width > other.x && 
				y < other.y + other.height && 
				y + height > other.y); 
		}

		bool intersects(const Point<T>& p) const {
			// NEVER CHANGE it to < or >. Keep it at >= and <= or else EVERYTHING will break!!
			return (x <= p.x && x + width >= p.x && y <= p.y && y + height >= p.y); 
		}

		Rectangle<T> calculate_overlap(const Rectangle<T>& other) const {
			Rectangle<T> overlap;

			// Calculate overlap in x-axis
			overlap.x = max(x, other.x);
			T x2 = min(x + width, other.x + other.width);
			overlap.width = max(static_cast<T>(0), x2 - overlap.x);

			// Calculate overlap in y-axis
			overlap.y = max(y, other.y);
			T y2 = min(y + height, other.y + other.height);
			overlap.height = max(static_cast<T>(0), y2 - overlap.y);

			return overlap;
		}

		/// <summary>
		/// Checks if the width and height are positive. If not it will change x,y,width and height to make it positive
		/// </summary>
		Rectangle<T>& validate() {
			if (width < 0) {
				x += width;
				width = -width;
			}
			if (height < 0) {
				y += height;
				height = -height;
			}

			return *this;
		}

	};

	struct Color {
		byte r, g, b = 0;
		Color() = default;
		Color(byte r, byte g, byte b) : r(r), g(g), b(b) {}

		operator D2D1_COLOR_F() const {
			return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f);
		}
	};

	struct AlphaColor : public Color {
		byte alpha = 0;
		AlphaColor() = default;
		AlphaColor(byte r, byte g, byte b, byte alpha) : Color(r, g, b), alpha(alpha) {}

		operator D2D1_COLOR_F() const {
			return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, alpha / 255.0f);
		}
	};


	struct RenderHandler {
		RenderHandler() = default;

		virtual void clear(Color c) {}

	};
}

/// <summary>
/// Given two rectangles r1 and r2 it will first remove the overlapping area of r2 from r1. The now non rectangle r1 will be split up into a maximum of
/// 4 rectangles and returned in an array. The array will be filled with std::nullopt if the rectangle does not exist. It will not return any rectangles
/// if r2 is either not overlapping or is bigger then r1.
/// 
/// Note that the array will always be filled from the begining. If the first element is  std::nullopt then the other elements will also be std::nullopt
/// </summary>
/// <typeparam name="T">float</typeparam>
/// <param name="r1">The main rectangle</param>
/// <param name="r2">The rectangle that shoulb be subtracted from r1</param>
/// <returns>An array of rectangles</returns>
template <typename T>
std::array<std::optional<Renderer::Rectangle<T>>, 4> splice_rect(Renderer::Rectangle<T> r1, Renderer::Rectangle<T> r2) {
	// validate the rectangles just in case
	r1.validate();
	r2.validate();
	
	// return array
	std::array<std::optional<Renderer::Rectangle<T>>, 4> return_arr = { std::nullopt, std::nullopt, std::nullopt, std::nullopt };

	if (r1.x == r2.x and r1.y == r2.y and r1.width == r2.width and r1.height == r2.height) {
		return { Renderer::Rectangle<T>(), std::nullopt, std::nullopt, std::nullopt};
	}

	// first we check if they overlap
	if (r1.intersects(r2) == false)
		return return_arr;
	// now we check for every point
	std::array<bool, 4> corner_check = { false, false, false, false };
	// upperleft
	if (r1.intersects(r2.upperleft()))
		corner_check[0] = true;
	// upperright
	if (r1.intersects(r2.upperright()))
		corner_check[1] = true;
	// bottomleft
	if (r1.intersects(r2.lowerleft()))
		corner_check[2] = true;
	// bottomright
	if (r1.intersects(r2.lowerright()))
		corner_check[3] = true;

	// count the number of corners that are inside
	byte num_corner_inside = 0;
	for (size_t i = 0; i < corner_check.size(); i++) {
		if (corner_check[i])
			num_corner_inside++;
	}

	if (num_corner_inside == 3) {
		// well we shouldn't be here
		Logger::log("This should not happen. Rectangles are weird...");
		return return_arr;
	}

	// case for 1 corner inside
	if (num_corner_inside == 1) {
		// upperleft is inside https://www.desmos.com/geometry/x17ptqj7yk
		if (corner_check[0] == true) {
			return_arr[0] = { {r1.x, r2.y}, {r2.x, r1.bottom() } };
			return_arr[1] = { r1.upperleft(), r2.upperleft() };
			return_arr[2] = { {r2.x, r1.y}, {r1.right(), r2.y} };
		}
		// upperright is inside https://www.desmos.com/geometry/clide4boo4
		else if (corner_check[1] == true) {
			return_arr[0] = { r2.upperright(), r1.lowerright() };
			return_arr[1] = { r1.upperright(), r2.upperright() };
			return_arr[2] = { {r1.x, r2.y}, {r2.right(), r1.y} };
		}
		// bottomleft is inside https://www.desmos.com/geometry/slaar3gg52
		else if (corner_check[2] == true) {
			return_arr[0] = { r2.lowerleft(), r1.lowerright() };
			return_arr[1] = { r2.lowerleft(), r1.lowerleft() };
			return_arr[2] = { {r1.x, r2.bottom()}, {r2.x, r1.y} };
		}
		// bottomright is inside https://www.desmos.com/geometry/w8a8tnweec
		else if (corner_check[3] == true) {
			return_arr[0] = { r2.lowerright() , r1.lowerright() };
			return_arr[1] = { { r1.x, r2.bottom()}, { r2.right(), r1.bottom() } };
			return_arr[2] = { { r2.right(), r1.y }, { r1.right(), r2.bottom() } };
		}
		return return_arr;
	}

	if (num_corner_inside == 2) {
		// upperleft and upperright https://www.desmos.com/geometry/p8by0k5swk
		if (corner_check[0] == true and corner_check[1] == true) {
			return_arr[0] = { r1.upperleft(), {r2.x, r1.bottom()} };
			return_arr[1] = { {r2.x, r1.y}, r2.upperright() };
			return_arr[2] = { {r2.right(), r1.y}, r1.lowerright() };
		}
		// upperleft and bottomleft https://www.desmos.com/geometry/e18umhhck7
		else if (corner_check[0] == true and corner_check[2] == true) {
			return_arr[0] = { r1.upperleft(), {r1.right(), r2.y} };
			return_arr[1] = { {r1.x, r2.y}, r2.lowerleft() };
			return_arr[2] = { {r1.x, r2.bottom()}, r1.lowerright() };
		}
		// bottomleft and bottomright https://www.desmos.com/geometry/g2fplsakd6
		else if (corner_check[2] == true and corner_check[3] == true) {
			return_arr[0] = { r1.upperleft(), {r2.x, r1.bottom()} };
			return_arr[1] = { r2.lowerleft(), {r2.right(), r1.bottom()} };
			return_arr[2] = { {r2.right(), r1.y}, r1.lowerright() };
		}
		// bottomright and upperright https://www.desmos.com/geometry/vwe0vjjtsm
		else if (corner_check[1] == true and corner_check[3] == true) {
			return_arr[0] = { r1.upperleft(), {r1.right(), r2.y} };
			return_arr[1] = { {r2.right(), r2.y}, {r1.right(), r2.bottom() } };
			return_arr[2] = { {r1.x, r2.bottom()}, r1.lowerright() };
		}
		return return_arr;
	}

	// the rectangle has been eaten o_o https://www.desmos.com/geometry/ziteq1xk2a
	if (num_corner_inside == 4) {
		return_arr[0] = { r1.upperleft(), {r2.x, r1.bottom()} };
		return_arr[1] = { {r2.x, r1.y}, r2.upperright() };
		return_arr[2] = { r2.lowerleft(), {r2.right(), r1.bottom() } };
		return_arr[3] = { {r2.right(), r1.y}, r1.lowerright() };
		return return_arr;
	}

	// we need to check if every point from r1 is inside r2. If this is the case r2 is bigger than r1 and we return nothing
	if (r2.intersects(r1.upperleft()) and r2.intersects(r1.upperright()) and r2.intersects(r1.lowerleft()) and r2.intersects(r1.lowerright()))
		return return_arr;

	// if the rectangle intersects but none of the r2 corners are inside 
	auto overlapped_rec = r1.calculate_overlap(r2);
	// Now we need to check which points are inside r2
	// upperleft and upperright
	if (r2.intersects(r1.upperleft()) and r2.intersects(r1.upperright())) {
		return_arr[0] = { overlapped_rec.lowerleft(), r1.lowerright() };
	}
	// upperleft and lowerleft
	else if (r2.intersects(r1.upperleft()) and r2.intersects(r1.lowerleft())) {
		return_arr[0] = { overlapped_rec.upperright(), r1.lowerright() };
	}
	// lowerleft and lowerright
	else if (r2.intersects(r1.lowerleft()) and r2.intersects(r1.lowerright())) {
		return_arr[0] = { r1.upperleft(), overlapped_rec.upperright() };
	}
	// upperright and lowerright
	else if (r2.intersects(r1.upperright()) and r2.intersects(r1.lowerright())) {
		return_arr[0] = { r1.upperleft(), overlapped_rec.lowerleft() };
	}

	// last case where the rectangle is like this
	//    ____
	//  _|____|___
	// | |    |   |
	// | |    |   |
	// | |    |   |
	// |_|____|___|
	//   |____|

	if (r1.x < overlapped_rec.x and r1.right() > overlapped_rec.right()) {
		return_arr[0] = { {r1.x, r1.y}, {overlapped_rec.x, r1.bottom()} };
		return_arr[1] = { {overlapped_rec.right(), r1.y}, {r1.right(), r1.bottom()} };
	}

	// or the other way round
	if (r1.y < overlapped_rec.y and r1.bottom() > overlapped_rec.bottom()) {
		return_arr[0] = { {r1.x, r1.y}, {r1.right(), overlapped_rec.y} };
		return_arr[1] = { {r1.x, overlapped_rec.bottom()}, {r1.right(), r1.bottom()} };
	}

	return return_arr;
}

template<typename T>
std::vector<Renderer::Rectangle<T>> splice_rect(Renderer::Rectangle<T> r1, const std::vector<Renderer::Rectangle<T>>& r2) {
	r1.validate();

	if (r2.empty())
		return std::vector<Renderer::Rectangle<T>>();
	std::vector<Renderer::Rectangle<T>> chopped_rects;
	chopped_rects.reserve(r2.size() * 3);
	chopped_rects.push_back(r1);
	// iterate over all secondary rectangles
	for (size_t i = 0; i < r2.size(); i++) {
		// we need to check for every spliced rectangle if it needs to be spliced again
		for (long j = (long)chopped_rects.size() - 1; j >= 0; j--) {
			// check if the rect is smaller than the r2
			if (r2.at(i).intersects(chopped_rects.at(j).upperleft())
				and r2.at(i).intersects(chopped_rects.at(j).upperright())
				and r2.at(i).intersects(chopped_rects.at(j).lowerleft())
				and r2.at(i).intersects(chopped_rects.at(j).lowerright())) {
				// remove if it is smaller
				chopped_rects.erase(chopped_rects.begin() + j);
				continue;
			}
			// create the new rects
			std::array<std::optional<Renderer::Rectangle<T>>, 4> temp = splice_rect(chopped_rects.at(j), r2.at(i));
			// check if array is empty
			if (temp[0].has_value() == false) {
				continue;
			}
			// we can remove the rectangle since it will be spliced
			chopped_rects.erase(chopped_rects.begin() + j);
			// now add the new rects
			for (size_t k = 0; k < temp.size(); k++) {
				if (temp[k].has_value()) {
					chopped_rects.push_back(temp[k].value());
				}
			}
		}

	}

	for (size_t i = 0; i < chopped_rects.size(); i++) {
		// validate all rects
		chopped_rects.at(i).validate();
	}
	return chopped_rects;
}

/// <summary>
/// Expects validated rectangles. Will merge together rectangles that are adjacent to each other. If only adjacent is true it will only merge rectangles that are directly adjacent to each other (so they are touching but not overlapping)
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="r"></param>
/// <param name="only_adjacent"></param>
template<typename T>
void merge_rects(std::vector<Renderer::Rectangle<T>>& r) {
	if (r.size() <= 1)
		return;

	bool done = true;

	while (done == true) {
		done = false;
		for (size_t i = 0; i < r.size(); i++) {
			for (size_t j = i; j < r.size(); j++) {
				//check if rectangles are mergeable
				if (i == j)
					continue;
				auto r1 = r.at(i);
				auto r2 = r.at(j);

				if (FLOAT_EQUAL(r1.x, r2.x)
					and FLOAT_EQUAL(r1.right(), r2.right())
					and (r1.bottom() >= r2.y and r2.bottom() >= r1.y)) {
					r.push_back({ r1.x, min(r1.y, r2.y), r1.width, max(r1.bottom(), r2.bottom()) - min(r1.y, r2.y)});
					r.erase(r.begin() + j);
					r.erase(r.begin() + i);
					done = true;
					continue;
				}

				if (FLOAT_EQUAL(r1.y, r2.y)
					and FLOAT_EQUAL(r1.bottom(), r2.bottom())
					and (r1.right() >= r2.x and r2.right() >= r1.x)) {
					r.push_back({ min(r1.x, r2.x), r1.y, max(r1.right(), r2.right()) - min(r1.x, r2.x), r1.height });
					r.erase(r.begin() + j);
					r.erase(r.begin() + i);
					done = true;
					continue;
				}
			}
		}
	}
}

class WindowHandler;

class Direct2DRenderer : public Renderer::RenderHandler {
	static ID2D1Factory* m_factory; 
	static IDWriteFactory3* m_writeFactory;

	HDC m_hdc = nullptr;
	HWND m_hwnd = nullptr;
	Renderer::Rectangle<long> m_window_size;
	ID2D1HwndRenderTarget* m_renderTarget = nullptr;
	
	D2D1::Matrix3x2F m_transformPosMatrix = D2D1::Matrix3x2F::Identity(), m_transformScaleMatrix = D2D1::Matrix3x2F::Identity();
	float m_transformScale = 1.0f;
	Renderer::Point<float> m_transformPos, m_transformScaleCenter;

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

	enum INTERPOLATION_MODE {
		NEAREST_NEIGHBOR,
		LINEAR
	};

	Direct2DRenderer(const WindowHandler& window);
	~Direct2DRenderer();

	void begin_draw(); 
	void end_draw();
	void clear(Renderer::Color c) override;
	void resize(Renderer::Rectangle<long> r);
	void draw_bitmap(BitmapObject& bitmap, Renderer::Point<float> pos, INTERPOLATION_MODE mode = INTERPOLATION_MODE::NEAREST_NEIGHBOR, float opacity = 1.0f);
	void draw_bitmap(BitmapObject& bitmap, Renderer::Rectangle<float> dest, INTERPOLATION_MODE mode = INTERPOLATION_MODE::NEAREST_NEIGHBOR, float opacity = 1.0f);
	void draw_text(const std::wstring& text, Renderer::Point<float> pos, TextFormatObject& format, BrushObject& brush);
	void draw_rect(Renderer::Rectangle<float> rec, BrushObject& brush, float thick);
	void draw_rect(Renderer::Rectangle<float> rec, BrushObject& brush);
	void draw_rect_filled(Renderer::Rectangle<float> rec, BrushObject& brush);

	// draw calls that create the brushobjects. Is slower than the rest i guess?
	void draw_rect(Renderer::Rectangle<float> rec, Renderer::Color c, float thick);
	void draw_rect(Renderer::Rectangle<float> rec, Renderer::Color c);
	void draw_rect_filled(Renderer::Rectangle<float> rec, Renderer::Color c);

	void set_current_transform_active();
	void set_identity_transform_active();

	void set_transform_matrix(Renderer::Point<float> p);
	void add_transform_matrix(Renderer::Point<float> p);
	void set_scale_matrix(float scale, Renderer::Point<float> center);
	void add_scale_matrix(float scale, Renderer::Point<float> center);

	float get_transform_scale() const;
	Renderer::Point<float> get_transform_pos() const;
	/// <summary>
	/// Will return the actual window size
	/// </summary>
	/// <returns></returns>
	Renderer::Rectangle<long> get_window_size() const;
	/// <summary>
	/// Will return the window size at 96 DPI
	/// </summary>
	/// <returns></returns>
	Renderer::Rectangle<double> get_window_size_normalized() const;
	/// <summary>
	/// Transforms the rectangle using the current transformation matrices
	/// </summary>
	/// <param name="rec">The rectangle to be transformed</param>
	/// <returns>The new transformed rectangle</returns>
	Renderer::Rectangle<float> transform_rect(const Renderer::Rectangle<float> rec) const;

	Renderer::Rectangle<float> inv_transform_rect(const Renderer::Rectangle<float> rec) const;

	Renderer::Point<float> transform_point(const Renderer::Point<float> p) const;

	Renderer::Point<float> inv_transform_point(const Renderer::Point<float> p) const;

	UINT get_dpi() const;

	TextFormatObject create_text_format(std::wstring font, float size);
	BrushObject create_brush(Renderer::Color c);
	BitmapObject create_bitmap(const std::wstring& path);
	BitmapObject create_bitmap(const byte* const data, Renderer::Rectangle<unsigned int> size, unsigned int stride, float dpi);
};

namespace FileHandler {
	struct File {
		byte* data = nullptr;
		size_t size = 0;

		File() = default;
		File(byte* data, size_t size) : data(data), size(size) {}
		File(File&& t) noexcept {
			data = t.data;
			t.data = nullptr;

			size = t.size;
			t.size = 0;
		}

		// Dont copy data as it could be very expensive
		File(const File& t) = delete;
		File& operator=(const File& t) = delete;

		File& operator=(File&& t) noexcept {
			if (this != &t) {
				data = t.data;
				t.data = nullptr;

				size = t.size;
				t.size = 0;
			}
			return *this;
		}

		~File() {
			delete data;
		}
	};


	/// <summary>
	/// Will create an open file dialog where only *one* file can be selected
	/// </summary>
	/// <param name="filter">A file extension filter. Example "Bitmaps (*.bmp)\0*.bmp\0All files (*.*)\0*.*\0\0"</param>
	/// <param name="windowhandle">Optional windowhandle</param>
	/// <returns>If succeful it will return the path of the file that was selected</returns>
	std::optional<std::wstring> open_file_dialog(const wchar_t* filter, HWND windowhandle = NULL);
	/// <summary>
	/// Will read the contents of the file specified by the path
	/// </summary>
	/// <param name="path">Filepath of the file</param>
	/// <returns>If succefull it will return a file struct with the data</returns>
	std::optional<File> open_file(const std::wstring& path);

	/// <summary>
	/// Returns size of the bitmap
	/// </summary>
	/// <param name="path">Path of the bitmap</param>
	/// <returns>A rectangle with the size</returns>
	std::optional<Renderer::Rectangle<DWORD>> get_bitmap_size(const std::wstring& path);

}

constexpr float MUPDF_DEFAULT_DPI = 72.0f;
class PDFRenderHandler;

struct CachedBitmap {
	Direct2DRenderer::BitmapObject bitmap;
	// coordinates in clip space
	Renderer::Rectangle<float> dest;
	float dpi = MUPDF_DEFAULT_DPI;
	size_t used = false;

	CachedBitmap() = default;
	CachedBitmap(Direct2DRenderer::BitmapObject bitmap, Renderer::Rectangle<float> dest, float dpi = MUPDF_DEFAULT_DPI) : bitmap(bitmap), dest(dest), dpi(dpi) {}
	CachedBitmap(Direct2DRenderer::BitmapObject bitmap, Renderer::Rectangle<float> dest, float dpi = MUPDF_DEFAULT_DPI, size_t used = 0) : bitmap(bitmap), dest(dest), dpi(dpi), used(used) {}
	CachedBitmap(CachedBitmap&& cachedbitmap) noexcept {
		bitmap = std::move(cachedbitmap.bitmap);
		dest = cachedbitmap.dest;
		dpi = cachedbitmap.dpi;
		used = cachedbitmap.used;
	}
	CachedBitmap& operator=(CachedBitmap&& t) noexcept {
		// new c++ stuff?
		this->~CachedBitmap();
		new(this) CachedBitmap(std::move(t));
		return *this;
	}
};


template <typename T>
using ThreadSafeVector = ThreadSafeWrapper<std::vector<T>>;

template <typename T>
using ThreadSafeDeque = ThreadSafeWrapper<std::deque<T>>;

typedef RawThreadSafeClass<fz_context*, std::recursive_mutex> ThreadSafeContextWrapper;
struct ContextWrapper : public ThreadSafeWrapper<fz_context*> {
	ContextWrapper(fz_context* c);

	/// <summary>
	/// alias for get_item
	/// </summary>
	/// <returns></returns>
	ThreadSafeContextWrapper get_context();

	~ContextWrapper();
};


struct DocumentWrapper;
typedef RawThreadSafeClass<fz_document*, std::recursive_mutex> ThreadSafeDocumentWrapper;

struct DocumentWrapper : public ThreadSafeWrapper<fz_document*>{
private:
	std::shared_ptr<ContextWrapper> m_context;
public:
	DocumentWrapper(std::shared_ptr<ContextWrapper> a, fz_document* d);

	ThreadSafeDocumentWrapper get_document();

	~DocumentWrapper();
};

struct PageWrapper;
typedef RawThreadSafeClass<fz_page*, std::recursive_mutex> ThreadSafePageWrapper;

struct PageWrapper : public ThreadSafeWrapper<fz_page*> {
private:
	std::shared_ptr<ContextWrapper> m_context; 
public:
	PageWrapper(std::shared_ptr<ContextWrapper> a, fz_page* p);

	ThreadSafePageWrapper get_page();

	~PageWrapper();
};


struct DisplayListWrapper;
typedef RawThreadSafeClass<fz_display_list*, std::recursive_mutex> ThreadSafeDisplayListWrapper;

struct DisplayListWrapper : public ThreadSafeWrapper<fz_display_list*> {
private:
	std::shared_ptr<ContextWrapper> m_context;
public:
	DisplayListWrapper(std::shared_ptr<ContextWrapper> a, fz_display_list* l);

	ThreadSafeDisplayListWrapper get_displaylist();

	~DisplayListWrapper();
};

class MuPDFHandler {
	std::shared_ptr<ContextWrapper> m_context;

	std::mutex m_mutex[FZ_LOCK_MAX];
	fz_locks_context m_locks;

	static void lock_mutex(void* user, int lock);
	static void unlock_mutex(void* user, int lock);
	static void error_callback(void* user, const char* message);
public:

	struct PDF : public FileHandler::File {
		struct PDFRenderInfoStruct {
			Direct2DRenderer* renderer = nullptr;
			size_t page = 0;
			Renderer::Rectangle<float> source;
			Renderer::Rectangle<float> dest;
			float dpi = MUPDF_DEFAULT_DPI;
		};

	private:
		// Pointer to the context from the MuPDFHandler
		std::shared_ptr<ContextWrapper> m_ctx;
		std::shared_ptr<DocumentWrapper> m_document;
		std::vector<std::shared_ptr<PageWrapper>> m_pages;
	public:

		PDF() = default; 
		PDF(std::shared_ptr<ContextWrapper> ctx, std::shared_ptr<DocumentWrapper> doc);

		// move constructor
		PDF(PDF&& t) noexcept;
		// move assignment
		PDF& operator=(PDF&& t) noexcept;

		// again dont copy
		PDF(const PDF& t) = delete;
		PDF& operator=(const PDF& t) = delete;

		// the fz_context is not allowed to be droped here
		~PDF();

		ThreadSafePageWrapper get_page(size_t page) const;
		ThreadSafeContextWrapper get_context() const;

		std::shared_ptr<ContextWrapper> get_context_wrapper() const;

		/// <summary>
		/// Retrieves the number of pdf pages
		/// </summary>
		/// <returns>Number of pages</returns>
		size_t get_page_count() const;

		Renderer::Rectangle<float> get_page_size(size_t page, float dpi = 72) const;
	};

	MuPDFHandler();

	std::optional<PDF> load_pdf(const std::wstring& path);

	ThreadSafeContextWrapper get_context();

	~MuPDFHandler();
};

class WindowHandler;

/// <summary>
/// Class to handle the rendering of only one pdf
/// </summary>
class PDFRenderHandler {
public:
	struct RenderInfo {
		size_t page = 0;
		Renderer::Rectangle<float> overlap_in_docspace;

		RenderInfo() = default;
		RenderInfo(size_t page, Renderer::Rectangle<float> overlap) : page(page), overlap_in_docspace(overlap) {}
		// move constructor
		RenderInfo(RenderInfo&& t) noexcept {
			page = t.page;
			overlap_in_docspace = t.overlap_in_docspace;
		}
		// move assignment
		RenderInfo& operator=(RenderInfo&& t) noexcept {
			this->~RenderInfo(); 
			new(this) RenderInfo(std::move(t));
			return *this;
		}

		RenderInfo(const RenderInfo& t) {
			page = t.page;
			overlap_in_docspace = t.overlap_in_docspace;
		}
		RenderInfo& operator=(const RenderInfo& t) {
			page = t.page;
			overlap_in_docspace = t.overlap_in_docspace;
			return *this;
		}

		~RenderInfo() {}
	};
private:
	// not owned by this class
	MuPDFHandler::PDF* const m_pdf = nullptr;
	// not owned by this class!
	Direct2DRenderer* const m_renderer = nullptr;
	// not owned by this class!
	WindowHandler* const m_window = nullptr;

	// Position and dimensions of the pages 
	std::vector<Renderer::Rectangle<float>> m_pagerec;

	// Preview rendering
	// rendererd at half scale or otherwise specified 
	std::shared_ptr<ThreadSafeVector<Direct2DRenderer::BitmapObject>> m_preview_bitmaps;
	std::thread m_preview_bitmap_thread;
	std::atomic_bool m_preview_bitmaps_processed = false;

	// High res rendering
	std::shared_ptr<ThreadSafeVector<std::shared_ptr<DisplayListWrapper>>> m_display_list;
	std::thread m_display_list_thread;

	std::shared_ptr<ThreadSafeDeque<CachedBitmap>> m_cachedBitmaps = nullptr; 

	// For Multithreading
	std::deque<std::tuple<Renderer::Rectangle<float>, float>> m_debug_cachedBitmap; 

	float m_seperation_distance = 10;
	float m_preview_scale = 0.5; 

	void create_preview(float scale = 0.5);
	void create_display_list();
	void create_display_list_async();

	void render_high_res();

	std::vector<Renderer::Rectangle<float>> render_job_splice_recs(Renderer::Rectangle<float>, const std::vector<size_t>& cashedBitmapindex);
	std::vector<size_t> render_job_clipped_cashed_bitmaps(Renderer::Rectangle<float> overlap_clip_space, float dpi);

	void remove_small_cached_bitmaps(float treshold);

	/// <summary>
	/// 
	/// </summary>
	/// <param name="max_memory">In Mega Bytes</param>
	void reduce_cache_size(unsigned long long max_memory);

public:
	PDFRenderHandler() = default;

	PDFRenderHandler(MuPDFHandler::PDF* const pdf, Direct2DRenderer* const renderer, WindowHandler* const window);

	// move constructor
	PDFRenderHandler(PDFRenderHandler&& t) noexcept;
	// move assignment
	PDFRenderHandler& operator=(PDFRenderHandler&& t) noexcept;

	~PDFRenderHandler(); 

	/// <summary>
	/// Will render out the pdf page to a bitmap. It will always give back a bitmap with the DPI 96.
	/// To account for higher DPI's it will instead scale the size of the image using <c>get_page_size </c> function
	/// </summary>
	/// <param name="renderer">Reference to the Direct2D renderer</param>
	/// <param name="page">The page to be rendererd</param>
	/// <param name="dpi">The dpi at which it should be rendered</param>
	/// <returns>The bitmap object</returns>
	Direct2DRenderer::BitmapObject get_bitmap(Direct2DRenderer& renderer, size_t page, float dpi = 72.0f) const;
	/// <summary>
	/// Will render out the pdf page to a bitmap with the given size
	/// </summary>
	/// <param name="renderer">Reference to the Direct2D renderer</param>
	/// <param name="page">The page to be rendererd</param>
	/// <param name="rec">The end size of the Bitmap</param>
	/// <returns>Bitmap</returns>
	Direct2DRenderer::BitmapObject get_bitmap(Direct2DRenderer& renderer, size_t page, Renderer::Rectangle<unsigned int> rec) const;
	/// <summary>
	/// Will render out the area of the pdf page given by the source rectangle to a bitmap with the given dpi. 
	/// </summary>
	/// <param name="renderer">Reference to the Direct2D renderer</param>
	/// <param name="page">The page to be rendererd</param>
	/// <param name="source">The area of the page in docpage pixels. Should be smaller than get_page_size() would give</param>
	/// <param name="dpi">The dpi at which it should be rendered</param>
	/// <returns>Returns the DPI-scaled bitmap</returns>
	/// <remark>If the DPI is 72 the size of the returned bitmap is the same as the size of the source</remark>
	Direct2DRenderer::BitmapObject get_bitmap(Direct2DRenderer& renderer, size_t page, Renderer::Rectangle<float> source, float dpi) const;

	void render_preview();
	void render_outline();
	void render();
	/// <summary>
	/// This HAS TO BE CALLED when multithreaded rendering is active
	/// </summary>
	//void stop_rendering(HWND callbackwindow);
	void debug_render();
	void sort_page_positions();

	unsigned long long get_cache_memory_usage() const;
};

class WindowHandler {
	HWND m_hwnd = NULL;
	HDC m_hdc = NULL;
	UINT m_dpi = 96;

	bool m_closeRequest = false;

	static std::unique_ptr<std::map<HWND, WindowHandler*>> m_allWindowInstances;

public:

	enum WINDOW_STATE {
		HIDDEN,
		MAXIMIZED,
		NORMAL,
		MINIMIZED
	};

	enum POINTER_TYPE {
		UNKNOWN,
		MOUSE,
		STYLUS,
		TOUCH
	};

	enum VK {
		LEFT_MB,
		RIGHT_MB,
		CANCEL,
		MIDDLE_MB,
		X1_MB,
		X2_MB,
		LEFT_SHIFT,
		RIGHT_SHIFT,
		LEFT_CONTROL,
		RIGHT_CONTROL,
		BACKSPACE,
		TAB,
		ENTER,
		ALT,
		PAUSE,
		CAPSLOCK,
		ESCAPE,
		SPACE,
		PAGE_UP,
		PAGE_DOWN,
		END,
		HOME,
		LEFTARROW,
		UPARROW,
		RIGHTARROW,
		DOWNARROW,
		SELECT,
		PRINT,
		EXECUTE,
		PRINT_SCREEN,
		INSERT,
		DEL,
		HELP,
		KEY_0,
		KEY_1,
		KEY_2,
		KEY_3,
		KEY_4,
		KEY_5,
		KEY_6,
		KEY_7,
		KEY_8,
		KEY_9,
		A,
		B,
		C,
		D,
		E,
		F,
		G,
		H,
		I,
		J,
		K,
		L,
		M,
		N,
		O,
		P,
		Q,
		R,
		S,
		T,
		U,
		V,
		W,
		X,
		Y,
		Z,
		LEFT_WINDOWS,
		RIGHT_WINDOWS,
		APPLICATION,
		SLEEP,
		SCROLL_LOCK,
		LEFT_MENU,
		RIGHT_MENU,
		VOLUME_MUTE,
		VOLUME_DOWN,
		VOLUME_UP,
		MEDIA_NEXT,
		MEDIA_LAST,
		MEDIA_STOP,
		MEDIA_PLAY_PAUSE,
		OEM_1,
		OEM_2,
		OEM_3,
		OEM_4,
		OEM_5,
		OEM_6,
		OEM_7,
		OEM_8,
		OEM_102,
		OEM_CLEAR,
		OEM_PLUS,
		OEM_COMMA,
		OEM_MINUS,
		OEM_PERIOD,
		NUMPAD_0,
		NUMPAD_1,
		NUMPAD_2,
		NUMPAD_3,
		NUMPAD_4,
		NUMPAD_5,
		NUMPAD_6,
		NUMPAD_7,
		NUMPAD_8,
		NUMPAD_9,
		NUMPAD_MULTIPLY,
		NUMPAD_ADD,
		NUMPAD_SEPERATOR,
		NUMPAD_SUBTRACT,
		NUMPAD_COMMA,
		NUMPAD_DIVIDE,
		NUMPAD_LOCK,
		F1,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,
		F13,
		F14,
		F15,
		F16,
		F17,
		F18,
		F19,
		F20,
		F21,
		F22,
		F23,
		F24,
		PLAY,
		ZOOM,
		UNKWON
	};

	struct PointerInfo {
		UINT id = 0;
		POINTER_TYPE type = POINTER_TYPE::UNKNOWN;
		Renderer::Point<float> pos = { 0, 0 };
		UINT32 pressure = 0;
		bool button1pressed = false; /* Left mouse button or barrel button */
		bool button2pressed = false; /* Right mouse button eareser button */
		bool button3pressed = false; /* Middle mouse button */
		bool button4pressed = false; /* X1 Button */
		bool button5pressed = false; /* X2 Button */
	};

private:
	std::function<void(std::optional<std::vector<CachedBitmap*>*>)> m_callback_paint;
	std::function<void(Renderer::Rectangle<long>)> m_callback_size;
	std::function<void(PointerInfo)> m_callback_pointer_down;
	std::function<void(PointerInfo)> m_callback_pointer_up;
	std::function<void(PointerInfo)> m_callback_pointer_update;
	std::function<void(short, bool, Renderer::Point<int>)> m_callback_mousewheel;
	std::function<void(VK)> m_callback_key_down;
	std::function<void(VK)> m_callback_key_up; 
public:

	static LRESULT parse_window_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Retrieves window messages for _all_ windows
	static void get_window_messages(bool blocking);

	bool init(std::wstring windowname, HINSTANCE instance);

	WindowHandler(std::wstring windowname, HINSTANCE instance);
	~WindowHandler();

	void set_state(WINDOW_STATE state);

	void set_callback_paint(std::function<void(std::optional<std::vector<CachedBitmap*>*>)> callback);
	void set_callback_size(std::function<void(Renderer::Rectangle<long>)> callback);
	void set_callback_pointer_down(std::function<void(PointerInfo)> callback);
	void set_callback_pointer_up(std::function<void(PointerInfo)> callback);
	void set_callback_pointer_update(std::function<void(PointerInfo)> callback);
	void set_callback_mousewheel(std::function<void(short, bool, Renderer::Point<int>)> callback);
	void set_callback_key_down(std::function<void(VK)> callback);
	void set_callback_key_up(std::function<void(VK)> callback);

	void invalidate_drawing_area();
	void invalidate_drawing_area(Renderer::Rectangle<long> rec);

	// Returns the window handle
	HWND get_hwnd() const;

	// Returns the device context
	HDC get_hdc() const;

	// Returns the DPI of the window
	UINT get_dpi() const;

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

	// Returns the window size
	Renderer::Rectangle<long> get_size() const;

	/// <summary>
	/// Returns true if a close request has been sent to the window
	/// </summary>
	/// <returns>true if there has been a close request</returns>
	bool close_request() const;

	static bool is_key_pressed(VK key);

	Renderer::Point<long> get_mouse_pos() const;

	/// <summary>
	///  returns the window handle
	/// </summary>
	operator HWND() const { return m_hwnd; }
};

//#ifndef NDEBUG
#define ASSERT(x, y) if (!(x)) { Logger::assert_msg(y, __FILE__, __LINE__); __debugbreak(); }
#define ASSERT_WIN(x,y) if (!(x)) { Logger::assert_msg_win(y, __FILE__, __LINE__); __debugbreak(); }
#define ASSERT_WITH_STATEMENT(x, y, z) if (!(x)) { Logger::assert_msg(y, __FILE__, __LINE__); __debugbreak(); z; }
#define ASSERT_WIN_WITH_STATEMENT(x, y, z) if (!(x)) { Logger::assert_msg_win(y, __FILE__, __LINE__); __debugbreak(); z; }
#define ASSERT_WIN_RETURN_FALSE(x,y)  if (!(x)) { Logger::assert_msg_win(y, __FILE__, __LINE__); __debugbreak(); return false; }
#define ASSERT_RETURN_NULLOPT(x,y) if (!(x)) { Logger::assert_msg(y, __FILE__, __LINE__); __debugbreak(); return std::nullopt; }
#define ASSERT_WIN_RETURN_NULLOPT(x,y)  if (!(x)) { Logger::assert_msg_win(y, __FILE__, __LINE__); __debugbreak(); return std::nullopt; }
//#else
//#define ASSERT(x, y)
//#endif // !NDEBUG