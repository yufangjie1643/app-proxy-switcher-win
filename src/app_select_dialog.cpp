#include "app_select_dialog.hpp"
#include "config_store.hpp"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string>
#include <chrono>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

namespace {

unsigned int GetSystemDpiForDialog() {
    HDC hdc = GetDC(nullptr);
    if (!hdc) return 96;
    int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(nullptr, hdc);
    return dpi > 0 ? static_cast<unsigned int>(dpi) : 96;
}

}

struct AppSelectData {
    ConfigStore* store;
};

void AppSelectDialog::RefreshList(HWND hwndList, ConfigStore& store) {
    SendMessageW(hwndList, LB_RESETCONTENT, 0, 0);

    for (const auto& app : store.GetApps()) {
        std::wstring item = app.displayName + L" [" + app.GetTypeLabel() + L"]";
        if (app.id == store.GetCurrentAppId()) {
            item += L" *";
        }
        int idx = static_cast<int>(SendMessageW(hwndList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str())));
        SendMessageW(hwndList, LB_SETITEMDATA, idx, reinterpret_cast<LPARAM>(&app));
    }
}

void AppSelectDialog::OnAddExe(HWND hwnd, ConfigStore& store) {
    wchar_t filePath[MAX_PATH] = {};

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"\u53ef\u6267\u884c\u6587\u4ef6 (*.exe)\0*.exe\0\u6240\u6709\u6587\u4ef6 (*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"\u9009\u62e9\u53ef\u6267\u884c\u6587\u4ef6";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        std::wstring path = filePath;
        std::wstring name = path.substr(path.find_last_of(L"\\/") + 1);
        size_t dotPos = name.find_last_of(L'.');
        if (dotPos != std::wstring::npos) {
            name = name.substr(0, dotPos);
        }

        store.AddExeApp(name, path);
        store.Save();

        HWND hwndList = GetDlgItem(hwnd, IDC_LIST_APPS);
        RefreshList(hwndList, store);
    }
}

static bool ShowInputDialog(HWND parent, const wchar_t* title, const wchar_t* label1, const wchar_t* default1,
                            const wchar_t* label2, const wchar_t* default2,
                            std::wstring& out1, std::wstring& out2) {
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"InputDlg";
    RegisterClassExW(&wc);

    unsigned int dpi = GetSystemDpiForDialog();
    auto scale = [dpi](int value) { return MulDiv(value, static_cast<int>(dpi), 96); };

    HWND hwndDlg = CreateWindowExW(WS_EX_DLGMODALFRAME,
        L"InputDlg", title,
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        0, 0, scale(340), scale(180), parent, nullptr, GetModuleHandleW(nullptr), nullptr);

    HFONT hFont = CreateFontW(-MulDiv(9, static_cast<int>(dpi), 72), 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

    RECT rcParent, rcDlg;
    GetWindowRect(parent, &rcParent);
    GetWindowRect(hwndDlg, &rcDlg);
    SetWindowPos(hwndDlg, nullptr,
        rcParent.left + (rcParent.right - rcParent.left - (rcDlg.right - rcDlg.left)) / 2,
        rcParent.top + (rcParent.bottom - rcParent.top - (rcDlg.bottom - rcDlg.top)) / 2,
        0, 0, SWP_NOSIZE);

    HWND hwndLabel1 = CreateWindowExW(0, L"STATIC", label1,
        WS_CHILD | WS_VISIBLE, scale(10), scale(10), scale(90), scale(18), hwndDlg, nullptr, nullptr, nullptr);
    HWND hwndEdit1 = CreateWindowExW(0, L"EDIT", default1,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, scale(100), scale(8), scale(210), scale(24), hwndDlg, nullptr, nullptr, nullptr);

    HWND hwndLabel2 = CreateWindowExW(0, L"STATIC", label2,
        WS_CHILD | WS_VISIBLE, scale(10), scale(44), scale(90), scale(18), hwndDlg, nullptr, nullptr, nullptr);
    HWND hwndEdit2 = CreateWindowExW(0, L"EDIT", default2,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, scale(100), scale(42), scale(210), scale(24), hwndDlg, nullptr, nullptr, nullptr);

    HWND hwndOK = CreateWindowExW(0, L"BUTTON", L"\u786e\u5b9a",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, scale(170), scale(104), scale(60), scale(28), hwndDlg, (HMENU)IDOK, nullptr, nullptr);
    HWND hwndCancel = CreateWindowExW(0, L"BUTTON", L"\u53d6\u6d88",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, scale(240), scale(104), scale(60), scale(28), hwndDlg, (HMENU)IDCANCEL, nullptr, nullptr);

    for (HWND control : { hwndLabel1, hwndEdit1, hwndLabel2, hwndEdit2, hwndOK, hwndCancel }) {
        if (control && hFont) {
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        }
    }

    BOOL ret;
    MSG msg;
    bool done = false;
    bool confirmed = false;
    while (!done && (ret = GetMessageW(&msg, nullptr, 0, 0)) != 0) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            confirmed = true;
            done = true;
        } else if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            done = true;
        }
        if (msg.message == WM_COMMAND && LOWORD(msg.wParam) == IDOK) {
            confirmed = true;
            done = true;
        }
        if (msg.message == WM_COMMAND && LOWORD(msg.wParam) == IDCANCEL) {
            done = true;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    bool result = false;
    if (confirmed) {
        wchar_t buf1[256], buf2[256];
        GetWindowTextW(hwndEdit1, buf1, 256);
        GetWindowTextW(hwndEdit2, buf2, 256);
        if (wcslen(buf1) > 0 && wcslen(buf2) > 0) {
            out1 = buf1;
            out2 = buf2;
            result = true;
        }
    }

    DestroyWindow(hwndDlg);
    if (hFont) {
        DeleteObject(hFont);
    }
    return result;
}

void AppSelectDialog::OnAddMsix(HWND hwnd, ConfigStore& store) {
    std::wstring name, pkg;
    if (ShowInputDialog(hwnd, L"\u6dfb\u52a0 MSIX \u5e94\u7528", L"\u663e\u793a\u540d\u79f0:", L"Claude Desktop",
                        L"\u5305\u540d:", L"Anthropic.Claude", name, pkg)) {
        store.AddDefaultApp(name, pkg, AppType::Msix);
        store.Save();
        HWND hwndList = GetDlgItem(hwnd, IDC_LIST_APPS);
        RefreshList(hwndList, store);
    }
}

void AppSelectDialog::OnAddNpm(HWND hwnd, ConfigStore& store) {
    std::wstring name, pkg;
    if (ShowInputDialog(hwnd, L"\u6dfb\u52a0 NPM CLI", L"\u663e\u793a\u540d\u79f0:", L"Codex CLI",
                        L"\u5305\u540d:", L"codex", name, pkg)) {
        AppConfig app;
        app.id = std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count());
        app.displayName = name;
        app.type = AppType::Npm;
        app.packageName = pkg;
        app.useProxy = true;
        store.GetApps().push_back(app);
        store.Save();
        HWND hwndList = GetDlgItem(hwnd, IDC_LIST_APPS);
        RefreshList(hwndList, store);
    }
}

void AppSelectDialog::OnAddVsCode(HWND hwnd, ConfigStore& store) {
    std::wstring name, extId;
    if (ShowInputDialog(hwnd, L"\u6dfb\u52a0 VSCode \u6269\u5c55", L"\u663e\u793a\u540d\u79f0:", L"Codex (VSCode)",
                        L"\u6269\u5c55 ID:", L"openai.codex-vscode", name, extId)) {
        AppConfig app;
        app.id = std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count());
        app.displayName = name;
        app.type = AppType::VsCodeExt;
        app.packageName = extId;
        app.useProxy = true;
        store.GetApps().push_back(app);
        store.Save();
        HWND hwndList = GetDlgItem(hwnd, IDC_LIST_APPS);
        RefreshList(hwndList, store);
    }
}

void AppSelectDialog::OnRemove(HWND hwnd, ConfigStore& store) {
    HWND hwndList = GetDlgItem(hwnd, IDC_LIST_APPS);
    int sel = static_cast<int>(SendMessageW(hwndList, LB_GETCURSEL, 0, 0));
    if (sel == LB_ERR) return;

    const AppConfig* app = reinterpret_cast<const AppConfig*>(SendMessageW(hwndList, LB_GETITEMDATA, sel, 0));
    if (!app) return;

    // Only protect the original MSIX default apps
    if (app->id == L"codex_default" || app->id == L"claude_default") {
        MessageBoxW(hwnd, L"\u4e0d\u80fd\u5220\u9664\u9884\u8bbe\u5e94\u7528", L"\u63d0\u793a", MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::wstring msg = L"\u786e\u5b9a\u8981\u5220\u9664\u5e94\u7528 \"" + app->displayName + L"\" \u5417\uff1f";
    if (MessageBoxW(hwnd, msg.c_str(), L"\u786e\u8ba4\u5220\u9664", MB_YESNO | MB_ICONQUESTION) == IDYES) {
        store.RemoveApp(app->id);
        store.Save();
        RefreshList(hwndList, store);
    }
}

void AppSelectDialog::OnSelect(HWND hwnd, ConfigStore& store) {
    HWND hwndList = GetDlgItem(hwnd, IDC_LIST_APPS);
    int sel = static_cast<int>(SendMessageW(hwndList, LB_GETCURSEL, 0, 0));
    if (sel == LB_ERR) return;

    const AppConfig* app = reinterpret_cast<const AppConfig*>(SendMessageW(hwndList, LB_GETITEMDATA, sel, 0));
    if (!app) return;

    store.SetCurrentAppId(app->id);
    store.Save();
    RefreshList(hwndList, store);
}

INT_PTR CALLBACK AppSelectDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            AppSelectData* data = reinterpret_cast<AppSelectData*>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));

            HWND hwndList = GetDlgItem(hwnd, IDC_LIST_APPS);
            RefreshList(hwndList, *data->store);

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
            AppSelectData* data = reinterpret_cast<AppSelectData*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (!data) return FALSE;

            switch (id) {
                case IDC_BTN_ADD_EXE:
                    OnAddExe(hwnd, *data->store);
                    return TRUE;
                case IDC_BTN_ADD_MSIX:
                    OnAddMsix(hwnd, *data->store);
                    return TRUE;
                case IDC_BTN_ADD_NPM:
                    OnAddNpm(hwnd, *data->store);
                    return TRUE;
                case IDC_BTN_ADD_VSCODE:
                    OnAddVsCode(hwnd, *data->store);
                    return TRUE;
                case IDC_BTN_REMOVE:
                    OnRemove(hwnd, *data->store);
                    return TRUE;
                case IDC_BTN_SELECT:
                    OnSelect(hwnd, *data->store);
                    return TRUE;
                case IDOK:
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    return TRUE;
            }
            break;
        }
    }
    return FALSE;
}

bool AppSelectDialog::Show(HWND parent, ConfigStore& configStore) {
    AppSelectData data = { &configStore };
    INT_PTR result = DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_APP_SELECT),
        parent, DialogProc, reinterpret_cast<LPARAM>(&data));
    return result == IDOK;
}
