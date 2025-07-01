#include "include.h"

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

	static LRESULT parse_window_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static std::map<HWND, Window*>& get_instances();

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

	bool get_close_request() const;

	void set_state(WINDOW_STATE state);
	// Sets a new windowsize
	void set_window_size(Docanto::Geometry::Rectangle<long> r);

	void set_callback_paint(std::function<void()> callback);
	void set_callback_size(std::function<void(Docanto::Geometry::Dimension<long>)> callback);

	friend class Direct2DRender;
};