import Docanto;

import <string>;
import <iostream>;

using namespace Docanto;

bool running = true;

int main() {
	std::wostream&  out = Logger::get_stream();

	std::wstring input;
	while (running) {
		out << "> ";

		if (!std::getline(std::wcin, input)) {
			break;
		}
		Logger::log(input, Logger::LEVEL::WARNING);
	}
	return 0;
}