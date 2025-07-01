#ifndef _WIN_INCLUDE_H_
#define _WIN_INCLUDE_H_

#include "DocantoLib.h"

#include <functional>

#define _UNICODE
#define UNICODE

#include <Windows.h>
#include <map>

#include <d2d1.h>
#include <dwrite_3.h>

#define APPLICATION_NAME L"Docanto"

inline Docanto::Geometry::Rectangle<long> RectToRectangle(const RECT& r) {
	return Docanto::Geometry::Rectangle<long>(r.left, r.top, r.right - r.left, r.bottom - r.top);
}

inline Docanto::Geometry::Dimension<long> RectToDimension(const RECT& r) {
	return Docanto::Geometry::Dimension<long>(r.right - r.left, r.bottom - r.top);
}

inline D2D1_SIZE_U DimensionToD2D1(const Docanto::Geometry::Dimension<long>& dim) {
	return { static_cast<UINT32>(dim.width), static_cast<UINT32>(dim.height) };
}

inline D2D1_RECT_F RectToD2D1(const Docanto::Geometry::Rectangle<float>& r) {
	return { r.x, r.y, r.x + r.width, r.y + r.height };
}

inline D2D1_POINT_2F PointToD2D1(const Docanto::Geometry::Point<float>& r) {
	return { r.x, r.y };
}

inline D2D1_COLOR_F ColorToD2D1(const Docanto::Color& c) {
	return D2D1::ColorF(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.alpha / 255.0f);
}

#endif