#include "system_proxy.hpp"
#include <wininet.h>
#include <string>

#pragma comment(lib, "wininet.lib")

SystemProxyManager::SystemProxyManager() = default;

SystemProxyManager::~SystemProxyManager() {
    if (enabled_) {
        Disable();
    }
}

bool SystemProxyManager::ReadProxyState(ProxyState& state) {
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) return false;

    DWORD type = 0;
    DWORD size = sizeof(DWORD);
    RegQueryValueExW(hKey, L"ProxyEnable", nullptr, &type, reinterpret_cast<LPBYTE>(&state.proxyEnable), &size);

    wchar_t buffer[512];
    size = sizeof(buffer);
    if (RegQueryValueExW(hKey, L"ProxyServer", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &size) == ERROR_SUCCESS) {
        state.proxyServer = buffer;
    }

    size = sizeof(buffer);
    if (RegQueryValueExW(hKey, L"ProxyOverride", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &size) == ERROR_SUCCESS) {
        state.proxyOverride = buffer;
    }

    RegCloseKey(hKey);
    return true;
}

bool SystemProxyManager::WriteProxyState(const ProxyState& state) {
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_WRITE, &hKey);
    if (result != ERROR_SUCCESS) return false;

    RegSetValueExW(hKey, L"ProxyEnable", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&state.proxyEnable), sizeof(DWORD));

    if (!state.proxyServer.empty()) {
        RegSetValueExW(hKey, L"ProxyServer", 0, REG_SZ, reinterpret_cast<const BYTE*>(state.proxyServer.c_str()),
                       static_cast<DWORD>((state.proxyServer.length() + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(hKey, L"ProxyServer");
    }

    if (!state.proxyOverride.empty()) {
        RegSetValueExW(hKey, L"ProxyOverride", 0, REG_SZ, reinterpret_cast<const BYTE*>(state.proxyOverride.c_str()),
                       static_cast<DWORD>((state.proxyOverride.length() + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(hKey, L"ProxyOverride");
    }

    RegCloseKey(hKey);
    return true;
}

void SystemProxyManager::RefreshSystemProxy() {
    // Notify the system that proxy settings have changed
    InternetSetOptionW(nullptr, INTERNET_OPTION_SETTINGS_CHANGED, nullptr, 0);
    InternetSetOptionW(nullptr, INTERNET_OPTION_REFRESH, nullptr, 0);

    // Also notify WinHTTP (used by many modern apps including MS Store apps)
    // WinHttpSetDefaultProxyConfiguration is not public API, but the registry method above
    // plus InternetSetOption is sufficient for most applications.
}

bool SystemProxyManager::Enable(const std::wstring& proxyHost, int proxyPort) {
    if (enabled_) {
        Disable();
    }

    // Save original state
    if (!ReadProxyState(originalState_)) {
        return false;
    }

    // Build proxy server string
    std::wstring proxyServer = proxyHost + L":" + std::to_wstring(proxyPort);

    // Build bypass list (LAN addresses + localhost)
    // <local> is a Windows magic string that bypasses all local (intranet) addresses
    std::wstring bypassList = L"localhost;127.*;10.*;172.16.*;172.17.*;172.18.*;172.19.*;172.20.*;172.21.*;172.22.*;172.23.*;172.24.*;172.25.*;172.26.*;172.27.*;172.28.*;172.29.*;172.30.*;172.31.*;192.168.*;<local>";

    ProxyState newState;
    newState.proxyEnable = 1;
    newState.proxyServer = proxyServer;
    newState.proxyOverride = bypassList;

    if (!WriteProxyState(newState)) {
        return false;
    }

    RefreshSystemProxy();
    enabled_ = true;
    return true;
}

bool SystemProxyManager::Disable() {
    if (!enabled_) return true;

    if (!WriteProxyState(originalState_)) {
        return false;
    }

    RefreshSystemProxy();
    enabled_ = false;
    return true;
}
