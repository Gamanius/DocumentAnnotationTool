#pragma once

#include <Windows.h>
#include <optional>
#include <string>
#include "Math.h"
#include "../General/General.h"

#ifndef _FILE_HANDLER_H_
#define _FILE_HANDLER_H_

namespace FileHandler {
	struct File {
		byte* data = nullptr;
		size_t size = 0;

		File() = default;
		File(byte* data, size_t size) : data(data), size(size) {}
		File(File&& t) noexcept {
			data = t.data;
			t.data = nullptr;

			size = t.size;
			t.size = 0;
		}

		// Dont copy data as it could be very expensive
		File(const File& t) = delete;
		File& operator=(const File& t) = delete;

		File& operator=(File&& t) noexcept {
			if (this != &t) {
				data = t.data;
				t.data = nullptr;

				size = t.size;
				t.size = 0;
			}
			return *this;
		}

		~File() {
			delete data;
		}
	};


	/// <summary>
	/// Will create an open file dialog where only *one* file can be selected
	/// </summary>
	/// <param name="filter">A file extension filter. Example "Bitmaps (*.bmp)\0*.bmp\0All files (*.*)\0*.*\0\0"</param>
	/// <param name="windowhandle">Optional windowhandle</param>
	/// <returns>If succeful it will return the path of the file that was selected</returns>
	std::optional<std::wstring> open_file_dialog(const wchar_t* filter, HWND windowhandle = NULL);

	std::optional<std::wstring> save_file_dialog(const wchar_t* filter, HWND windowhandle = NULL);

	/// <summary>
	/// Will read the contents of the file specified by the path
	/// </summary>
	/// <param name="path">Filepath of the file</param>
	/// <returns>If succefull it will return a file struct with the data</returns>
	std::optional<File> open_file(const std::wstring& path);

	bool write_file(byte* data, size_t amount, const std::wstring& path, bool overwrite = true);

	/// <summary>
	/// Returns size of the bitmap
	/// </summary>
	/// <param name="path">Path of the bitmap</param>
	/// <returns>A rectangle with the size</returns>
	std::optional<Math::Rectangle<DWORD>> get_bitmap_size(const std::wstring& path);
}

#endif // FILEHANDLER_H
