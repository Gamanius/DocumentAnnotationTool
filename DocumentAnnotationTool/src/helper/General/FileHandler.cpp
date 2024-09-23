#include "FileHandler.h"
#include <ShObjIdl.h>
#include <KnownFolders.h>

std::optional<std::wstring> FileHandler::open_file_dialog(const wchar_t* filter, HWND windowhandle) {

    OPENFILENAME ofn;
    WCHAR szFile[MAX_PATH] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = windowhandle; // If you have a window handle, specify it here.
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ENABLESIZING;

    if (GetOpenFileName(&ofn)) {
        return std::wstring(ofn.lpstrFile);
    }
    auto error = CommDlgExtendedError();
    ASSERT_WIN(error == 0, "Unknown file dialog error with comm error " + std::to_string(error));
    return std::nullopt;
}

std::optional<std::wstring> FileHandler::save_file_dialog(const wchar_t* filter, HWND windowhandle) {
    OPENFILENAME ofn;
    WCHAR szFile[MAX_PATH] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = windowhandle; // If you have a window handle, specify it here.
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT;

    if (GetSaveFileName(&ofn)) {
        return std::wstring(ofn.lpstrFile);
    }
    auto error = CommDlgExtendedError();
    ASSERT_WIN(error == 0, "Unknown file dialog error with comm error " + std::to_string(error));
    return std::nullopt;
}

std::optional<FileHandler::File> FileHandler::open_file(const std::wstring& path) {
    HANDLE hFile = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        Logger::warn("Could not open file ", path);
        return std::nullopt;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    ASSERT_WIN_WITH_STATEMENT(fileSize != INVALID_FILE_SIZE,
        CloseHandle(hFile); return std::nullopt; ,
        "Could not get file size of file ", path);

    File file;
    file.data = new byte[fileSize];
    file.size = fileSize;

    ASSERT_WIN_WITH_STATEMENT(ReadFile(hFile, file.data, fileSize, NULL, NULL),
        CloseHandle(hFile); delete file.data; return std::nullopt;,
        "Could not read file ", path);

    CloseHandle(hFile);
    return std::move(file);
}

std::optional<FileHandler::File> FileHandler::open_file_appdata(const std::wstring& path) {
    return open_file(FileHandler::get_appdata_path() / path);
}

bool FileHandler::write_file(byte* data, size_t amount, const std::wstring& path, bool overwrite) {
    HANDLE h = CreateFile(path.c_str(), GENERIC_WRITE, 0, NULL, overwrite ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    ASSERT_WIN_RETURN_FALSE(h != INVALID_HANDLE_VALUE, "Could not create file ", path);
    bool succ = WriteFile(h, reinterpret_cast<void*>(data), static_cast<DWORD>(amount), NULL, NULL);

    CloseHandle(h);
    return succ;
}

bool FileHandler::write_file_to_appdata(const File& f, std::filesystem::path path, bool overwrite) {
    auto file_path = get_appdata_path() / path;
    HANDLE h = CreateFile(file_path.c_str(), GENERIC_WRITE, 0, NULL, overwrite ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	ASSERT_WIN_RETURN_FALSE(h != INVALID_HANDLE_VALUE, "Could not create file ", path); 
	bool succ = WriteFile(h, reinterpret_cast<void*>(f.data), static_cast<DWORD>(f.size), NULL, NULL);

	CloseHandle(h);
	return succ;
}

std::filesystem::path FileHandler::get_appdata_path() {
    auto hr = CoInitialize(NULL);

    IKnownFolderManager* pKnownFolderManager = nullptr;
    hr = CoCreateInstance(CLSID_KnownFolderManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pKnownFolderManager));
    ASSERT_WIN(hr == S_OK, "Failed to create instance of IKnownFolderManager");

    IKnownFolder* pKnownFolder = nullptr;
    PWSTR pszPath = nullptr;
    hr = pKnownFolderManager->GetFolder(FOLDERID_RoamingAppData, &pKnownFolder);
    ASSERT_WIN(hr == S_OK, "Failed to get Roaming AppData folder");

    hr = pKnownFolder->GetPath(0, &pszPath);
    ASSERT_WIN(hr == S_OK, "Failed to get path of Roaming AppData folder");

    std::filesystem::path path(pszPath);

    CoTaskMemFree(pszPath);
    pKnownFolder->Release();
    pKnownFolderManager->Release();
    CoUninitialize();

    path = path / APPLICATION_NAME;
    if (!std::filesystem::exists(path)) {
	    auto result = std::filesystem::create_directories(path);
		ASSERT(result, "Could not create Roaming Dir: ", path);
    }

    return path;
}

std::optional<Math::Rectangle<DWORD>> FileHandler::get_bitmap_size(const std::wstring& path) {
    HBITMAP hBitmap = (HBITMAP)LoadImage(NULL, path.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    ASSERT_WIN_RETURN_NULLOPT(hBitmap != NULL, "Could not load bitmap " + std::string(path.begin(), path.end()));

    BITMAP bm;
    ASSERT_WIN_WITH_STATEMENT(GetObject(hBitmap, sizeof(bm), &bm),
        DeleteObject(hBitmap); return std::nullopt;,
        "Could not get bitmap size of file ", path);

    return Math::Rectangle<DWORD>(0, 0, bm.bmWidth, bm.bmHeight);
}
