#include "../include.h"
#include "helper/Geometry.h"


namespace DocantoWin {
	class Direct2DRender;

	class Window {
		HWND m_hwnd = NULL;
		HDC m_hdc = NULL;

		bool m_closeRequest = false;

		std::function<void()> m_callback_paint;
		std::function<void(Docanto::Geometry::Dimension<long>)> m_callback_size;

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

		static LRESULT wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT parse_message(UINT uMsg, WPARAM wParam, LPARAM lParam);

		// Retrieves window messages for _all_ windows
		static void get_window_messages(bool blocking);

		Window(HINSTANCE h);

		Window(Window&& other) noexcept = delete;
		Window& operator=(Window&& other) noexcept = delete;
		Window(const Window& other) = delete;
		Window& operator=(Window& other) = delete;
		~Window();


		// Returns the DPI of the window
		UINT get_dpi() const;

		void set_window_title(const std::wstring& s);

		// Returns the window size
		Docanto::Geometry::Dimension<long> get_client_size() const;
		Docanto::Geometry::Dimension<long> get_window_size() const;
		Docanto::Geometry::Point<long> get_window_position() const;

		bool is_window_maximized() const;


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


		template<typename T>
		ExtendedPoint<T> PxToDp(const Docanto::Geometry::Point<T>& pxPoint) const {
			ExtendedPoint<T> dpPoint;
			dpPoint.x = pxPoint.x / (get_dpi() / 96.0f);
			dpPoint.y = pxPoint.y / (get_dpi() / 96.0f);
			return dpPoint;
		}

		template<typename T>
		ExtendedPoint<T> DpToPx(const Docanto::Geometry::Point<T>& dpPoint) const {
			ExtendedPoint<T> pxPoint;
			pxPoint.x = dpPoint.x * (get_dpi() / 96.0f);
			pxPoint.y = dpPoint.y * (get_dpi() / 96.0f);
			return pxPoint;
		}

		template<typename T>
		ExtendedRectangle<T> PxToDp(const Docanto::Geometry::Rectangle<T>& pxRect) const {
			ExtendedRectangle<T> dpRect;
			dpRect.x = pxRect.x / (get_dpi() / 96.0f);
			dpRect.y = pxRect.y / (get_dpi() / 96.0f);
			dpRect.width = pxRect.width / (get_dpi() / 96.0f);
			dpRect.height = pxRect.height / (get_dpi() / 96.0f);
			return dpRect;
		}

		template<typename T>
		ExtendedRectangle<T> DpToPx(const Docanto::Geometry::Rectangle<T>& dpRect) const {
			ExtendedRectangle<T> pxRect;
			pxRect.x = dpRect.x * (get_dpi() / 96.0f);
			pxRect.y = dpRect.y * (get_dpi() / 96.0f);
			pxRect.width = dpRect.width * (get_dpi() / 96.0f);
			pxRect.height = dpRect.height * (get_dpi() / 96.0f);
			return pxRect;
		}


		bool get_close_request() const;

		void set_state(WINDOW_STATE state);
		// Sets a new windowsize
		void set_window_size(Docanto::Geometry::Rectangle<long> r);

		void set_callback_paint(std::function<void()> callback);
		void set_callback_size(std::function<void(Docanto::Geometry::Dimension<long>)> callback);

		friend class Direct2DRender;
	};
}