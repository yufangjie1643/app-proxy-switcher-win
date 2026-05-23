#include <windows.h>
#include <commctrl.h>
#include "dpi_utils.hpp"
#include "webview_window.hpp"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    EnablePerMonitorDpiAwareness();

    INITCOMMONCONTROLSEX iccex = { sizeof(iccex), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&iccex);

    WebViewWindow app;
    if (!app.Initialize(hInstance, nCmdShow)) {
        MessageBoxW(nullptr, L"初始化 WebUI 应用程序失败", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    return app.RunMessageLoop();
}
