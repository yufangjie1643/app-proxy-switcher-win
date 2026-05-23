#include "app_window.hpp"
#include "config_store.hpp"
#include "dpi_utils.hpp"
#include "app_finder.hpp"
#include "app_select_dialog.hpp"
#include "launcher.hpp"
#include "proxy_dialog.hpp"
#include "utils.hpp"
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <algorithm>
#include <string>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

#define IDC_BTN_NATIVE      201
#define IDC_BTN_PROXY       202
#define IDC_BTN_SWITCH_APP  301
#define IDC_BTN_MODE        302
#define IDC_BTN_DISABLE_SYS 303
#define IDC_BTN_PROXY_ADDR  304
#define IDC_BTN_CONFIG_DIR  305

namespace {

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

constexpr int kBaseClientWidth = 560;
constexpr int kBaseClientHeight = 420;

}

AppWindow::AppWindow() = default;
AppWindow::~AppWindow() {
    if (hFont_) {
        DeleteObject(hFont_);
        hFont_ = nullptr;
    }
}

bool AppWindow::Initialize(HINSTANCE hInstance, int nCmdShow) {
    hInst_ = hInstance;
    dpi_ = GetSystemDpi();

    INITCOMMONCONTROLSEX iccex = { sizeof(iccex), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&iccex);

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"ProxySwitcherMain";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassExW(&wc);

    DWORD exStyle = WS_EX_DLGMODALFRAME;
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    SIZE initialWindowSize = GetWindowSizeForClientDpi(
        Scale(kBaseClientWidth), Scale(kBaseClientHeight), style, exStyle, dpi_);

    hwnd_ = CreateWindowExW(
        exStyle,
        L"ProxySwitcherMain",
        L"Codex 代理启动器",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT, initialWindowSize.cx, initialWindowSize.cy,
        nullptr, nullptr, hInstance, this
    );

    if (!hwnd_) return false;
    UpdateDpi(GetWindowDpi(hwnd_));
    ResizeWindowForClientDpi(hwnd_, Scale(kBaseClientWidth), Scale(kBaseClientHeight), dpi_);

    configStore_ = std::make_unique<ConfigStore>();
    configStore_->Load();

    appFinder_ = std::make_unique<AppFinder>();
    UpdateAppDisplay();

    launcher_ = std::make_unique<Launcher>();

    OnCreate();
    RefreshStatus();

    ShowWindow(hwnd_, nCmdShow);
    UpdateWindow(hwnd_);

    if (configStore_->IsFirstLaunch()) {
        ShowProxyDialog();
    }

    return true;
}

void AppWindow::UpdateAppDisplay() {
    auto* appConfig = configStore_->GetCurrentApp();
    if (appConfig) {
        currentAppInfo_ = appFinder_->FindApp(*appConfig);
    } else {
        currentAppInfo_ = AppInstallInfo{};
    }
}

void AppWindow::OnCreate() {
    hwndLblApp_ = CreateWindowExW(0, L"STATIC", L"应用: (未选择)",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 0, 0, 0, hwnd_, nullptr, hInst_, nullptr);

    hwndLblProxy_ = CreateWindowExW(0, L"STATIC", L"代理: http://127.0.0.1:7897",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 0, 0, 0, hwnd_, nullptr, hInst_, nullptr);

    hwndLblMode_ = CreateWindowExW(0, L"STATIC", L"模式: 环境变量",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 0, 0, 0, hwnd_, nullptr, hInst_, nullptr);

    hwndLblPath_ = CreateWindowExW(0, L"STATIC", L"路径: 未检测到",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
        0, 0, 0, 0, hwnd_, nullptr, hInst_, nullptr);

    hwndBtnNative_ = CreateWindowExW(0, L"BUTTON", L"原生启动",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        0, 0, 0, 0, hwnd_, (HMENU)IDC_BTN_NATIVE, hInst_, nullptr);

    hwndBtnProxy_ = CreateWindowExW(0, L"BUTTON", L"代理启动",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        0, 0, 0, 0, hwnd_, (HMENU)IDC_BTN_PROXY, hInst_, nullptr);

    hwndBtnSwitchApp_ = CreateWindowExW(0, L"BUTTON", L"切换应用",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        0, 0, 0, 0, hwnd_, (HMENU)IDC_BTN_SWITCH_APP, hInst_, nullptr);

    hwndBtnMode_ = CreateWindowExW(0, L"BUTTON", L"切换模式",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        0, 0, 0, 0, hwnd_, (HMENU)IDC_BTN_MODE, hInst_, nullptr);

    hwndBtnDisableSys_ = CreateWindowExW(0, L"BUTTON", L"关闭系统代理",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        0, 0, 0, 0, hwnd_, (HMENU)IDC_BTN_DISABLE_SYS, hInst_, nullptr);

    hwndBtnProxyAddr_ = CreateWindowExW(0, L"BUTTON", L"配置代理",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        0, 0, 0, 0, hwnd_, (HMENU)IDC_BTN_PROXY_ADDR, hInst_, nullptr);

    hwndBtnConfigDir_ = CreateWindowExW(0, L"BUTTON", L"配置目录",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        0, 0, 0, 0, hwnd_, (HMENU)IDC_BTN_CONFIG_DIR, hInst_, nullptr);

    ApplyControlFont(hwndLblApp_);
    ApplyControlFont(hwndLblProxy_);
    ApplyControlFont(hwndLblMode_);
    ApplyControlFont(hwndLblPath_);
    ApplyControlFont(hwndBtnNative_);
    ApplyControlFont(hwndBtnProxy_);
    ApplyControlFont(hwndBtnSwitchApp_);
    ApplyControlFont(hwndBtnMode_);
    ApplyControlFont(hwndBtnDisableSys_);
    ApplyControlFont(hwndBtnProxyAddr_);
    ApplyControlFont(hwndBtnConfigDir_);

    LayoutControls();
    UpdateProxyButtonLabel();
}

int AppWindow::Scale(int value) const {
    return ScaleForDpi(value, dpi_);
}

void AppWindow::ApplyControlFont(HWND hwnd) {
    if (hwnd && hFont_) {
        SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(hFont_), TRUE);
    }
}

void AppWindow::UpdateDpi(unsigned int dpi) {
    dpi_ = dpi > 0 ? dpi : 96;

    if (hFont_) {
        DeleteObject(hFont_);
        hFont_ = nullptr;
    }

    int fontHeight = -MulDiv(9, static_cast<int>(dpi_), 72);
    hFont_ = CreateFontW(fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

    ApplyControlFont(hwndLblApp_);
    ApplyControlFont(hwndLblProxy_);
    ApplyControlFont(hwndLblMode_);
    ApplyControlFont(hwndLblPath_);
    ApplyControlFont(hwndBtnNative_);
    ApplyControlFont(hwndBtnProxy_);
    ApplyControlFont(hwndBtnSwitchApp_);
    ApplyControlFont(hwndBtnMode_);
    ApplyControlFont(hwndBtnDisableSys_);
    ApplyControlFont(hwndBtnProxyAddr_);
    ApplyControlFont(hwndBtnConfigDir_);
}

void AppWindow::LayoutControls() {
    if (!hwnd_) return;

    RECT rc;
    GetClientRect(hwnd_, &rc);
    int clientW = std::max(Scale(360), static_cast<int>(rc.right - rc.left));
    int clientH = std::max(Scale(kBaseClientHeight), static_cast<int>(rc.bottom - rc.top));

    int margin = Scale(16);
    int gap = Scale(12);
    int labelH = Scale(22);
    int pathMinH = Scale(52);
    int bigButtonH = Scale(48);
    int smallButtonH = Scale(32);
    int contentW = std::max(Scale(1), clientW - margin * 2);

    int y = margin;
    MoveWindow(hwndLblApp_, margin, y, contentW, labelH, TRUE);
    y += Scale(24);
    MoveWindow(hwndLblProxy_, margin, y, contentW, labelH, TRUE);
    y += Scale(24);
    MoveWindow(hwndLblMode_, margin, y, contentW, labelH, TRUE);
    y += Scale(24);

    int pathH = pathMinH;
    if (hwndLblPath_) {
        int textLen = GetWindowTextLengthW(hwndLblPath_);
        if (textLen > 0) {
            std::wstring text(static_cast<size_t>(textLen) + 1, L'\0');
            GetWindowTextW(hwndLblPath_, text.data(), textLen + 1);

            HDC hdc = GetDC(hwnd_);
            if (hdc) {
                HFONT oldFont = hFont_
                    ? reinterpret_cast<HFONT>(SelectObject(hdc, hFont_))
                    : nullptr;
                RECT calc = { 0, 0, contentW, 0 };
                DrawTextW(hdc, text.c_str(), -1, &calc,
                    DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
                if (oldFont) {
                    SelectObject(hdc, oldFont);
                }
                ReleaseDC(hwnd_, hdc);
                pathH = std::max(pathMinH, static_cast<int>(calc.bottom - calc.top) + Scale(8));
            }
        }
    }
    MoveWindow(hwndLblPath_, margin, y, contentW, pathH, TRUE);

    y += pathH + Scale(24);
    int bigW = (contentW - gap) / 2;
    MoveWindow(hwndBtnNative_, margin, y, bigW, bigButtonH, TRUE);
    MoveWindow(hwndBtnProxy_, margin + bigW + gap, y, contentW - bigW - gap, bigButtonH, TRUE);

    y += bigButtonH + Scale(20);
    int smallW = (contentW - gap) / 2;
    MoveWindow(hwndBtnSwitchApp_, margin, y, smallW, smallButtonH, TRUE);
    MoveWindow(hwndBtnMode_, margin + smallW + gap, y, contentW - smallW - gap, smallButtonH, TRUE);

    y += smallButtonH + Scale(12);
    MoveWindow(hwndBtnProxyAddr_, margin, y, smallW, smallButtonH, TRUE);
    MoveWindow(hwndBtnConfigDir_, margin + smallW + gap, y, contentW - smallW - gap, smallButtonH, TRUE);

    y += smallButtonH + Scale(12);
    if (hwndBtnDisableSys_) {
        bool disableVisible = IsWindowVisible(hwndBtnDisableSys_) != FALSE;
        MoveWindow(hwndBtnDisableSys_, margin, y, contentW, smallButtonH, TRUE);
        if (disableVisible) {
            y += smallButtonH + Scale(12);
        }
    }

    int requiredH = y + margin;
    if (requiredH > clientH) {
        ResizeWindowForClientDpi(hwnd_, clientW, requiredH, dpi_);
    }
}

void AppWindow::OnSize() {
    LayoutControls();
}

void AppWindow::UpdateProxyButtonLabel() {
    if (!hwndBtnProxy_) return;
    if (configStore_->GetProxyMode() == ProxyMode::EnvVar) {
        SetWindowTextW(hwndBtnProxy_, L"代理启动 (环境变量)");
    } else {
        SetWindowTextW(hwndBtnProxy_, L"代理启动 (系统代理)");
    }

    if (hwndBtnDisableSys_) {
        bool visible = launcher_->GetSystemProxyManager() && launcher_->GetSystemProxyManager()->IsEnabled();
        ShowWindow(hwndBtnDisableSys_, visible ? SW_SHOW : SW_HIDE);
        LayoutControls();
    }
}

LRESULT CALLBACK AppWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppWindow* pThis = nullptr;

    if (msg == WM_NCCREATE) {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pThis = static_cast<AppWindow*>(lpcs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->hwnd_ = hwnd;
    } else {
        pThis = reinterpret_cast<AppWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->ProcessMessage(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT AppWindow::ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            OnCommand(LOWORD(wParam));
            return 0;

        case WM_SIZE:
            OnSize();
            return 0;

        case WM_DPICHANGED: {
            unsigned int dpi = LOWORD(wParam);
            UpdateDpi(dpi);
            RECT* suggested = reinterpret_cast<RECT*>(lParam);
            if (suggested) {
                SetWindowPos(hwnd_, nullptr,
                    suggested->left, suggested->top,
                    suggested->right - suggested->left,
                    suggested->bottom - suggested->top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
            }
            LayoutControls();
            RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
            return 0;
        }

        case WM_GETMINMAXINFO: {
            auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
            DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd_, GWL_STYLE));
            DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd_, GWL_EXSTYLE));
            SIZE minSize = GetWindowSizeForClientDpi(
                Scale(kBaseClientWidth), Scale(kBaseClientHeight), style, exStyle, dpi_);
            mmi->ptMinTrackSize.x = minSize.cx;
            mmi->ptMinTrackSize.y = minSize.cy;
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void AppWindow::OnCommand(int id) {
    switch (id) {
        case IDC_BTN_NATIVE:
            LaunchNative();
            break;
        case IDC_BTN_PROXY:
            LaunchWithProxy();
            break;
        case IDC_BTN_SWITCH_APP:
            ShowAppSelectDialog();
            break;
        case IDC_BTN_MODE:
            ToggleProxyMode();
            break;
        case IDC_BTN_DISABLE_SYS:
            DisableSystemProxy();
            break;
        case IDC_BTN_PROXY_ADDR:
            ShowProxyDialog();
            break;
        case IDC_BTN_CONFIG_DIR:
            OpenConfigDir();
            break;
    }
}

void AppWindow::RefreshStatus() {
    auto* appConfig = configStore_->GetCurrentApp();
    std::wstring appName = appConfig ? appConfig->displayName : L"(未选择)";
    std::wstring appType = appConfig ? appConfig->GetTypeLabel() : L"";

    if (hwndLblApp_) {
        std::wstring text = L"应用: " + appName + L" [" + appType + L"]";
        if (currentAppInfo_.IsFound()) {
            text += L" (已找到)";
        } else {
            text += L" (未找到)";
        }
        SetWindowTextW(hwndLblApp_, text.c_str());
    }

    if (hwndLblProxy_) {
        std::wstring text = L"代理: " + FormatProxyUrl(
            configStore_->GetSettings().scheme,
            configStore_->GetSettings().host,
            configStore_->GetSettings().port);
        SetWindowTextW(hwndLblProxy_, text.c_str());
    }

    if (hwndLblMode_) {
        std::wstring text = L"模式: ";
        if (configStore_->GetProxyMode() == ProxyMode::EnvVar) {
            text += L"环境变量";
        } else {
            bool enabled = launcher_->GetSystemProxyManager() && launcher_->GetSystemProxyManager()->IsEnabled();
            text += enabled ? L"系统代理 (已激活)" : L"系统代理";
        }
        SetWindowTextW(hwndLblMode_, text.c_str());
    }

    if (hwndLblPath_) {
        std::wstring text = L"路径: ";
        if (currentAppInfo_.IsFound()) {
            text += currentAppInfo_.exePath;
        } else {
            text += L"未检测到";
        }
        SetWindowTextW(hwndLblPath_, text.c_str());
    }

    LayoutControls();
}

void AppWindow::LaunchNative() {
    if (!currentAppInfo_.IsFound()) {
        MessageBoxW(hwnd_, L"未找到目标应用程序。\n\n请使用\"切换应用\"选择一个有效的应用。",
            L"错误", MB_OK | MB_ICONWARNING);
        return;
    }

    auto result = launcher_->Launch(configStore_->GetSettings(), ProxyMode::EnvVar, currentAppInfo_);
    if (!result.success) {
        MessageBoxW(hwnd_, result.errorMessage.c_str(), L"启动失败", MB_OK | MB_ICONERROR);
    }
}

void AppWindow::LaunchWithProxy() {
    if (configStore_->GetSettings().port <= 0) {
        if (MessageBoxW(hwnd_, L"代理端口未配置，是否现在配置？", L"代理未设置",
            MB_YESNO | MB_ICONQUESTION) == IDYES) {
            ShowProxyDialog();
        }
        return;
    }

    if (!currentAppInfo_.IsFound()) {
        MessageBoxW(hwnd_, L"未找到目标应用程序。\n\n请使用\"切换应用\"选择一个有效的应用。",
            L"错误", MB_OK | MB_ICONWARNING);
        return;
    }

    auto mode = configStore_->GetProxyMode();
    auto result = launcher_->Launch(configStore_->GetSettings(), mode, currentAppInfo_);
    if (!result.success) {
        MessageBoxW(hwnd_, result.errorMessage.c_str(), L"启动失败", MB_OK | MB_ICONERROR);
    } else if (mode == ProxyMode::SystemProxy) {
        UpdateProxyButtonLabel();
        RefreshStatus();
    }
}

void AppWindow::ToggleProxyMode() {
    auto current = configStore_->GetProxyMode();
    auto next = (current == ProxyMode::EnvVar) ? ProxyMode::SystemProxy : ProxyMode::EnvVar;
    configStore_->SetProxyMode(next);
    configStore_->Save();

    if (next == ProxyMode::EnvVar) {
        MessageBoxW(hwnd_, L"已切换到环境变量模式。\n\n仅对当前启动的程序注入代理环境变量。",
            L"模式切换", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxW(hwnd_, L"已切换到系统代理模式。\n\n将修改 Windows Internet Options 系统代理设置，全局生效（绕过局域网）。",
            L"模式切换", MB_OK | MB_ICONINFORMATION);
    }

    UpdateProxyButtonLabel();
    RefreshStatus();
}

void AppWindow::DisableSystemProxy() {
    if (launcher_->GetSystemProxyManager()) {
        launcher_->GetSystemProxyManager()->Disable();
        UpdateProxyButtonLabel();
        RefreshStatus();
        MessageBoxW(hwnd_, L"系统代理已关闭", L"提示", MB_OK | MB_ICONINFORMATION);
    }
}

void AppWindow::ShowProxyDialog() {
    std::wstring currentAddress = FormatProxyUrl(
        configStore_->GetSettings().scheme,
        configStore_->GetSettings().host,
        configStore_->GetSettings().port);

    std::wstring newAddress;
    if (ProxyDialog::Show(hwnd_, currentAddress, newAddress)) {
        size_t schemeEnd = newAddress.find(L"://");
        if (schemeEnd != std::wstring::npos) {
            configStore_->GetSettings().scheme = newAddress.substr(0, schemeEnd);
            std::wstring rest = newAddress.substr(schemeEnd + 3);
            size_t colonPos = rest.find_last_of(L':');
            if (colonPos != std::wstring::npos) {
                configStore_->GetSettings().host = rest.substr(0, colonPos);
                try {
                    configStore_->GetSettings().port = std::stoi(rest.substr(colonPos + 1));
                } catch (...) {}
            }
        }
        configStore_->Save();
        RefreshStatus();
    }
}

void AppWindow::ShowAppSelectDialog() {
    AppSelectDialog::Show(hwnd_, *configStore_);
    UpdateAppDisplay();
    RefreshStatus();
}

void AppWindow::OpenConfigDir() {
    std::wstring dir = configStore_->GetConfigDirectory();
    if (dir.empty() || !DirectoryExists(dir)) {
        dir = GetExecutableDirectory();
    }
    ShellExecuteW(nullptr, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
}

int AppWindow::RunMessageLoop() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(hwnd_, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return static_cast<int>(msg.wParam);
}
