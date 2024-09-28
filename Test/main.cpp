#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <string>

#pragma comment(lib, "wininet.lib")

std::string HttpRequest(const std::string& url) {
    HINTERNET hInternet = InternetOpen(L"GitHubReleaseChecker", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        std::cerr << "Failed to initialize WinINet." << std::endl;
        return "";
    }

    HINTERNET hConnect = InternetOpenUrl(hInternet, std::wstring(url.begin(), url.end()).c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        std::cerr << "Failed to open URL." << std::endl;
        InternetCloseHandle(hInternet);
        return "";
    }

    char buffer[1024];
    DWORD bytesRead;
    std::string response;

    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead != 0) {
        response.append(buffer, bytesRead);
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return response;
}

int main() {
    // GitHub API endpoint for latest release
    std::string url = "https://api.github.com/repos/Gamanius/DocumentAnnotationTool/releases/latest";

    std::string response = HttpRequest(url);

    if (!response.empty()) {
        std::cout << "GitHub API Response:\n" << response << std::endl;
        // You can now parse the JSON response using any JSON parsing library (like nlohmann/json)
    }
    else {
        std::cerr << "Failed to fetch the release info." << std::endl;
    }

    return 0;
}
