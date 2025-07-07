#ifndef _MATHHELPER_H_
#define _MATHHELPER_H_


namespace Docanto {
	namespace Geometry {
		template <typename T>
		struct Point {
			T x = 0;
			T y = 0;


			Point() = default;
			Point(T x, T y) : x(x), y(y) {}

			T distance() const {
				return std::sqrt(x * x + y * y);
			}

			template <typename W>
			operator Point<W>() const {
				return Point<W>((W)x, (W)y);
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
			Point<T>& operator -=(const Point<T>& p) {
				x -= p.x;
				y -= p.y;

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

		template <typename T>
		struct Rectangle {
			T x      = 0;
			T y      = 0;
			T width  = 0;
			T height = 0;

			Rectangle() = default;
			Rectangle(T x, T y, T width, T height) : x(x), y(y), width(width), height(height) {}
			Rectangle(Point<T> p1, Point<T> p2) : x(p1.x), y(p1.y), width(p2.x - p1.x), height(p2.y - p1.y) {}


			Rectangle(const Rectangle& r) : x(r.x), y(r.y), width(r.width), height(r.height) {}
			Rectangle& operator=(const Rectangle& r) {
				x = r.x;
				y = r.y;
				width = r.width;
				height = r.height;

				return *this;
			}

			T right() const { return x + width; }
			T bottom() const { return y + height; }
			Point<T> upperleft() const { return { x, y }; }
			Point<T> upperright() const { return { right(), y }; }
			Point<T> lowerleft() const { return { x, bottom() }; }
			Point<T> lowerright() const { return { right(), bottom() }; }

			template <typename W>
			operator Rectangle<W>() const {
				return Rectangle<W>((W)x, (W)y, (W)width, (W)height);
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

		};

		template <typename T>
		struct Dimension {
			T width;
			T height;

			template <typename W>
			operator Dimension<W>() const {
				return Dimension<W>((W)width, (W)height);
			}

			template <typename F>
			Dimension<T> operator / (const F& f) const {
				return Dimension<T>(width / static_cast<T>(f), height / static_cast<T>(f));
			}

			template <typename F>
			Dimension<T> operator *(const F& f) const {
				return Dimension<T>(width * static_cast<T>(f), height * static_cast<T>(f));
			}
		};
	}
}

template<typename T>
std::wostream& operator<<(std::wostream& os, const Docanto::Geometry::Point<T>& p) {
	return os << "[x=" << p.x << ", y=" << p.y << "]";
}
template<typename T>
std::wostream& operator<<(std::wostream& os, const Docanto::Geometry::Rectangle<T>& p) {
	return os << "[x=" << p.x << ", y=" << p.y << ", width=" << p.width << ", height=" << p.height << "]";
}

template<typename T>
std::wostream& operator<<(std::wostream& os, const Docanto::Geometry::Dimension<T>& p) {
	return os << "[width=" << p.width << ", height=" << p.height << "]";
}


#endif // !_MATHHELPER_H_