#include "dpi_utils.hpp"
#include <algorithm>

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif

namespace {

unsigned int NormalizeDpi(unsigned int dpi) {
    return dpi > 0 ? dpi : 96;
}

BOOL AdjustRectForDpi(RECT& rc, DWORD style, DWORD exStyle, unsigned int dpi) {
    using AdjustWindowRectExForDpiFn = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    auto adjustForDpi = user32
        ? reinterpret_cast<AdjustWindowRectExForDpiFn>(GetProcAddress(user32, "AdjustWindowRectExForDpi"))
        : nullptr;
    if (adjustForDpi) {
        return adjustForDpi(&rc, style, FALSE, exStyle, NormalizeDpi(dpi));
    }
    return AdjustWindowRectEx(&rc, style, FALSE, exStyle);
}

}

void EnablePerMonitorDpiAwareness() {
    using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    auto setContext = user32
        ? reinterpret_cast<SetProcessDpiAwarenessContextFn>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"))
        : nullptr;
    if (setContext && setContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        return;
    }

    using SetProcessDpiAwarenessFn = HRESULT(WINAPI*)(int);
    HMODULE shcore = LoadLibraryW(L"shcore.dll");
    auto setAwareness = shcore
        ? reinterpret_cast<SetProcessDpiAwarenessFn>(GetProcAddress(shcore, "SetProcessDpiAwareness"))
        : nullptr;
    if (setAwareness) {
        setAwareness(2);
    }
    if (shcore) {
        FreeLibrary(shcore);
    }
}

unsigned int GetSystemDpi() {
    HDC hdc = GetDC(nullptr);
    if (!hdc) return 96;
    int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(nullptr, hdc);
    return dpi > 0 ? static_cast<unsigned int>(dpi) : 96;
}

unsigned int GetWindowDpi(HWND hwnd) {
    using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    auto getDpiForWindow = user32
        ? reinterpret_cast<GetDpiForWindowFn>(GetProcAddress(user32, "GetDpiForWindow"))
        : nullptr;
    if (getDpiForWindow && hwnd) {
        return getDpiForWindow(hwnd);
    }
    return GetSystemDpi();
}

int ScaleForDpi(int value, unsigned int dpi) {
    return MulDiv(value, static_cast<int>(NormalizeDpi(dpi)), 96);
}

SIZE GetWindowSizeForClientDpi(int clientWidth, int clientHeight, DWORD style, DWORD exStyle, unsigned int dpi) {
    RECT rc = {
        0,
        0,
        std::max(0, clientWidth),
        std::max(0, clientHeight)
    };
    AdjustRectForDpi(rc, style, exStyle, dpi);
    SIZE size = { rc.right - rc.left, rc.bottom - rc.top };
    return size;
}

void ResizeWindowForClientDpi(HWND hwnd, int clientWidth, int clientHeight, unsigned int dpi) {
    if (!hwnd) return;
    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
    DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
    SIZE size = GetWindowSizeForClientDpi(clientWidth, clientHeight, style, exStyle, dpi);
    SetWindowPos(hwnd, nullptr, 0, 0, size.cx, size.cy,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}
