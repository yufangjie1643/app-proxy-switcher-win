#pragma once
#include <windows.h>
#include <string>

class ProxyDialog {
public:
    // Input/output full proxy address like "127.0.0.1:7897" or "http://192.168.1.100:8080"
    static bool Show(HWND parent, const std::wstring& currentAddress, std::wstring& outAddress);

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static bool ParseAddress(const std::wstring& input, std::wstring& outHost, int& outPort, std::wstring& outScheme, std::wstring& error);
};
