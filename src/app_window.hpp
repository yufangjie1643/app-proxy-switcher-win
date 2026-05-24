#pragma once
#include <windows.h>
#include <memory>
#include <string>
#include "app_config.hpp"
#include "proxy_mode.hpp"

class ConfigStore;
class AppFinder;
class Launcher;

class AppWindow {
public:
    AppWindow();
    ~AppWindow();

    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    int RunMessageLoop();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void OnCreate();
    void OnSize();
    void OnCommand(int id);

    void LayoutControls();
    void UpdateDpi(unsigned int dpi);
    int Scale(int value) const;
    void ApplyControlFont(HWND hwnd);
    void RefreshStatus();
    void UpdateAppDisplay();
    void LaunchNative();
    void LaunchWithProxy();
    void ShowProxyDialog();
    void ShowAppSelectDialog();
    void OpenConfigDir();
    void ToggleProxyMode();
    void DisableSystemProxy();
    void CheckProxyAvailabilityAndSuggest();
    void UpdateProxyButtonLabel();

    HWND hwnd_ = nullptr;
    HINSTANCE hInst_ = nullptr;

    std::unique_ptr<ConfigStore> configStore_;
    std::unique_ptr<AppFinder> appFinder_;
    std::unique_ptr<Launcher> launcher_;

    AppInstallInfo currentAppInfo_;
    unsigned int dpi_ = 96;
    HFONT hFont_ = nullptr;

    // Standard Win32 controls
    HWND hwndLblApp_ = nullptr;
    HWND hwndLblProxy_ = nullptr;
    HWND hwndLblPath_ = nullptr;
    HWND hwndLblMode_ = nullptr;
    HWND hwndBtnNative_ = nullptr;
    HWND hwndBtnProxy_ = nullptr;
    HWND hwndBtnSwitchApp_ = nullptr;
    HWND hwndBtnMode_ = nullptr;
    HWND hwndBtnDisableSys_ = nullptr;
    HWND hwndBtnProxyAddr_ = nullptr;
    HWND hwndBtnCheckProxy_ = nullptr;
    HWND hwndBtnConfigDir_ = nullptr;
};
