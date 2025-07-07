#include "win/MainWindowHandler.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <cstdlib>

#include <iostream>

using namespace DocantoWin;

void exit_func() {
	Docanto::Logger::print_to_debug();
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	AllocConsole();
	FILE* fpstdout;
	freopen_s(&fpstdout, "CONOUT$", "w", stdout);
	Docanto::Logger::init(&std::wcout);

	std::atexit(exit_func);

	MainWindowHandler window(hInstance);
	window.run();

	return 0;
}