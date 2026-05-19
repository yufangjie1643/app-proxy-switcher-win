#include <windows.h>
#include <commctrl.h>
#include "app_window.hpp"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX iccex = { sizeof(iccex), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&iccex);

    AppWindow app;
    if (!app.Initialize(hInstance, nCmdShow)) {
        MessageBoxW(nullptr, L"初始化应用程序失败", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    return app.RunMessageLoop();
}
