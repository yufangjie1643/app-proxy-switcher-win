#include "proxy_dialog.hpp"
#include "resource.h"
#include <windows.h>
#include <string>
#include <algorithm>

struct DialogData {
    std::wstring currentAddress;
    std::wstring resultAddress;
};

bool ProxyDialog::ParseAddress(const std::wstring& input, std::wstring& outHost, int& outPort, std::wstring& outScheme, std::wstring& error) {
    std::wstring trimmed = input;
    auto start = trimmed.find_first_not_of(L" \t\r\n");
    if (start == std::wstring::npos) {
        error = L"地址不能为空";
        return false;
    }
    auto end = trimmed.find_last_not_of(L" \t\r\n");
    trimmed = trimmed.substr(start, end - start + 1);

    std::wstring scheme = L"http";
    std::wstring rest = trimmed;

    size_t schemePos = trimmed.find(L"://");
    if (schemePos != std::wstring::npos) {
        scheme = trimmed.substr(0, schemePos);
        rest = trimmed.substr(schemePos + 3);
    }

    size_t colonPos = rest.find_last_of(L':');
    if (colonPos == std::wstring::npos) {
        error = L"格式错误，需要包含端口号 (如 127.0.0.1:7897)";
        return false;
    }

    std::wstring host;
    std::wstring portStr;
    if (rest[0] == L'[') {
        size_t bracketEnd = rest.find(L']');
        if (bracketEnd == std::wstring::npos || bracketEnd + 1 >= rest.length() || rest[bracketEnd + 1] != L':') {
            error = L"IPv6 地址格式错误，应为 [::1]:7890";
            return false;
        }
        host = rest.substr(1, bracketEnd - 1);
        portStr = rest.substr(bracketEnd + 2);
    } else {
        host = rest.substr(0, colonPos);
        portStr = rest.substr(colonPos + 1);
    }

    if (host.empty()) {
        error = L"主机地址不能为空";
        return false;
    }

    if (portStr.empty()) {
        error = L"端口号不能为空";
        return false;
    }

    try {
        size_t idx = 0;
        int port = std::stoi(portStr, &idx);
        if (idx != portStr.length()) {
            error = L"端口号必须是数字";
            return false;
        }
        if (port < 1 || port > 65535) {
            error = L"端口号必须在 1-65535 之间";
            return false;
        }
        outHost = host;
        outPort = port;
        outScheme = scheme;
        return true;
    } catch (...) {
        error = L"端口号格式错误";
        return false;
    }
}

INT_PTR CALLBACK ProxyDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            DialogData* data = reinterpret_cast<DialogData*>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
            SetDlgItemTextW(hwnd, IDC_EDIT_ADDRESS, data->currentAddress.c_str());
            HWND parent = GetParent(hwnd);
            if (parent) {
                RECT rcParent, rcDlg;
                GetWindowRect(parent, &rcParent);
                GetWindowRect(hwnd, &rcDlg);
                int x = rcParent.left + (rcParent.right - rcParent.left - (rcDlg.right - rcDlg.left)) / 2;
                int y = rcParent.top + (rcParent.bottom - rcParent.top - (rcDlg.bottom - rcDlg.top)) / 2;
                SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
            return TRUE;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == IDOK) {
                wchar_t buffer[256];
                GetDlgItemTextW(hwnd, IDC_EDIT_ADDRESS, buffer, 256);
                std::wstring input = buffer;

                std::wstring host, scheme, error;
                int port = 0;
                if (!ParseAddress(input, host, port, scheme, error)) {
                    SetDlgItemTextW(hwnd, IDC_STATIC_ERR, error.c_str());
                    return TRUE;
                }

                DialogData* data = reinterpret_cast<DialogData*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
                data->resultAddress = scheme + L"://" + host + L":" + std::to_wstring(port);
                EndDialog(hwnd, IDOK);
                return TRUE;
            } else if (id == IDCANCEL) {
                EndDialog(hwnd, IDCANCEL);
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}

bool ProxyDialog::Show(HWND parent, const std::wstring& currentAddress, std::wstring& outAddress) {
    DialogData data = { currentAddress, L"" };
    INT_PTR result = DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_PROXY),
        parent, DialogProc, reinterpret_cast<LPARAM>(&data));
    if (result == IDOK) {
        outAddress = data.resultAddress;
        return true;
    }
    return false;
}
