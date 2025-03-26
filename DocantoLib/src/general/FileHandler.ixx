export module Docanto:FileHandler;
import :Common;

import <optional>;
import <string>;
import <memory>;
import <functional>;

namespace Docanto::FileHandler {
	export struct File {
		std::shared_ptr<byte> data = nullptr;
		size_t size = 0;

		File() = default;
		File(std::shared_ptr<byte> data, size_t size);

		// Dont copy data as it could be very expensive
		File(const File& other) = delete;
		File& operator=(File other) = delete;

		File(File&& other) noexcept;
		File& operator=(File&& t) noexcept;

		~File();
	};


	export std::optional<File> load_file(std::wstring filepath);
}