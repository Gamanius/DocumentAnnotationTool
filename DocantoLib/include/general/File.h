#include "Common.h"
#include "Logger.h"

#ifndef _FILE_H_
#define _FILE_H_

namespace Docanto {
	struct File {
		std::unique_ptr<byte> data = nullptr;
		size_t size = 0;

		File() = default;
		File(std::unique_ptr<byte> data, size_t size) noexcept : data(std::move(data)), size(size) {}

		File(const File& other) = delete;
		File& operator=(const File& other) = delete;
		File(File&& other) noexcept;
		File& operator=(File&& other) noexcept;
		~File() = default;

		static std::optional<File> load(const std::filesystem::path& p);
		
	};
}

#endif // !_FILE_H_