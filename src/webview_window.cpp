#include "webview_window.hpp"
#include "app_select_dialog.hpp"
#include "config_store.hpp"
#include "dpi_utils.hpp"
#include "app_finder.hpp"
#include "launcher.hpp"
#include "proxy_detector.hpp"
#include "proxy_dialog.hpp"
#include "utils.hpp"
#include "webview_ui.hpp"
#include <algorithm>
#include <string>
#include <windows.h>
#include <shellapi.h>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace {

constexpr int kBaseClientWidth = 980;
constexpr int kBaseClientHeight = 600;

std::wstring JsonEscape(const std::wstring& value) {
    std::wstring out;
    out.reserve(value.size() + 16);
    for (wchar_t ch : value) {
        switch (ch) {
            case L'\\': out += L"\\\\"; break;
            case L'"': out += L"\\\""; break;
            case L'\b': out += L"\\b"; break;
            case L'\f': out += L"\\f"; break;
            case L'\n': out += L"\\n"; break;
            case L'\r': out += L"\\r"; break;
            case L'\t': out += L"\\t"; break;
            default:
                if (ch < 0x20) {
                    wchar_t buffer[7] = {};
                    swprintf_s(buffer, L"\\u%04x", ch);
                    out += buffer;
                } else {
                    out += ch;
                }
                break;
        }
    }
    return out;
}

std::wstring JsonStringField(const std::wstring& json, const std::wstring& key) {
    std::wstring pattern = L"\"" + key + L"\"";
    size_t keyPos = json.find(pattern);
    if (keyPos == std::wstring::npos) return L"";
    size_t colon = json.find(L':', keyPos + pattern.size());
    if (colon == std::wstring::npos) return L"";
    size_t quote = json.find(L'"', colon + 1);
    if (quote == std::wstring::npos) return L"";

    std::wstring value;
    bool escaping = false;
    for (size_t i = quote + 1; i < json.size(); ++i) {
        wchar_t ch = json[i];
        if (escaping) {
            switch (ch) {
                case L'"': value += L'"'; break;
                case L'\\': value += L'\\'; break;
                case L'n': value += L'\n'; break;
                case L'r': value += L'\r'; break;
                case L't': value += L'\t'; break;
                default: value += ch; break;
            }
            escaping = false;
        } else if (ch == L'\\') {
            escaping = true;
        } else if (ch == L'"') {
            break;
        } else {
            value += ch;
        }
    }
    return value;
}

std::wstring BoolJson(bool value) {
    return value ? L"true" : L"false";
}

void EnsureDirectory(const std::wstring& path) {
    if (path.empty()) return;
    CreateDirectoryW(path.c_str(), nullptr);
}

}

WebViewWindow::WebViewWindow() = default;
WebViewWindow::~WebViewWindow() = default;

bool WebViewWindow::Initialize(HINSTANCE hInstance, int nCmdShow) {
    hInst_ = hInstance;
    unsigned int dpi = GetSystemDpi();

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"ProxySwitcherWebView";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassExW(&wc);

    DWORD exStyle = WS_EX_APPWINDOW;
    DWORD style = WS_OVERLAPPEDWINDOW;
    SIZE initialWindowSize = GetWindowSizeForClientDpi(
        ScaleForDpi(kBaseClientWidth, dpi), ScaleForDpi(kBaseClientHeight, dpi), style, exStyle, dpi);

    hwnd_ = CreateWindowExW(
        exStyle,
        L"ProxySwitcherWebView",
        L"Codex 代理启动器 WebUI",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT, initialWindowSize.cx, initialWindowSize.cy,
        nullptr, nullptr, hInstance, this);

    if (!hwnd_) return false;

    InitializeCore();
    ShowWindow(hwnd_, nCmdShow);
    UpdateWindow(hwnd_);
    InitializeWebView();
    return true;
}

void WebViewWindow::InitializeCore() {
    configStore_ = std::make_unique<ConfigStore>();
    configStore_->Load();
    appFinder_ = std::make_unique<AppFinder>();
    launcher_ = std::make_unique<Launcher>();
    UpdateAppDisplay();
}

void WebViewWindow::InitializeWebView() {
    std::wstring userDataDir = configStore_->GetConfigDirectory() + L"\\WebView2";
    EnsureDirectory(configStore_->GetConfigDirectory());
    EnsureDirectory(userDataDir);

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        userDataDir.c_str(),
        nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result) || !env) {
                    MessageBoxW(hwnd_, L"无法初始化 WebView2 Runtime。\n\n请安装 Microsoft Edge WebView2 Runtime 后重试。",
                        L"WebView2 初始化失败", MB_OK | MB_ICONERROR);
                    return result;
                }
                env->CreateCoreWebView2Controller(
                    hwnd_,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this](HRESULT controllerResult, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(controllerResult) || !controller) {
                                MessageBoxW(hwnd_, L"无法创建 WebView2 控件。", L"WebView2 初始化失败", MB_OK | MB_ICONERROR);
                                return controllerResult;
                            }

                            webviewController_ = controller;
                            webviewController_->get_CoreWebView2(&webview_);
                            ResizeWebView();

                            ComPtr<ICoreWebView2Settings> settings;
                            if (SUCCEEDED(webview_->get_Settings(&settings)) && settings) {
                                settings->put_AreDefaultContextMenusEnabled(FALSE);
                                settings->put_AreDevToolsEnabled(FALSE);
                                settings->put_IsStatusBarEnabled(FALSE);
                            }

                            EventRegistrationToken token = {};
                            webview_->add_WebMessageReceived(
                                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                    [this](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                        LPWSTR raw = nullptr;
                                        if (SUCCEEDED(args->get_WebMessageAsJson(&raw)) && raw) {
                                            std::wstring json = raw;
                                            CoTaskMemFree(raw);
                                            HandleWebMessage(json);
                                        }
                                        return S_OK;
                                    }).Get(),
                                &token);

                            std::wstring html = BuildWebViewHtml();
                            webview_->NavigateToString(html.c_str());
                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());

    if (FAILED(hr)) {
        MessageBoxW(hwnd_, L"无法启动 WebView2 Runtime。", L"WebView2 初始化失败", MB_OK | MB_ICONERROR);
    }
}

void WebViewWindow::ResizeWebView() {
    if (!webviewController_) return;
    RECT bounds = {};
    GetClientRect(hwnd_, &bounds);
    webviewController_->put_Bounds(bounds);
}

void WebViewWindow::HandleWebMessage(const std::wstring& json) {
    std::wstring command = JsonStringField(json, L"command");
    if (command == L"ready" || command == L"refresh") {
        UpdateAppDisplay();
        PostStatus();
    } else if (command == L"rescan") {
        UpdateAppDisplay();
        lastActionText_ = currentAppInfo_.IsFound()
            ? L"重新扫描完成：已找到目标程序。"
            : L"重新扫描完成：仍未找到。请点击“切换应用”手动选择 exe。";
        PostToast(lastActionText_);
        PostStatus();
    } else if (command == L"launchNative") {
        LaunchNative();
    } else if (command == L"launchProxy") {
        LaunchWithProxy();
    } else if (command == L"checkProxy") {
        CheckProxyAvailabilityAndSuggest();
    } else if (command == L"switchMode") {
        ToggleProxyMode();
    } else if (command == L"disableSystemProxy") {
        DisableSystemProxy();
    } else if (command == L"configureProxy") {
        ShowProxyDialog();
    } else if (command == L"switchApp") {
        ShowAppSelectDialog();
    } else if (command == L"openConfigDir") {
        OpenConfigDir();
    }
}

void WebViewWindow::PostStatus() {
    if (!webview_) return;
    auto* appConfig = configStore_->GetCurrentApp();
    std::wstring appName = appConfig ? appConfig->displayName : L"(未选择)";
    std::wstring appType = appConfig ? appConfig->GetTypeLabel() : L"";
    std::wstring pathText = currentAppInfo_.IsFound() ? currentAppInfo_.exePath : L"未检测到";
    std::wstring proxyUrl = FormatProxyUrl(
        configStore_->GetSettings().scheme,
        configStore_->GetSettings().host,
        configStore_->GetSettings().port);

    bool systemProxyEnabled = launcher_->GetSystemProxyManager() && launcher_->GetSystemProxyManager()->IsEnabled();
    bool envMode = configStore_->GetProxyMode() == ProxyMode::EnvVar;
    std::wstring modeText = envMode ? L"环境变量" : (systemProxyEnabled ? L"系统代理 (已激活)" : L"系统代理");
    std::wstring proxyButtonText = envMode ? L"代理启动 (环境变量)" : L"代理启动 (系统代理)";

    std::wstring json = L"{"
        L"\"type\":\"status\","
        L"\"appName\":\"" + JsonEscape(appName) + L"\","
        L"\"appType\":\"" + JsonEscape(appType) + L"\","
        L"\"proxyUrl\":\"" + JsonEscape(proxyUrl) + L"\","
        L"\"modeText\":\"" + JsonEscape(modeText) + L"\","
        L"\"pathText\":\"" + JsonEscape(pathText) + L"\","
        L"\"proxyCheckText\":\"" + JsonEscape(proxyCheckText_) + L"\","
        L"\"lastActionText\":\"" + JsonEscape(lastActionText_) + L"\","
        L"\"proxyButtonText\":\"" + JsonEscape(proxyButtonText) + L"\","
        L"\"found\":" + BoolJson(currentAppInfo_.IsFound()) + L","
        L"\"envMode\":" + BoolJson(envMode) + L","
        L"\"systemProxyActive\":" + BoolJson(systemProxyEnabled) + L","
        L"\"canDisableSystemProxy\":" + BoolJson(systemProxyEnabled) +
        L"}";
    webview_->PostWebMessageAsJson(json.c_str());
}

void WebViewWindow::PostToast(const std::wstring& message) {
    if (!webview_) return;
    std::wstring json = L"{\"type\":\"toast\",\"message\":\"" + JsonEscape(message) + L"\"}";
    webview_->PostWebMessageAsJson(json.c_str());
}

void WebViewWindow::UpdateAppDisplay() {
    auto* appConfig = configStore_->GetCurrentApp();
    currentAppInfo_ = appConfig ? appFinder_->FindApp(*appConfig) : AppInstallInfo{};
}

void WebViewWindow::LaunchNative() {
    if (!currentAppInfo_.IsFound()) {
        lastActionText_ = L"启动失败：未找到目标程序。请重新扫描、切换应用或手动选择 exe。";
        PostToast(lastActionText_);
        PostStatus();
        return;
    }
    auto result = launcher_->Launch(configStore_->GetSettings(), ProxyMode::EnvVar, currentAppInfo_);
    if (!result.success) {
        lastActionText_ = L"启动失败：" + result.errorMessage;
        PostToast(lastActionText_);
    } else {
        lastActionText_ =
            L"已原生启动：" + currentAppInfo_.displayName +
            L"。未注入代理环境变量。";
        PostToast(L"已原生启动");
    }
    PostStatus();
}

void WebViewWindow::LaunchWithProxy() {
    if (configStore_->GetSettings().port <= 0) {
        lastActionText_ = L"代理端口未配置，请先填写代理地址。";
        ShowProxyDialog();
        return;
    }
    if (!currentAppInfo_.IsFound()) {
        lastActionText_ = L"启动失败：未找到目标程序。请重新扫描、切换应用或手动选择 exe。";
        PostToast(lastActionText_);
        PostStatus();
        return;
    }
    auto mode = configStore_->GetProxyMode();
    auto result = launcher_->Launch(configStore_->GetSettings(), mode, currentAppInfo_);
    if (!result.success) {
        lastActionText_ = L"启动失败：" + result.errorMessage;
        PostToast(lastActionText_);
    } else {
        std::wstring proxyUrl = FormatProxyUrl(
            configStore_->GetSettings().scheme,
            configStore_->GetSettings().host,
            configStore_->GetSettings().port);
        if (mode == ProxyMode::EnvVar) {
            lastActionText_ =
                L"已使用环境变量代理启动：" + currentAppInfo_.displayName +
                L"。HTTP_PROXY / HTTPS_PROXY / ALL_PROXY = " + proxyUrl +
                L"；NO_PROXY = localhost,127.0.0.1,::1。";
        } else {
            lastActionText_ =
                L"已使用系统代理启动：" + currentAppInfo_.displayName +
                L"。系统代理会影响全局网络，用完请点击“关闭系统代理”。";
        }
        PostToast(mode == ProxyMode::EnvVar ? L"已使用环境变量代理启动" : L"已使用系统代理启动");
    }
    PostStatus();
}

void WebViewWindow::ToggleProxyMode() {
    auto current = configStore_->GetProxyMode();
    auto next = (current == ProxyMode::EnvVar) ? ProxyMode::SystemProxy : ProxyMode::EnvVar;
    configStore_->SetProxyMode(next);
    configStore_->Save();
    lastActionText_ = next == ProxyMode::EnvVar
        ? L"已切换到环境变量模式：安全，仅影响从本工具启动的程序。"
        : L"已切换到系统代理模式：会影响全局网络，用完请关闭系统代理。";
    PostToast(lastActionText_);
    PostStatus();
}

void WebViewWindow::DisableSystemProxy() {
    if (launcher_->GetSystemProxyManager()) {
        launcher_->GetSystemProxyManager()->Disable();
        lastActionText_ = L"系统代理已关闭。";
        PostToast(lastActionText_);
        PostStatus();
    }
}

void WebViewWindow::CheckProxyAvailabilityAndSuggest() {
    auto& settings = configStore_->GetSettings();
    auto result = CheckProxyAvailability(settings);
    proxyCheckText_ = DescribeProxyScanResult(settings, result);
    lastActionText_ = proxyCheckText_;

    if (!result.currentOpen && !result.openPorts.empty()) {
        int suggestedPort = result.openPorts.front();
        std::wstring suggestedUrl = L"http://127.0.0.1:" + std::to_wstring(suggestedPort);
        std::wstring message =
            proxyCheckText_ + L"\n\n"
            L"是否切换到推荐地址：\n" + suggestedUrl + L" ?";
        if (MessageBoxW(hwnd_, message.c_str(), L"发现可用代理端口", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            settings.scheme = L"http";
            settings.host = L"127.0.0.1";
            settings.port = suggestedPort;
            configStore_->Save();
            proxyCheckText_ = L"已切换到可用代理：" + suggestedUrl;
            lastActionText_ = proxyCheckText_;
        }
    }

    PostToast(proxyCheckText_);
    PostStatus();
}

void WebViewWindow::ShowProxyDialog() {
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
        proxyCheckText_ = L"代理地址已更新，建议重新点击“检测代理”。";
        lastActionText_ = L"代理配置已保存。";
        PostToast(L"代理配置已保存");
        PostStatus();
    }
}

void WebViewWindow::ShowAppSelectDialog() {
    AppSelectDialog::Show(hwnd_, *configStore_);
    UpdateAppDisplay();
    lastActionText_ = currentAppInfo_.IsFound()
        ? L"已切换应用并找到目标程序。"
        : L"已切换应用，但当前未找到目标程序。可重新扫描或手动选择 exe。";
    PostStatus();
}

void WebViewWindow::OpenConfigDir() {
    std::wstring dir = configStore_->GetConfigDirectory();
    if (dir.empty() || !DirectoryExists(dir)) {
        dir = GetExecutableDirectory();
    }
    ShellExecuteW(nullptr, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
}

LRESULT CALLBACK WebViewWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WebViewWindow* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pThis = static_cast<WebViewWindow*>(lpcs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->hwnd_ = hwnd;
    } else {
        pThis = reinterpret_cast<WebViewWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    return pThis ? pThis->ProcessMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT WebViewWindow::ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SIZE:
            ResizeWebView();
            return 0;
        case WM_DPICHANGED: {
            RECT* suggested = reinterpret_cast<RECT*>(lParam);
            if (suggested) {
                SetWindowPos(hwnd, nullptr, suggested->left, suggested->top,
                    suggested->right - suggested->left, suggested->bottom - suggested->top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
            }
            ResizeWebView();
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WebViewWindow::RunMessageLoop() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
