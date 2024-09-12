#pragma once

#define EPSILON 0.00001f
#define FLOAT_EQUAL(a, b) (abs(a - b) < EPSILON)

#include <d2d1.h>
#include <cmath>
#include "mupdf/fitz/geometry.h"
#include "Logger.h"


#ifndef _MATH_H_
#define _MATH_H_

namespace Math { 
	template <typename T>
	struct Point {
		T x = 0, y = 0;

		Point() = default;
		Point(POINT p) : x(p.x), y(p.y) {}
		template <typename W>
		Point(Point<W> p) : x((T)p.x), y((T)p.y) {}
		Point(D2D1_POINT_2F p) : x((T)p.x), y((T)p.y) {}
		Point(fz_point p) : x((T)p.x), y((T)p.y) {}
		Point(T x, T y) : x(x), y(y) {}

		operator D2D1_POINT_2F() const {
			return D2D1::Point2F(x, y);
		}

		operator fz_point() const {
			fz_point p;
			p.x = x;
			p.y = y;
			return p;
		}

		operator std::wstring() const {
			return std::to_wstring(x) + L", " + std::to_wstring(y);
		}

		T distance() const {
			return std::sqrt(x * x + y * y);
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


		template <typename F>
		Point<T> operator / (const F& f) const {
			return Point<T>(x / static_cast<T>(f), y / static_cast<T>(f));
		}

		template <typename F>
		Point<T> operator *(const F& f) const {
			return Point<T>(x * static_cast<T>(f), y * static_cast<T>(f));
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

	template <typename T, typename W>
	Point<T> operator*(Point<T> p, W f) {
		return { p.x * static_cast<T>(f), p.y * static_cast<T>(f) };
	}

	template <typename T, typename W>
	Point<T> operator*(W f, Point<T> p) {
		return { p.x * static_cast<T>(f), p.y * static_cast<T>(f) };
	}

	/// <summary>
	/// This function checks if two line segments intersects. It is copied on the code from https://www.geeksforgeeks.org/check-if-two-given-line-segments-intersect/
	/// </summary>
	/// <param name="p1">First point from the first segment</param>
	/// <param name="q1">Second point from the first segment</param>
	/// <param name="p2">First point from the seconds segment</param>
	/// <param name="q2">Second point from the second segment</param>
	/// <returns></returns>
	template <typename T>
	inline bool line_segment_intersects(Point<T> p1, Point<T> q1, Point<T> p2, Point<T> q2) {
		auto orientation = [](Point<T> p, Point<T> q, Point<T> r) {
			T val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
			if (val == 0) return 0;
			return (val > 0) ? 1 : 2;
			};

		auto onSegment = [](Point<T> p, Point<T> q, Point<T> r) {
			if (q.x <= max(p.x, r.x) and q.x >= min(p.x, r.x) and
				q.y <= max(p.y, r.y) and q.y >= min(p.y, r.y))
				return true;
			return false;
			};

		int o1 = orientation(p1, q1, p2);
		int o2 = orientation(p1, q1, q2);
		int o3 = orientation(p2, q2, p1);
		int o4 = orientation(p2, q2, q1);

		// General case 
		if (o1 != o2 && o3 != o4)
			return true;

		// Special Cases 
		// p1, q1 and p2 are collinear and p2 lies on segment p1q1 
		if (o1 == 0 && onSegment(p1, p2, q1)) return true;

		// p1, q1 and q2 are collinear and q2 lies on segment p1q1 
		if (o2 == 0 && onSegment(p1, q2, q1)) return true;

		// p2, q2 and p1 are collinear and p1 lies on segment p2q2 
		if (o3 == 0 && onSegment(p2, p1, q2)) return true;

		// p2, q2 and q1 are collinear and q1 lies on segment p2q2 
		if (o4 == 0 && onSegment(p2, q1, q2)) return true;

		return false; // Doesn't fall in any of the above cases 
	}

	template <typename T>
	struct Rectangle {
		T x = 0, y = 0;
		T width = 0, height = 0;

		Rectangle() = default;
		Rectangle(T x, T y, T width, T height) : x(x), y(y), width(width), height(height) {}
		Rectangle(Point<T> p, T width, T height) : x(p.x), y(p.y), width(width), height(height) {}
		Rectangle(Point<T> p1, Point<T> p2) : x(p1.x), y(p1.y), width(p2.x - p1.x), height(p2.y - p1.y) {}
		Rectangle(fz_rect rect) : x(rect.x0), y(rect.y0), width(rect.x1 - rect.x0), height(rect.y1 - rect.y0) {}
		Rectangle(fz_irect rect) : x(rect.x0), y(rect.y0), width(rect.x1 - rect.x0), height(rect.y1 - rect.y0) {}
		Rectangle(const RECT& rect) : x(rect.left), y(rect.top), width(rect.right - rect.left), height(rect.bottom - rect.top) {}

		operator D2D1_SIZE_U() const {
			return D2D1::SizeU((UINT32)width, (UINT32)height);
		}

		T right() const { return x + width; }
		T bottom() const { return y + height; }
		Point<T> upperleft() const { return { x, y }; }
		Point<T> upperright() const { return { right(), y }; }
		Point<T> lowerleft() const { return { x, bottom() }; }
		Point<T> lowerright() const { return { right(), bottom() }; }

		Rectangle<T> operator+(const Point<T>& p) const {
			return Rectangle<T>(x + p.x, y + p.y, width, height);
		}

		Rectangle<T> operator-(const Point<T>& p) const {
			return Rectangle<T>(x - p.x, y - p.y, width, height);
		}

		Rectangle<T>& operator+=(const Point<T>& p) {
			x += p.x;
			y += p.y;

			return *this;
		}

		Rectangle<T>& operator-=(const Point<T>& p) {
			x -= p.x;
			y -= p.y;

			return *this;
		}


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

	// deduction guide for rectangle
	Rectangle(const RECT&)->Rectangle<long>;

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
	std::array<std::optional<Math::Rectangle<T>>, 4> splice_rect(Math::Rectangle<T> r1, Math::Rectangle<T> r2) {
		// validate the rectangles just in case
		r1.validate();
		r2.validate();

		// return array
		std::array<std::optional<Math::Rectangle<T>>, 4> return_arr = { std::nullopt, std::nullopt, std::nullopt, std::nullopt };

		if (r1.x == r2.x and r1.y == r2.y and r1.width == r2.width and r1.height == r2.height) {
			return { Math::Rectangle<T>(), std::nullopt, std::nullopt, std::nullopt };
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
	std::vector<Math::Rectangle<T>> splice_rect(Math::Rectangle<T> r1, const std::vector<Math::Rectangle<T>>& r2) {
		r1.validate();

		if (r2.empty())
			return std::vector<Math::Rectangle<T>>();
		std::vector<Math::Rectangle<T>> chopped_rects;
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
				std::array<std::optional<Math::Rectangle<T>>, 4> temp = splice_rect(chopped_rects.at(j), r2.at(i));
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
	/// Will check how many corners of r2 are inside of r1
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <param name="r1">The bigger rectangle</param>
	/// <param name="r2">The smaller rectangle</param>
	/// <returns></returns>
	template<typename T>
	byte amount_of_corners_inside_rectangles(Math::Rectangle<T> r1, Math::Rectangle<T> r2) {
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

		return num_corner_inside;
	}

	/// <summary>
	/// Expects validated rectangles. Will merge together rectangles that are adjacent to each other. If only adjacent is true it will only merge rectangles that are directly adjacent to each other (so they are touching but not overlapping)
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <param name="r"></param>
	/// <param name="only_adjacent"></param>
	template<typename T>
	void merge_rects(std::vector<Math::Rectangle<T>>& r) {
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
						r.push_back({ r1.x, min(r1.y, r2.y), r1.width, max(r1.bottom(), r2.bottom()) - min(r1.y, r2.y) });
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
}

// Overload the stringstream operator for Point and Rectangle
template <typename T>
auto& operator<<(std::wstringstream& s, const Math::Point<T> p) {
	return s << L"(x=" << p.x << L", y=" << p.y << L")";
}

template <typename T>
auto& operator<<(std::wstringstream& s, const Math::Rectangle<T>& rect) {
	return s << L"(x=" << rect.x << L", y=" << rect.y << L", width=" << rect.width << L", height=" << rect.height << L")";
}

#endif // _MATH_H_
