#ifndef _IMAGE_H_
#define _IMAGE_H_

#include "Common.h"
#include "File.h"
#include "MathHelper.h"

namespace Docanto {
	struct Image : public File {
		Geometry::Dimension<size_t> dims = {};
		size_t stride = 0;
		size_t components = 0;
		size_t  dpi = 0;

		Image() = default;
		Image(std::unique_ptr<byte> data, size_t size, size_t width);

		Image(const Image&) = delete;
		Image& operator=(const Image&) = delete;

		Image(Image&& other) noexcept;
		Image& operator=(Image&& other) noexcept;
	};
}


#endif // !_IMAGE_H_