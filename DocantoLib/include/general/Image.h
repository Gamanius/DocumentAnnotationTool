#ifndef _IMAGE_H_
#define _IMAGE_H_

#include "Common.h"
#include "File.h"
#include "MathHelper.h"

namespace Docanto {
	struct Image : public File {
		Geometry::Dimension<size_t> dims;
		size_t stride;
		size_t components;
		size_t  dpi;

		Image() = default;
		Image(std::unique_ptr<byte> data, size_t size, size_t width);

		Image(Image&& other) noexcept;
		Image& operator=(Image&& other) noexcept;
	};
}


#endif // !_IMAGE_H_