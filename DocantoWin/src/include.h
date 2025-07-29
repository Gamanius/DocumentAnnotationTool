#ifndef _WIN_INCLUDE_H_
#define _WIN_INCLUDE_H_

#include "DocantoLib.h"

#include <functional>

#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>
#include <map>

#include <d2d1.h>
#include <dwrite_3.h>

#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "hid.lib")

#define APPLICATION_NAME L"Docanto"

#define EPSILON 0.00001f
#define FLOAT_EQUAL(a, b) (abs(a - b) < EPSILON)

inline constexpr auto PI = 3.14159265359;
inline constexpr auto RAD_TO_DEG = 180 / PI;

inline constexpr auto WINDOWS_DEFAULT_DPI = 96.0f;

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

inline Docanto::Geometry::Point<float> D2D1ToPoint(const D2D1_POINT_2F& r) {
	return { r.x, r.y };
}

inline D2D1_COLOR_F ColorToD2D1(const Docanto::Color& c) {
	return D2D1::ColorF(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.alpha / 255.0f);
}

inline bool print_last_windows_error() {
    // Get the last error code from the Windows API
    DWORD errorCode = GetLastError();

    // If there's no error (errorCode is 0), just print a message and return.
    if (errorCode == 0) {
        return false;
    }

    // Buffer to hold the error message.
    // Use a wide character string buffer for Windows API, convert to std::string for ostream.
    LPWSTR messageBuffer = nullptr;

    DWORD charsWritten = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,           
        errorCode,       
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&messageBuffer, 
        0,               
        NULL             
    );

    if (charsWritten == 0) {
        // FormatMessage itself can fail. Get its error code.
        DWORD formatMessageError = GetLastError();
        Docanto::Logger::warn("Couldn't get the last window error... with windows error code ", formatMessageError);
        return false;
    }

    Docanto::Logger::error("Windows error (", errorCode, "): \"", messageBuffer, "\"");

    // Free the buffer allocated by FormatMessageW
    LocalFree(messageBuffer);

    return true;
}

#define WIN_ASSERT_OK(res, ...)\
(!FAILED(res) || \
     (Docanto::Logger::error(__VA_ARGS__), print_last_windows_error(), false))


#endif