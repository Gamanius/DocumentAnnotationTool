#ifndef _MATHHELPER_H_
#define _MATHHELPER_H_


namespace Docanto {
	namespace Geometry {
		template <typename T>
		struct Point {
			T x;
			T y;
		};

		template <typename T>
		struct Rectangle {
			T x      = 0;
			T y      = 0;
			T width  = 0;
			T height = 0;

			Rectangle();
			Rectangle(T x, T y, T width, T height) : x(x), y(y), width(width), height(height) {}

			template <typename W>
			operator Rectangle<W>() const {
				return Rectangle<W>((W)x, (W)y, (W)width, (W)height);
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