#pragma once
#include <string>
#include <optional>
#include "app_config.hpp"

struct AppxPackageInfo {
    std::wstring installLocation;
    std::wstring packageFamilyName;
};

std::optional<AppxPackageInfo> ParseAppxPackageJson(const std::string& output);
std::vector<std::wstring> GetNpmCommandCandidatesForPackage(const std::wstring& packageName);
std::wstring ParseVsCodeExtensionPackageJson(const std::string& json);

class AppFinder {
public:
    AppInstallInfo FindApp(const AppConfig& config);

private:
    AppInstallInfo FindMsixApp(const std::wstring& packageName);
    AppInstallInfo FindExeApp(const AppConfig& config);
    AppInstallInfo FindNpmApp(const std::wstring& packageName);
    AppInstallInfo FindVsCodeExtApp(const std::wstring& extensionId);
    AppInstallInfo TryFindViaPowerShell(const std::wstring& packageName);
    AppInstallInfo TryFindViaDirectoryWalk(const std::wstring& packagePrefix);
    AppInstallInfo TryFindExeViaStartMenuShortcut(const AppConfig& config);
    std::wstring TryParseAppIdFromManifest(const std::wstring& installLocation);
};
