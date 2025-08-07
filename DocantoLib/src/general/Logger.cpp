#include "Logger.h"

#include <iostream>

#ifdef _MSVC_LANG
	#ifndef _UNICODE
	#define _UNICODE
	#endif // !_UNICODE

	#ifndef UNICODE
	#define UNICODE
	#endif // !UNICODE

	#include <windows.h>
#endif // _MSVC_LANG 

void Docanto::Logger::init(std::wostream* buffer) {
	if (buffer == nullptr) {
		_internal_buffer = std::make_shared<std::wstringstream>();
		_msg_buffer = _internal_buffer.get();
		return;
	}
	_msg_buffer = buffer;
}

void Docanto::Logger::print_to_debug() {
#ifdef _MSVC_LANG 
	if (auto* ss = dynamic_cast<std::wstringstream*>(_msg_buffer)) {
		std::wstring content = ss->str();

		if (content.empty())
			return;

		OutputDebugString(content.c_str());
		OutputDebugString(L"\n");

	}
#endif // _MSVC_LANG 

}