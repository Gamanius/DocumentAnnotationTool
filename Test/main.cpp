#include <Windows.h>

int WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    char* pCmdLine,
    int nCmdShow
) {
	MessageBoxA(NULL, pCmdLine, "Title", MB_OK);
	return 0;
}