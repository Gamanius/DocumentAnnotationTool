#pragma once

#include "helper/include.h"

#define APPLICATION_NAME L"Docanto"		

void memory_leak_check() {
	auto allocated_bytes = MemoryWatcher::get_allocated_bytes();
	// check if all bytes have been deallocated
	ASSERT(!allocated_bytes, "Not all bytes have been deallocated :/");
}

void init() {
	std::atexit(memory_leak_check);
	Logger::init();
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	init();
	auto b = AllocConsole();
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);

	auto window = WindowHandler(APPLICATION_NAME, hInstance);
	window.set_state(WindowHandler::WINDOW_STATE::NORMAL);

	auto render = Direct2DRenderer(&window);
	window.set_renderer((Renderer::RenderHandler*)&render);
	
	while (!window.close_request()) { 
		WindowHandler::get_window_messages(true);
		render.draw_text(L"Hello ma man", { 0.0f, 0.0f }, L"Arial", 50);
	}
	
	Logger::print_to_console(handle);
	Logger::clear();
}