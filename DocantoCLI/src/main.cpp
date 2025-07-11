#include <iostream>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <cstdlib>


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "DocantoLib.h"

#include <chrono>


#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.Foundation.Collections.h>
using namespace winrt::Windows::UI::ViewManagement;

using namespace Docanto; 
inline bool IsColorLight(winrt::Windows::UI::Color& clr) {
	return (((5 * clr.G) + (2 * clr.R) + clr.B) > (8 * 128));
}

using namespace std::literals::chrono_literals;
int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Logger::init(&std::wcout);
	
	winrt::init_apartment();
	auto settings = UISettings();

	auto foreground = settings.GetColorValue(UIColorType::Foreground);
	bool isDarkMode = static_cast<bool>(IsColorLight(foreground));
	{

		auto revoker = settings.ColorValuesChanged([&](auto&&...) {
			auto foregroundRevoker = settings.GetColorValue(UIColorType::Foreground);
			bool isDarkModeRevoker = static_cast<bool>(IsColorLight(foregroundRevoker));
			wprintf(L"isDarkModeRevoker: %d\n", isDarkModeRevoker);
			});
	}

	static bool s_go = true;
	while (s_go) {
		std::this_thread::sleep_for(2s);
	}

	Logger::log(isDarkMode);

	Logger::print_to_debug();
}