#include <iostream>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <cstdlib>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "DocantoLib.h"

using namespace Docanto;

int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Logger::init(&std::wcout);

	auto a = std::make_shared<PDF>("C:/repos/Docanto/pdf_tests/Ghent_PDF_Output_Suite_V50_Testpages/GhentPDFOutputSuite50_ReadMes.pdf");
	PDFRenderer render(a);
	auto i = render.get_image(0);

	stbi_write_png("test.png", i.dims.width, i.dims.height, i.components, (void*)(i.data.get()), i.stride);

	Logger::print_to_debug();
}