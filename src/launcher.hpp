#pragma once
#include <string>
#include <memory>
#include "app_config.hpp"
#include "config_store.hpp"
#include "proxy_mode.hpp"
#include "system_proxy.hpp"

struct LaunchResult {
    bool success = false;
    std::wstring errorMessage;

    static LaunchResult Ok() { return {true, L""}; }
    static LaunchResult Fail(const std::wstring& msg) { return {false, msg}; }
};

class Launcher {
public:
    Launcher();
    ~Launcher();

    LaunchResult Launch(const ProxySettings& settings, ProxyMode mode, const AppInstallInfo& info);
    bool StopApp(const std::wstring& processName);

    SystemProxyManager* GetSystemProxyManager() { return sysProxy_.get(); }

private:
    bool StartAppDirect(const AppInstallInfo& info, const std::wstring& proxyUrl, const std::wstring& noProxy);
    bool StartAppViaActivation(const AppInstallInfo& info);
    void* BuildEnvironmentBlock(const std::wstring& proxyUrl, const std::wstring& noProxy);
    void FreeEnvironmentBlock(void* block);
    std::wstring GetProcessNameFromExePath(const std::wstring& exePath);

    std::unique_ptr<SystemProxyManager> sysProxy_;
};
