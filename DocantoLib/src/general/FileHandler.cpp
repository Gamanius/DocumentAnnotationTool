module Docanto;
import :FileHandler;
import :Logger;

using namespace Docanto::FileHandler;

import <filesystem>;
import <fstream>;


File::File(std::shared_ptr<byte> data, size_t size) : data(data), size(size) {
	
}

File::File(File&& t) noexcept {

	data = t.data;
	t.data = nullptr;

	size = t.size;
	t.size = 0;
}

File& File::operator=(File&& t) noexcept {
	if (this != &t) {
		data = t.data;
		t.data = nullptr;

		size = t.size;
		t.size = 0;
	}
	return *this;
}

File::~File() {
}


std::optional<File> Docanto::FileHandler::load_file(std::wstring filepath) {
	std::ifstream file_in;
	file_in.open(filepath, std::ios_base::in | std::ios_base::binary | std::ios_base::ate);

	if (!file_in.is_open()) {
		file_in.close();
		Logger::error(L"Couldn't open file \"", filepath, L"\"");
		return std::nullopt;
	}
	auto length = file_in.tellg();
	std::shared_ptr<byte> d(new byte[length]);

	file_in.seekg(0);
	file_in.read(reinterpret_cast<char*>(d.get()), length);
	file_in.close();

	Logger::log(L"Loaded file at filepath \"", filepath, "\" with ", length, " bytes");

	return std::move(File(d, length));
}