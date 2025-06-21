#include "Common.h"
#include "Logger.h"

namespace Docanto {
	struct File {
		std::unique_ptr<byte> data = nullptr;
		size_t size = 0;

		File() = default;
		File(const File& other) = delete;
		File& operator=(const File& other) = delete;
		File(File&& other) noexcept;
		File& operator=(File&& other) noexcept;
		~File() = default;

		static std::optional<File> load(std::filesystem::path& p);
		
	};
}