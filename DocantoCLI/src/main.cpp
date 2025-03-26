import Docanto;

import <string>;
import <iostream>;

using namespace Docanto;

bool running = true;

int main() {
	std::wostream&  out = Logger::get_stream();
	auto f = FileHandler::load_file(L"../pdf_tests/1.txt");
	std::wstring input;
	while (running) {
		out << "> ";

		if (!std::getline(std::wcin, input)) {
			break;
		}
	}
	return 0;
}