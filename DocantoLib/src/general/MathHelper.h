namespace Docanto {
	namespace Geometry {
		template <typename T>
		struct Point {
			T x;
			T y;
		};

		template <typename T>
		struct Rectangle {
			T x;
			T y;
			T width;
			T height;

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