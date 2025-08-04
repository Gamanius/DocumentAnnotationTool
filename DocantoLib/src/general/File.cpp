#include "File.h"

Docanto::File::File(Docanto::File&& other) noexcept {
	std::swap(data, other.data);
	this->size = other.size;
	this->path = std::move(other.path);

	other.data = nullptr;
	other.path = std::filesystem::path();
	other.size = 0;
}

Docanto::File& Docanto::File::operator=(File&& other) noexcept {
	std::swap(data, other.data);
	this->size = other.size;
	this->path = std::move(other.path);

	other.data = nullptr;
	other.path = std::filesystem::path();
	other.size = 0;

	return *this;
}

std::optional<Docanto::File> Docanto::File::load(const std::filesystem::path& p) {
	if (!std::filesystem::exists(p)) {
		// Log error: file does not exist
		Logger::error("File does not exist at path: ", p);
		return std::nullopt;
	}
	if (!std::filesystem::is_regular_file(p)) {
		// Log error: path is not a regular file (e.g., it's a directory)
		Logger::error("Path is not a File (is it a directory?): ", p);
		return std::nullopt;
	}


	std::ifstream stream(p, std::ios::binary | std::ios::ate);

	// Check if the stream opened successfully
	if (!stream.is_open()) {
		// Log error: failed to open file
		Logger::error("Failed to open file at path: ", p);
		return std::nullopt;
	}

	auto size = stream.tellg();
	auto file_buffer = std::unique_ptr<byte>(new byte[size]);

	stream.seekg(0, std::ios::beg);
	if (!stream.read(reinterpret_cast<char*>(file_buffer.get()), size)) {
		Logger::error("Failed to read file: ", p);
		return std::nullopt;
	}

	Docanto::File file;
	file.data = std::move(file_buffer);
	file.size = size;
	file.path = p;


	return std::move(file);
}

void Docanto::File::write(const std::filesystem::path& p, const byte* data, size_t len) {
	if (std::filesystem::is_directory(p)) {
		Logger::error("Wanted to save a file at a directory: ", p);
		return;
	}

	std::ofstream stream(p, std::ios::binary);

	if (!stream.is_open()) {
		// Log error: failed to open file
		Logger::error("Failed to open stream to path: ", p);
		return;
	}

	stream.write(reinterpret_cast<const char*>(data), len);
	stream.close();
}

