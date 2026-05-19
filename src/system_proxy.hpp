#pragma once
#include <string>
#include <windows.h>

class SystemProxyManager {
public:
    SystemProxyManager();
    ~SystemProxyManager();

    // Enable system proxy with the given URL (e.g. "127.0.0.1:7897")
    // Automatically bypasses LAN addresses (localhost, 10.*, 172.16-31.*, 192.168.*)
    bool Enable(const std::wstring& proxyHost, int proxyPort);

    // Restore previous system proxy settings
    bool Disable();

    // Check if we currently have system proxy enabled
    bool IsEnabled() const { return enabled_; }

private:
    static constexpr wchar_t* REG_PATH = L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings";

    struct ProxyState {
        DWORD proxyEnable = 0;
        std::wstring proxyServer;
        std::wstring proxyOverride;
    };

    bool ReadProxyState(ProxyState& state);
    bool WriteProxyState(const ProxyState& state);
    void RefreshSystemProxy();

    ProxyState originalState_;
    bool enabled_ = false;
};
