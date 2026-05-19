#pragma once
#include <string>
#include <vector>
#include "app_config.hpp"
#include "proxy_mode.hpp"

struct ProxySettings {
    std::wstring scheme = L"http";
    std::wstring host = L"127.0.0.1";
    int port = 7897;
    bool desktopShortcutPrompted = false;
    ProxyMode proxyMode = ProxyMode::EnvVar;
};

class ConfigStore {
public:
    ConfigStore();

    bool Load();
    bool Save();
    bool IsFirstLaunch() const { return firstLaunch_; }

    ProxySettings& GetSettings() { return settings_; }
    const ProxySettings& GetSettings() const { return settings_; }

    ProxyMode GetProxyMode() const { return settings_.proxyMode; }
    void SetProxyMode(ProxyMode mode) { settings_.proxyMode = mode; }

    std::vector<AppConfig>& GetApps() { return apps_; }
    const std::vector<AppConfig>& GetApps() const { return apps_; }

    const std::wstring& GetCurrentAppId() const { return currentAppId_; }
    void SetCurrentAppId(const std::wstring& id) { currentAppId_ = id; }

    AppConfig* FindApp(const std::wstring& id);
    AppConfig* GetCurrentApp();
    void AddDefaultApp(const std::wstring& displayName, const std::wstring& packageName, AppType type);
    void AddExeApp(const std::wstring& displayName, const std::wstring& exePath);
    void RemoveApp(const std::wstring& id);

    std::wstring GetConfigDirectory() const;

private:
    std::wstring GetSettingsPath() const;
    void InitDefaultApps();
    bool MigrateFromLegacy();
    bool MigrateFromProxyUrlFile();
    bool ExtractJsonStringField(const std::string& json, const std::string& key, std::string& out);
    bool ExtractJsonIntField(const std::string& json, const std::string& key, int& out);
    bool ExtractJsonBoolField(const std::string& json, const std::string& key, bool& out);

    std::wstring GenerateId();

    ProxySettings settings_;
    std::vector<AppConfig> apps_;
    std::wstring currentAppId_;
    bool firstLaunch_ = false;
    std::wstring configDir_;
};
