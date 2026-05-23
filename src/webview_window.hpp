#pragma once
#include <windows.h>
#include <memory>
#include <string>
#include <wrl.h>
#include "WebView2.h"
#include "app_config.hpp"
#include "proxy_mode.hpp"

class ConfigStore;
class AppFinder;
class Launcher;
struct ICoreWebView2;
struct ICoreWebView2Controller;

class WebViewWindow {
public:
    WebViewWindow();
    ~WebViewWindow();

    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    int RunMessageLoop();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void InitializeCore();
    void InitializeWebView();
    void ResizeWebView();
    void HandleWebMessage(const std::wstring& json);
    void PostStatus();
    void PostToast(const std::wstring& message);
    void UpdateAppDisplay();
    void LaunchNative();
    void LaunchWithProxy();
    void ToggleProxyMode();
    void DisableSystemProxy();
    void ShowProxyDialog();
    void ShowAppSelectDialog();
    void OpenConfigDir();

    HWND hwnd_ = nullptr;
    HINSTANCE hInst_ = nullptr;

    std::unique_ptr<ConfigStore> configStore_;
    std::unique_ptr<AppFinder> appFinder_;
    std::unique_ptr<Launcher> launcher_;
    AppInstallInfo currentAppInfo_;

    Microsoft::WRL::ComPtr<ICoreWebView2Controller> webviewController_;
    Microsoft::WRL::ComPtr<ICoreWebView2> webview_;
};
