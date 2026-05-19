#include "config_store.hpp"
#include "known_agents.hpp"
#include "utils.hpp"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <chrono>

ConfigStore::ConfigStore() {
    configDir_ = GetAppDataPath() + L"\\CodexProxySwitcher";
    if (!configDir_.empty() && !DirectoryExists(configDir_)) {
        CreateDirectoryW(configDir_.c_str(), nullptr);
    }
}

std::wstring ConfigStore::GetSettingsPath() const {
    return configDir_ + L"\\settings.ini";
}

std::wstring ConfigStore::GetConfigDirectory() const {
    return configDir_;
}

std::wstring ConfigStore::GenerateId() {
    static int counter = 0;
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::to_wstring(now) + L"_" + std::to_wstring(counter++);
}

void ConfigStore::InitDefaultApps() {
    apps_.clear();
    currentAppId_ = L"codex_default";
    EnsureKnownAgentApps(apps_, currentAppId_);
}

bool ConfigStore::Load() {
    std::wstring path = GetSettingsPath();

    if (FileExists(path)) {
        wchar_t buffer[512];

        // Proxy settings
        GetPrivateProfileStringW(L"Proxy", L"Scheme", L"http", buffer, 256, path.c_str());
        settings_.scheme = buffer;

        GetPrivateProfileStringW(L"Proxy", L"Host", L"127.0.0.1", buffer, 256, path.c_str());
        settings_.host = buffer;

        settings_.port = GetPrivateProfileIntW(L"Proxy", L"Port", 7897, path.c_str());
        settings_.desktopShortcutPrompted = GetPrivateProfileIntW(L"UI", L"DesktopShortcutPrompted", 0, path.c_str()) != 0;
        settings_.proxyMode = GetPrivateProfileIntW(L"Proxy", L"Mode", 0, path.c_str()) == 0 ? ProxyMode::EnvVar : ProxyMode::SystemProxy;

        // Current app
        GetPrivateProfileStringW(L"App", L"CurrentId", L"", buffer, 256, path.c_str());
        currentAppId_ = buffer;

        // App list
        int appCount = GetPrivateProfileIntW(L"AppList", L"Count", 0, path.c_str());
        apps_.clear();

        for (int i = 0; i < appCount; i++) {
            std::wstring section = L"App" + std::to_wstring(i);

            AppConfig app;
            GetPrivateProfileStringW(section.c_str(), L"Id", L"", buffer, 256, path.c_str());
            app.id = buffer;

            GetPrivateProfileStringW(section.c_str(), L"DisplayName", L"", buffer, 256, path.c_str());
            app.displayName = buffer;

            app.type = static_cast<AppType>(GetPrivateProfileIntW(section.c_str(), L"Type", 0, path.c_str()));

            GetPrivateProfileStringW(section.c_str(), L"PackageName", L"", buffer, 256, path.c_str());
            app.packageName = buffer;

            GetPrivateProfileStringW(section.c_str(), L"ExePath", L"", buffer, 512, path.c_str());
            app.exePath = buffer;

            app.useProxy = GetPrivateProfileIntW(section.c_str(), L"UseProxy", 1, path.c_str()) != 0;

            if (app.IsValid()) {
                apps_.push_back(app);
            }
        }

        if (apps_.empty()) {
            InitDefaultApps();
        } else {
            EnsureKnownAgentApps(apps_, currentAppId_);
            Save();
        }

        // Validate current app id
        if (FindApp(currentAppId_) == nullptr && !apps_.empty()) {
            currentAppId_ = apps_[0].id;
        }

        firstLaunch_ = false;
        return true;
    }

    // Try migration from legacy
    if (MigrateFromLegacy()) {
        InitDefaultApps();
        // Set current app to Codex
        for (auto& app : apps_) {
            if (app.packageName == L"OpenAI.Codex") {
                currentAppId_ = app.id;
                break;
            }
        }
        firstLaunch_ = false;
        Save();
        return true;
    }

    if (MigrateFromProxyUrlFile()) {
        InitDefaultApps();
        firstLaunch_ = false;
        Save();
        return true;
    }

    // First launch: init defaults
    InitDefaultApps();
    firstLaunch_ = true;
    return false;
}

bool ConfigStore::Save() {
    std::wstring path = GetSettingsPath();

    // Proxy settings
    WritePrivateProfileStringW(L"Proxy", L"Scheme", settings_.scheme.c_str(), path.c_str());
    WritePrivateProfileStringW(L"Proxy", L"Host", settings_.host.c_str(), path.c_str());

    std::wstring portStr = std::to_wstring(settings_.port);
    WritePrivateProfileStringW(L"Proxy", L"Port", portStr.c_str(), path.c_str());

    std::wstring promptedStr = settings_.desktopShortcutPrompted ? L"1" : L"0";
    WritePrivateProfileStringW(L"UI", L"DesktopShortcutPrompted", promptedStr.c_str(), path.c_str());

    std::wstring modeStr = settings_.proxyMode == ProxyMode::EnvVar ? L"0" : L"1";
    WritePrivateProfileStringW(L"Proxy", L"Mode", modeStr.c_str(), path.c_str());

    // Current app
    WritePrivateProfileStringW(L"App", L"CurrentId", currentAppId_.c_str(), path.c_str());

    // App list
    std::wstring countStr = std::to_wstring(static_cast<int>(apps_.size()));
    WritePrivateProfileStringW(L"AppList", L"Count", countStr.c_str(), path.c_str());

    for (size_t i = 0; i < apps_.size(); i++) {
        std::wstring section = L"App" + std::to_wstring(i);
        const auto& app = apps_[i];

        WritePrivateProfileStringW(section.c_str(), L"Id", app.id.c_str(), path.c_str());
        WritePrivateProfileStringW(section.c_str(), L"DisplayName", app.displayName.c_str(), path.c_str());
        WritePrivateProfileStringW(section.c_str(), L"Type", std::to_wstring(static_cast<int>(app.type)).c_str(), path.c_str());
        WritePrivateProfileStringW(section.c_str(), L"PackageName", app.packageName.c_str(), path.c_str());
        WritePrivateProfileStringW(section.c_str(), L"ExePath", app.exePath.c_str(), path.c_str());
        WritePrivateProfileStringW(section.c_str(), L"UseProxy", app.useProxy ? L"1" : L"0", path.c_str());
    }

    return true;
}

AppConfig* ConfigStore::FindApp(const std::wstring& id) {
    for (auto& app : apps_) {
        if (app.id == id) return &app;
    }
    return nullptr;
}

AppConfig* ConfigStore::GetCurrentApp() {
    return FindApp(currentAppId_);
}

void ConfigStore::AddDefaultApp(const std::wstring& displayName, const std::wstring& packageName, AppType type) {
    AppConfig app;
    app.id = GenerateId();
    app.displayName = displayName;
    app.type = type;
    app.packageName = packageName;
    app.useProxy = true;
    apps_.push_back(app);
}

void ConfigStore::AddExeApp(const std::wstring& displayName, const std::wstring& exePath) {
    AppConfig app;
    app.id = GenerateId();
    app.displayName = displayName;
    app.type = AppType::Exe;
    app.exePath = exePath;
    app.useProxy = true;
    apps_.push_back(app);
}

void ConfigStore::RemoveApp(const std::wstring& id) {
    for (auto it = apps_.begin(); it != apps_.end(); ++it) {
        if (it->id == id) {
            apps_.erase(it);
            break;
        }
    }
    if (currentAppId_ == id && !apps_.empty()) {
        currentAppId_ = apps_[0].id;
    }
}

bool ConfigStore::MigrateFromLegacy() {
    std::wstring legacyDir = GetAppDataPath() + L"\\CodexLauncher";
    std::wstring legacyPath = legacyDir + L"\\settings.json";

    if (!FileExists(legacyPath)) {
        return false;
    }

    std::ifstream file(legacyPath, std::ios::binary);
    if (!file) return false;

    std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    std::string scheme, host;
    int port = 0;
    bool prompted = false;

    if (ExtractJsonStringField(json, "proxyScheme", scheme)) {
        settings_.scheme = Utf8ToWide(scheme);
    }
    if (ExtractJsonStringField(json, "proxyHost", host)) {
        settings_.host = Utf8ToWide(host);
    }
    if (ExtractJsonIntField(json, "proxyPort", port)) {
        settings_.port = port;
    }
    ExtractJsonBoolField(json, "desktopShortcutPrompted", prompted);
    settings_.desktopShortcutPrompted = prompted;

    return true;
}

bool ConfigStore::MigrateFromProxyUrlFile() {
    std::wstring legacyDir = GetAppDataPath() + L"\\CodexLauncher";
    std::wstring proxyUrlPath = legacyDir + L"\\proxy-url.txt";

    if (!FileExists(proxyUrlPath)) {
        proxyUrlPath = configDir_ + L"\\proxy-url.txt";
        if (!FileExists(proxyUrlPath)) {
            return false;
        }
    }

    std::ifstream file(proxyUrlPath);
    if (!file) return false;

    std::string url;
    std::getline(file, url);
    url = Trim(url);

    if (url.empty()) return false;

    if (url.find("://") != std::string::npos) {
        size_t schemeEnd = url.find("://");
        settings_.scheme = Utf8ToWide(url.substr(0, schemeEnd));

        size_t hostStart = schemeEnd + 3;
        size_t colonPos = url.find(':', hostStart);
        if (colonPos != std::string::npos) {
            settings_.host = Utf8ToWide(url.substr(hostStart, colonPos - hostStart));
            std::string portStr = url.substr(colonPos + 1);
            try {
                settings_.port = std::stoi(portStr);
            } catch (...) {}
        }
    } else {
        size_t colonPos = url.find(':');
        if (colonPos != std::string::npos) {
            settings_.host = Utf8ToWide(url.substr(0, colonPos));
            try {
                settings_.port = std::stoi(url.substr(colonPos + 1));
            } catch (...) {}
        }
    }

    return true;
}

bool ConfigStore::ExtractJsonStringField(const std::string& json, const std::string& key, std::string& out) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return false;

    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return false;

    pos = json.find('"', pos);
    if (pos == std::string::npos) return false;

    size_t end = json.find('"', pos + 1);
    if (end == std::string::npos) return false;

    out = json.substr(pos + 1, end - pos - 1);
    return true;
}

bool ConfigStore::ExtractJsonIntField(const std::string& json, const std::string& key, int& out) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return false;

    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return false;

    while (pos < json.length() && (json[pos] == ':' || json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) pos++;

    size_t end = pos;
    while (end < json.length() && (json[end] == '-' || isdigit(static_cast<unsigned char>(json[end])))) end++;

    try {
        out = std::stoi(json.substr(pos, end - pos));
        return true;
    } catch (...) {
        return false;
    }
}

bool ConfigStore::ExtractJsonBoolField(const std::string& json, const std::string& key, bool& out) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return false;

    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return false;

    while (pos < json.length() && (json[pos] == ':' || json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) pos++;

    if (pos + 4 <= json.length() && json.compare(pos, 4, "true") == 0) {
        out = true;
        return true;
    }
    if (pos + 5 <= json.length() && json.compare(pos, 5, "false") == 0) {
        out = false;
        return true;
    }
    return false;
}
