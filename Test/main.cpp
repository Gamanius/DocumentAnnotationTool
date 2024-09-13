#include <windows.h>
#include <dwmapi.h>
#include <windowsx.h>
#pragma comment(lib, "dwmapi.lib")

#define CLOSE_BUTTON_WIDTH  200
#define CLOSE_BUTTON_HEIGHT 30

// Function to draw the close button
void DrawCloseButton(HDC hdc, RECT rcButton, BOOL hovered) {
    HBRUSH brush = CreateSolidBrush(hovered ? RGB(255, 0, 0) : RGB(200, 0, 0));  // Red color
    FillRect(hdc, &rcButton, brush);
    DeleteObject(brush);

    // Draw the 'X' for close button
    SetTextColor(hdc, RGB(255, 255, 255)); // White color for 'X'
    SetBkMode(hdc, TRANSPARENT);
    DrawText(hdc, L"X", -1, &rcButton, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static BOOL hovered = FALSE;  // For hover effect
    switch (uMsg) {
    case WM_NCHITTEST:
    {
        LRESULT hit = DefWindowProc(hwnd, uMsg, wParam, lParam);

        // Get the window dimensions
        RECT rcWindow;
        GetWindowRect(hwnd, &rcWindow);

        // Calculate close button position
        RECT rcCloseButton = {
            rcWindow.right - CLOSE_BUTTON_WIDTH,
            rcWindow.top,
            rcWindow.right,
            rcWindow.top + CLOSE_BUTTON_HEIGHT
        };

        // Determine if mouse is on close button
        POINT ptMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (PtInRect(&rcCloseButton, ptMouse))
            return HTCLIENT; // Mark it as part of the client area to custom handle clicks

        return 0;
    }

    case WM_NCPAINT:
    {
        // Default non-client area drawing
        DefWindowProc(hwnd, uMsg, wParam, lParam);

        // Get device context for the non-client area
        HDC hdc = GetWindowDC(hwnd);

        // Get window rect to draw the close button
        RECT rcWindow;
        GetWindowRect(hwnd, &rcWindow);

        // Set close button position
        RECT rcCloseButton = {
            rcWindow.right - CLOSE_BUTTON_WIDTH,
            rcWindow.top,
            rcWindow.right,
            rcWindow.top + CLOSE_BUTTON_HEIGHT
        };

        // Adjust rect to client coordinates
        OffsetRect(&rcCloseButton, -rcWindow.left, -rcWindow.top);

        // Draw the close button
        DrawCloseButton(hdc, rcCloseButton, hovered);

        ReleaseDC(hwnd, hdc);
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        // Handle hover effect on close button
        POINT ptMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        RECT rcCloseButton = {
            rcClient.right - CLOSE_BUTTON_WIDTH,
            rcClient.top,
            rcClient.right,
            rcClient.top + CLOSE_BUTTON_HEIGHT
        };

        BOOL isHovered = PtInRect(&rcCloseButton, ptMouse);
        if (isHovered != hovered) {
            hovered = isHovered;
            InvalidateRect(hwnd, NULL, TRUE);  // Trigger repaint to show hover effect
        }
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        // Handle close button click
        POINT ptMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        RECT rcCloseButton = {
            rcClient.right - CLOSE_BUTTON_WIDTH,
            rcClient.top,
            rcClient.right,
            rcClient.top + CLOSE_BUTTON_HEIGHT
        };

        if (PtInRect(&rcCloseButton, ptMouse)) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Entry point (WinMain)
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"CustomCloseButtonWindow";

    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"Window Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Create the window
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Custom Close Button Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Show the window
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Main message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
