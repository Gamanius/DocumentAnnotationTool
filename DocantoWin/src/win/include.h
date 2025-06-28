#include "DocantoLib.h"
#define _UNICODE
#define UNICODE
#include <Windows.h>
#include <map>

#define APPLICATION_NAME L"Docanto"

inline Docanto::Geometry::Rectangle<long> RectToRectangle(const RECT& r) {
	return Docanto::Geometry::Rectangle<long>(r.left, r.top, r.right - r.left, r.bottom - r.top);
}