#include "include.h"

class Window {
	static std::unique_ptr<std::map<HWND, Window*>> m_allWindowInstances;
	HWND m_hwnd = NULL;
	HDC m_hdc = NULL;

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

	// Retrieves window messages for _all_ windows
	static void get_window_messages(bool blocking);

	Window(HINSTANCE h);


	// Returns the DPI of the window
	UINT get_dpi() const;

	void set_window_title(const std::wstring& s);

	// Returns the window size
	Docanto::Geometry::Rectangle<long> get_client_size() const;
	Docanto::Geometry::Rectangle<long> get_window_size() const;
	Docanto::Geometry::Point<long> get_window_position() const;

	bool is_window_maximized() const;


	void set_state(WINDOW_STATE state);
};