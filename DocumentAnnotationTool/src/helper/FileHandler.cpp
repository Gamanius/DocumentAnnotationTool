#include "include.h"

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

std::optional<FileHandler::File> FileHandler::open_file(const std::wstring& path) {
    HANDLE hFile = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ASSERT_WIN_RETURN_NULLOPT(hFile != INVALID_HANDLE_VALUE, "Could not open file " + std::string(path.begin(), path.end()));

    DWORD fileSize = GetFileSize(hFile, NULL); 
    ASSERT_WIN_WITH_STATEMENT(fileSize != INVALID_FILE_SIZE, 
        "Could not get file size of file " + std::string(path.begin(), path.end()), 
        CloseHandle(hFile); return std::nullopt; );
    
    File file; 
    file.data = new byte[fileSize]; 
    file.size = fileSize; 

    ASSERT_WIN_WITH_STATEMENT(ReadFile(hFile, file.data, fileSize, NULL, NULL), 
        		"Could not read file " + std::string(path.begin(), path.end()),  
        		CloseHandle(hFile); delete file.data; return std::nullopt; ); 

    CloseHandle(hFile);
    return std::move(file); 
}

std::optional<Renderer::Rectangle<DWORD>> FileHandler::get_bitmap_size(const std::wstring& path) {
    HBITMAP hBitmap = (HBITMAP)LoadImage(NULL, path.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);

    ASSERT_WIN_RETURN_NULLOPT(hBitmap != NULL, "Could not load bitmap " + std::string(path.begin(), path.end()));

    BITMAP bm;
    ASSERT_WIN_WITH_STATEMENT(GetObject(hBitmap, sizeof(bm), &bm), 
        		"Could not get bitmap size of file " + std::string(path.begin(), path.end()),  
        		DeleteObject(hBitmap); return std::nullopt; );

    return Renderer::Rectangle<DWORD>(0, 0, bm.bmWidth, bm.bmHeight);
}
