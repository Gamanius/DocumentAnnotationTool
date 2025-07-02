#ifndef _GEOMETRY_H_
#define _GEOMETRY_H_

#include "../include.h"

template <typename T>
struct ExtendedRectangle : public Docanto::Geometry::Rectangle<T> {
	ExtendedRectangle() = default;

	ExtendedRectangle(const Docanto::Geometry::Rectangle<T>& r)
		: Docanto::Geometry::Rectangle<T>(r) {} // Initialize base class from argument

	operator RECT() const {
		return { this->x, this->y, this->right(), this->bottom() };
	}

	operator D2D1_RECT_F() const {
		return { this->x, this->y, this->right(), this->bottom() };
	}
};

template <typename T>
struct ExtendedPoint : public Docanto::Geometry::Point<T> {
	ExtendedPoint() = default;

	ExtendedPoint(const Docanto::Geometry::Point<T>& p)
		: Docanto::Geometry::Point<T>(p) {}

	operator D2D1_POINT_2F() const {
		return D2D1::Point2F(this->x, this->y);
	}
};

#endif // !_GEOMETRY_H_
