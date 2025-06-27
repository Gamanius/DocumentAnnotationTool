#include "Image.h"

Docanto::Image::Image(std::unique_ptr<byte> data, size_t size, size_t width) : File(std::move(data), size) {
	dims = { size, width };
}

Docanto::Image::Image(Image&& other) noexcept : File(std::move(other)) {
	dims = std::move(other.dims);
	stride = std::move(other.stride);
	components = std::move(other.components);
	dpi = std::move(other.dpi);
}

Docanto::Image& Docanto::Image::operator=(Image&& other) noexcept {
	if (this != &other) {
		File::operator=(std::move(other));

		dims = std::move(other.dims);
		stride = std::move(other.stride);
		components = std::move(other.components);
		dpi = std::move(other.dpi);
	}

	return *this;
}