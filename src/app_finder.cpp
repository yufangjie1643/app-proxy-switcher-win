#include "app_finder.hpp"
#include "utils.hpp"
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <objbase.h>
#include <shlobj.h>
#include <string>
#include <utility>
#include <vector>
#include <optional>

namespace {

bool IsHex(char ch) {
    return std::isxdigit(static_cast<unsigned char>(ch)) != 0;
}

int HexValue(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return 0;
}

void AppendUtf8(std::string& out, unsigned int codePoint) {
    if (codePoint <= 0x7F) {
        out.push_back(static_cast<char>(codePoint));
    } else if (codePoint <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
}

bool ParseJsonStringAt(const std::string& json, size_t quotePos, std::string& out, size_t* nextPos = nullptr) {
    if (quotePos >= json.size() || json[quotePos] != '"') return false;

    out.clear();
    for (size_t i = quotePos + 1; i < json.size(); ++i) {
        char ch = json[i];
        if (ch == '"') {
            if (nextPos) *nextPos = i + 1;
            return true;
        }

        if (ch != '\\') {
            out.push_back(ch);
            continue;
        }

        if (++i >= json.size()) return false;
        char escaped = json[i];
        switch (escaped) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                if (i + 4 >= json.size() ||
                    !IsHex(json[i + 1]) || !IsHex(json[i + 2]) ||
                    !IsHex(json[i + 3]) || !IsHex(json[i + 4])) {
                    return false;
                }
                unsigned int codePoint =
                    (HexValue(json[i + 1]) << 12) |
                    (HexValue(json[i + 2]) << 8) |
                    (HexValue(json[i + 3]) << 4) |
                    HexValue(json[i + 4]);
                AppendUtf8(out, codePoint);
                i += 4;
                break;
            }
            default:
                return false;
        }
    }
    return false;
}

bool FindJsonStringField(const std::string& json, const std::string& key, size_t start, size_t end, std::string& out) {
    const std::string quotedKey = "\"" + key + "\"";
    size_t pos = start;

    while ((pos = json.find(quotedKey, pos)) != std::string::npos && pos < end) {
        size_t colon = json.find(':', pos + quotedKey.size());
        if (colon == std::string::npos || colon >= end) return false;

        size_t valuePos = colon + 1;
        while (valuePos < end && std::isspace(static_cast<unsigned char>(json[valuePos]))) {
            ++valuePos;
        }

        if (valuePos < end && json[valuePos] == '"') {
            return ParseJsonStringAt(json, valuePos, out);
        }

        pos = colon + 1;
    }

    return false;
}

bool FindContainingObject(const std::string& json, size_t keyPos, size_t& objectStart, size_t& objectEnd) {
    bool inString = false;
    bool escaped = false;
    int depth = 0;
    objectStart = std::string::npos;

    for (size_t i = 0; i < json.size(); ++i) {
        char ch = json[i];
        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '"') {
                inString = false;
            }
            continue;
        }

        if (ch == '"') {
            inString = true;
        } else if (ch == '{') {
            if (depth == 0) objectStart = i;
            ++depth;
        } else if (ch == '}') {
            if (depth > 0) --depth;
            if (depth == 0 && objectStart != std::string::npos) {
                objectEnd = i + 1;
                if (keyPos >= objectStart && keyPos < objectEnd) {
                    return true;
                }
                objectStart = std::string::npos;
            }
        }
    }

    return false;
}

std::wstring EscapePowerShellSingleQuotedString(const std::wstring& value) {
    std::wstring escaped;
    escaped.reserve(value.size());
    for (wchar_t ch : value) {
        escaped.push_back(ch);
        if (ch == L'\'') {
            escaped.push_back(L'\'');
        }
    }
    return escaped;
}

std::wstring BuildAppxPackageFamilyNameFromDirectory(const std::wstring& dirName) {
    size_t firstUnderscore = dirName.find(L'_');
    size_t publisherMarker = dirName.rfind(L"__");
    if (firstUnderscore == std::wstring::npos || publisherMarker == std::wstring::npos ||
        publisherMarker + 2 >= dirName.size()) {
        return L"";
    }

    return dirName.substr(0, firstUnderscore) + L"_" + dirName.substr(publisherMarker + 2);
}

void AddUnique(std::vector<std::wstring>& values, const std::wstring& value) {
    if (value.empty()) return;
    for (const auto& existing : values) {
        if (_wcsicmp(existing.c_str(), value.c_str()) == 0) {
            return;
        }
    }
    values.push_back(value);
}

std::wstring GetEnvVar(const wchar_t* name) {
    wchar_t buffer[MAX_PATH];
    DWORD len = GetEnvironmentVariableW(name, buffer, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return L"";
    }
    return buffer;
}

std::wstring FindFirstOnPath(const std::wstring& command) {
    std::wstring whereCmd = L"where.exe " + command;
    wchar_t cmdLine[512];
    wcsncpy_s(cmdLine, whereCmd.c_str(), _TRUNCATE);

    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        return L"";
    }

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(nullptr, cmdLine, nullptr, nullptr, TRUE,
                       CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hWrite);
        CloseHandle(hRead);
        return L"";
    }

    CloseHandle(hWrite);

    char buffer[1024];
    DWORD bytesRead = 0;
    std::string output;
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output.append(buffer);
    }
    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (output.empty()) return L"";

    size_t newline = output.find('\n');
    if (newline != std::string::npos) {
        output = output.substr(0, newline);
    }
    if (!output.empty() && output.back() == '\r') {
        output.pop_back();
    }
    return output.empty() ? L"" : Utf8ToWide(output);
}

bool DirectoryHasExtension(const std::wstring& extensionsDir, const std::wstring& extensionId) {
    if (!DirectoryExists(extensionsDir)) {
        return false;
    }

    std::wstring searchPattern = extensionsDir + L"\\*";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return false;
    }

    bool found = false;
    do {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            continue;
        }

        std::wstring dirName = findData.cFileName;
        if (dirName == L"." || dirName == L"..") {
            continue;
        }

        std::wstring packagePath = extensionsDir + L"\\" + dirName + L"\\package.json";
        HANDLE hFile = CreateFileW(packagePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD size = GetFileSize(hFile, nullptr);
            if (size != INVALID_FILE_SIZE && size > 0 && size <= 1024 * 1024) {
                std::string json(static_cast<size_t>(size), 0);
                DWORD read = 0;
                if (ReadFile(hFile, json.data(), size, &read, nullptr)) {
                    json.resize(read);
                    std::wstring identity = ParseVsCodeExtensionPackageJson(json);
                    if (!identity.empty() && _wcsicmp(identity.c_str(), extensionId.c_str()) == 0) {
                        found = true;
                    }
                }
            }
            CloseHandle(hFile);
            if (found) {
                break;
            }
        } else {
            if (_wcsicmp(dirName.c_str(), extensionId.c_str()) == 0) {
                found = true;
                break;
            }
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return found;
}

std::wstring GetFileNameWithoutExtension(const std::wstring& path) {
    size_t slash = path.find_last_of(L"\\/");
    std::wstring name = slash == std::wstring::npos ? path : path.substr(slash + 1);
    size_t dot = name.find_last_of(L'.');
    if (dot != std::wstring::npos) {
        name = name.substr(0, dot);
    }
    return name;
}

bool ShortcutNameMatches(const std::wstring& shortcutPath, const AppConfig& config) {
    std::wstring name = GetFileNameWithoutExtension(shortcutPath);
    if (!config.packageName.empty() && _wcsicmp(name.c_str(), config.packageName.c_str()) == 0) {
        return true;
    }
    if (!config.displayName.empty() && _wcsicmp(name.c_str(), config.displayName.c_str()) == 0) {
        return true;
    }
    return false;
}

std::optional<std::wstring> ResolveShortcutTarget(const std::wstring& shortcutPath) {
    HRESULT initHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool shouldUninitialize = SUCCEEDED(initHr);
    if (FAILED(initHr) && initHr != RPC_E_CHANGED_MODE) {
        return std::nullopt;
    }

    IShellLinkW* shellLink = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
        IID_IShellLinkW, reinterpret_cast<void**>(&shellLink));
    if (FAILED(hr) || !shellLink) {
        if (shouldUninitialize) CoUninitialize();
        return std::nullopt;
    }

    IPersistFile* persistFile = nullptr;
    hr = shellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&persistFile));
    if (FAILED(hr) || !persistFile) {
        shellLink->Release();
        if (shouldUninitialize) CoUninitialize();
        return std::nullopt;
    }

    std::optional<std::wstring> result;
    hr = persistFile->Load(shortcutPath.c_str(), STGM_READ);
    if (SUCCEEDED(hr)) {
        wchar_t target[MAX_PATH] = {};
        WIN32_FIND_DATAW findData = {};
        hr = shellLink->GetPath(target, MAX_PATH, &findData, SLGP_RAWPATH);
        if (SUCCEEDED(hr) && target[0] != L'\0' && FileExists(target)) {
            result = target;
        }
    }

    persistFile->Release();
    shellLink->Release();
    if (shouldUninitialize) CoUninitialize();
    return result;
}

std::vector<std::wstring> GetStartMenuRoots() {
    std::vector<std::wstring> roots;

    wchar_t path[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_STARTMENU, nullptr, 0, path))) {
        AddUnique(roots, std::wstring(path) + L"\\Programs");
    }
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_COMMON_STARTMENU, nullptr, 0, path))) {
        AddUnique(roots, std::wstring(path) + L"\\Programs");
    }

    return roots;
}

std::optional<std::wstring> FindShortcutTargetInDirectory(const std::wstring& directory, const AppConfig& config) {
    std::wstring searchPath = directory + L"\\*";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }

    std::optional<std::wstring> result;
    do {
        std::wstring name = findData.cFileName;
        if (name == L"." || name == L"..") {
            continue;
        }

        std::wstring fullPath = directory + L"\\" + name;
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            result = FindShortcutTargetInDirectory(fullPath, config);
            if (result) break;
        } else if (name.size() > 4 && _wcsicmp(name.substr(name.size() - 4).c_str(), L".lnk") == 0 &&
                   ShortcutNameMatches(fullPath, config)) {
            result = ResolveShortcutTarget(fullPath);
            if (result) break;
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return result;
}

struct VsCodeHostCandidate {
    std::wstring label;
    std::wstring extensionsDir;
    std::vector<std::wstring> exePaths;
    std::wstring pathCommand;
};

std::vector<VsCodeHostCandidate> BuildVsCodeHostCandidates() {
    std::wstring userProfile = GetEnvVar(L"USERPROFILE");
    std::wstring localAppData = GetEnvVar(L"LOCALAPPDATA");
    std::wstring programFiles = GetEnvVar(L"ProgramFiles");
    std::wstring programFilesX86 = GetEnvVar(L"ProgramFiles(x86)");

    std::vector<VsCodeHostCandidate> hosts;
    if (userProfile.empty()) {
        return hosts;
    }

    auto AddHost = [&](const std::wstring& label, const std::wstring& extensionsDir,
                       std::vector<std::wstring> exePaths, const std::wstring& pathCommand) {
        hosts.push_back({ label, extensionsDir, std::move(exePaths), pathCommand });
    };

    AddHost(L"VSCode", userProfile + L"\\.vscode\\extensions", {
        localAppData + L"\\Programs\\Microsoft VS Code\\Code.exe",
        programFiles + L"\\Microsoft VS Code\\Code.exe",
        programFilesX86 + L"\\Microsoft VS Code\\Code.exe"
    }, L"code");

    AddHost(L"VSCode Insiders", userProfile + L"\\.vscode-insiders\\extensions", {
        localAppData + L"\\Programs\\Microsoft VS Code Insiders\\Code - Insiders.exe",
        programFiles + L"\\Microsoft VS Code Insiders\\Code - Insiders.exe",
        programFilesX86 + L"\\Microsoft VS Code Insiders\\Code - Insiders.exe"
    }, L"code-insiders");

    AddHost(L"Cursor", userProfile + L"\\.cursor\\extensions", {
        localAppData + L"\\Programs\\Cursor\\Cursor.exe"
    }, L"cursor");

    AddHost(L"Windsurf", userProfile + L"\\.windsurf\\extensions", {
        localAppData + L"\\Programs\\Windsurf\\Windsurf.exe"
    }, L"windsurf");

    AddHost(L"VSCodium", userProfile + L"\\.vscode-oss\\extensions", {
        localAppData + L"\\Programs\\VSCodium\\VSCodium.exe",
        programFiles + L"\\VSCodium\\VSCodium.exe",
        programFilesX86 + L"\\VSCodium\\VSCodium.exe"
    }, L"codium");

    return hosts;
}

}

std::optional<AppxPackageInfo> ParseAppxPackageJson(const std::string& output) {
    const std::string installKey = "\"InstallLocation\"";
    size_t pos = 0;

    while ((pos = output.find(installKey, pos)) != std::string::npos) {
        size_t objectStart = 0;
        size_t objectEnd = output.size();
        if (!FindContainingObject(output, pos, objectStart, objectEnd)) {
            objectStart = 0;
            objectEnd = output.size();
        }

        std::string installLocation;
        if (!FindJsonStringField(output, "InstallLocation", objectStart, objectEnd, installLocation) ||
            installLocation.empty()) {
            pos += installKey.size();
            continue;
        }

        std::string packageFamilyName;
        FindJsonStringField(output, "PackageFamilyName", objectStart, objectEnd, packageFamilyName);

        return AppxPackageInfo{
            Utf8ToWide(installLocation),
            Utf8ToWide(packageFamilyName)
        };
    }

    return std::nullopt;
}

std::vector<std::wstring> GetNpmCommandCandidatesForPackage(const std::wstring& packageName) {
    std::vector<std::wstring> commands;

    if (_wcsicmp(packageName.c_str(), L"@openai/codex") == 0 || _wcsicmp(packageName.c_str(), L"codex") == 0) {
        AddUnique(commands, L"codex");
    } else if (_wcsicmp(packageName.c_str(), L"@anthropic-ai/claude-code") == 0 ||
               _wcsicmp(packageName.c_str(), L"claude") == 0) {
        AddUnique(commands, L"claude");
    } else if (_wcsicmp(packageName.c_str(), L"@google/gemini-cli") == 0) {
        AddUnique(commands, L"gemini");
    } else if (_wcsicmp(packageName.c_str(), L"@qwen-code/qwen-code") == 0) {
        AddUnique(commands, L"qwen");
    } else if (_wcsicmp(packageName.c_str(), L"opencode-ai") == 0) {
        AddUnique(commands, L"opencode");
    } else if (_wcsicmp(packageName.c_str(), L"@sourcegraph/amp") == 0) {
        AddUnique(commands, L"amp");
    }

    AddUnique(commands, packageName);

    size_t slashPos = packageName.find(L'/');
    if (slashPos != std::wstring::npos && slashPos + 1 < packageName.size()) {
        AddUnique(commands, packageName.substr(slashPos + 1));
    }

    return commands;
}

std::wstring ParseVsCodeExtensionPackageJson(const std::string& json) {
    std::string publisher;
    std::string name;
    if (!FindJsonStringField(json, "publisher", 0, json.size(), publisher) ||
        !FindJsonStringField(json, "name", 0, json.size(), name) ||
        publisher.empty() || name.empty()) {
        return L"";
    }

    return Utf8ToWide(publisher + "." + name);
}

AppInstallInfo AppFinder::FindApp(const AppConfig& config) {
    switch (config.type) {
        case AppType::Msix:
            return FindMsixApp(config.packageName);
        case AppType::Exe:
            return FindExeApp(config);
        case AppType::Npm:
            return FindNpmApp(config.packageName);
        case AppType::VsCodeExt:
            return FindVsCodeExtApp(config.packageName);
    }
    return AppInstallInfo{};
}

AppInstallInfo AppFinder::FindMsixApp(const std::wstring& packageName) {
    auto info = TryFindViaPowerShell(packageName);
    if (info.IsFound()) return info;

    info = TryFindViaDirectoryWalk(packageName);
    if (info.IsFound()) return info;

    return AppInstallInfo{};
}

AppInstallInfo AppFinder::FindExeApp(const AppConfig& config) {
    AppInstallInfo info;
    if (!config.exePath.empty() && FileExists(config.exePath)) {
        info.exePath = config.exePath;
        info.displayName = config.displayName.empty()
            ? config.exePath.substr(config.exePath.find_last_of(L"\\") + 1)
            : config.displayName;
        return info;
    }

    return TryFindExeViaStartMenuShortcut(config);
}

AppInstallInfo AppFinder::TryFindExeViaStartMenuShortcut(const AppConfig& config) {
    AppInstallInfo info;
    if (config.packageName.empty() && config.displayName.empty()) {
        return info;
    }

    for (const auto& root : GetStartMenuRoots()) {
        auto target = FindShortcutTargetInDirectory(root, config);
        if (target && FileExists(*target)) {
            info.exePath = *target;
            info.displayName = config.displayName.empty() ? GetFileNameWithoutExtension(*target) : config.displayName;
            return info;
        }
    }

    return info;
}

AppInstallInfo AppFinder::FindNpmApp(const std::wstring& packageName) {
    AppInstallInfo info;

    // Common npm global prefix paths
    std::vector<std::wstring> npmPaths;

    wchar_t appDataPath[MAX_PATH];
    if (GetEnvironmentVariableW(L"APPDATA", appDataPath, MAX_PATH) > 0) {
        npmPaths.push_back(std::wstring(appDataPath) + L"\\npm");
    }

    wchar_t localAppDataPath[MAX_PATH];
    if (GetEnvironmentVariableW(L"LOCALAPPDATA", localAppDataPath, MAX_PATH) > 0) {
        npmPaths.push_back(std::wstring(localAppDataPath) + L"\\npm");
    }

    // Also check npm_config_prefix
    wchar_t npmPrefix[MAX_PATH];
    if (GetEnvironmentVariableW(L"npm_config_prefix", npmPrefix, MAX_PATH) > 0) {
        npmPaths.push_back(std::wstring(npmPrefix));
    }

    auto commandCandidates = GetNpmCommandCandidatesForPackage(packageName);

    for (const auto& command : commandCandidates) {
        for (const auto& path : npmPaths) {
            std::wstring cmdPath = path + L"\\" + command + L".cmd";
            if (FileExists(cmdPath)) {
                info.exePath = cmdPath;
                info.displayName = packageName + L" (npm)";
                return info;
            }

            std::wstring psPath = path + L"\\" + command + L".ps1";
            if (FileExists(psPath)) {
                info.exePath = psPath;
                info.displayName = packageName + L" (npm)";
                return info;
            }

            std::wstring cmdPath2 = path + L"\\" + command;
            if (FileExists(cmdPath2)) {
                info.exePath = cmdPath2;
                info.displayName = packageName + L" (npm)";
                return info;
            }
        }
    }

    for (const auto& command : commandCandidates) {
        std::wstring whereCmd = L"where.exe " + command;
        wchar_t cmdLine[512];
        wcsncpy_s(cmdLine, whereCmd.c_str(), _TRUNCATE);

        SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
        HANDLE hRead = nullptr, hWrite = nullptr;
        if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
            continue;
        }

        STARTUPINFOW si = { sizeof(si) };
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hWrite;
        si.hStdError = hWrite;
        si.wShowWindow = SW_HIDE;

        PROCESS_INFORMATION pi = {};
        if (CreateProcessW(nullptr, cmdLine, nullptr, nullptr, TRUE,
                           CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
            CloseHandle(hWrite);

            char buffer[1024];
            DWORD bytesRead = 0;
            std::string output;
            while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                output.append(buffer);
            }
            CloseHandle(hRead);
            WaitForSingleObject(pi.hProcess, 5000);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            if (!output.empty()) {
                size_t newline = output.find('\n');
                if (newline != std::string::npos) {
                    output = output.substr(0, newline);
                }
                if (!output.empty() && output.back() == '\r') {
                    output.pop_back();
                }
                if (!output.empty()) {
                    info.exePath = Utf8ToWide(output);
                    info.displayName = packageName + L" (npm)";
                    return info;
                }
            }
        } else {
            CloseHandle(hWrite);
            CloseHandle(hRead);
        }
    }

    return info;
}

AppInstallInfo AppFinder::FindVsCodeExtApp(const std::wstring& extensionId) {
    AppInstallInfo info;

    for (const auto& host : BuildVsCodeHostCandidates()) {
        if (!DirectoryHasExtension(host.extensionsDir, extensionId)) {
            continue;
        }

        for (const auto& path : host.exePaths) {
            if (!path.empty() && FileExists(path)) {
                info.exePath = path;
                info.displayName = extensionId + L" (" + host.label + L")";
                return info;
            }
        }

        std::wstring pathResult = FindFirstOnPath(host.pathCommand);
        if (!pathResult.empty()) {
            info.exePath = pathResult;
            info.displayName = extensionId + L" (" + host.label + L")";
            return info;
        }
    }

    return info;
}

AppInstallInfo AppFinder::TryFindViaPowerShell(const std::wstring& packageName) {
    std::wstring escapedPackageName = EscapePowerShellSingleQuotedString(packageName);
    std::wstring cmd =
        L"powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command "
        L"\"$name='" + escapedPackageName + L"'; "
        L"$pkg=Get-AppxPackage -Name $name -ErrorAction SilentlyContinue | Select-Object -First 1; "
        L"if (-not $pkg) { "
        L"$like='*' + $name + '*'; "
        L"$pkg=Get-AppxPackage -ErrorAction SilentlyContinue | "
        L"Where-Object { $_.Name -eq $name -or $_.Name -like $like -or $_.PackageFamilyName -like $like } | "
        L"Select-Object -First 1 "
        L"}; "
        L"if ($pkg) { $pkg | Select-Object InstallLocation, PackageFamilyName | ConvertTo-Json -Compress }\"";

    std::vector<wchar_t> cmdLine(cmd.begin(), cmd.end());
    cmdLine.push_back(L'\0');

    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };

    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        return AppInstallInfo{};
    }

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return AppInstallInfo{};
    }

    CloseHandle(hWrite);

    std::string output;
    char buffer[4096];
    DWORD bytesRead = 0;
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output.append(buffer);
    }

    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    auto packageInfo = ParseAppxPackageJson(output);
    if (!packageInfo || packageInfo->installLocation.empty()) return AppInstallInfo{};

    std::wstring wInstallLoc = packageInfo->installLocation;
    std::wstring exePath = wInstallLoc + L"\\app\\Codex.exe";

    if (!FileExists(exePath)) {
        exePath = wInstallLoc + L"\\app.exe";
        if (!FileExists(exePath)) {
            std::wstring searchPath = wInstallLoc + L"\\*.exe";
            WIN32_FIND_DATAW findData;
            HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
            if (hFind != INVALID_HANDLE_VALUE) {
                exePath = wInstallLoc + L"\\" + findData.cFileName;
                FindClose(hFind);
            } else {
                return AppInstallInfo{};
            }
        }
    }

    AppInstallInfo info;
    info.exePath = exePath;
    info.displayName = packageName;

    if (!packageInfo->packageFamilyName.empty()) {
        std::wstring appId = TryParseAppIdFromManifest(wInstallLoc);
        if (!appId.empty()) {
            info.appUserModelId = packageInfo->packageFamilyName + L"!" + appId;
        }
    }

    return info;
}

AppInstallInfo AppFinder::TryFindViaDirectoryWalk(const std::wstring& packagePrefix) {
    const wchar_t* windowsAppsRoot = L"C:\\Program Files\\WindowsApps";

    if (!DirectoryExists(windowsAppsRoot)) {
        return AppInstallInfo{};
    }

    WIN32_FIND_DATAW findData;
    std::wstring searchPath = std::wstring(windowsAppsRoot) + L"\\" + packagePrefix + L"_*";

    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return AppInstallInfo{};
    }

    AppInstallInfo result;

    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::wstring dirName = findData.cFileName;
            std::wstring fullPath = std::wstring(windowsAppsRoot) + L"\\" + dirName;
            std::wstring exePath = fullPath + L"\\app\\Codex.exe";

            if (!FileExists(exePath)) {
                exePath = fullPath + L"\\app.exe";
                if (!FileExists(exePath)) {
                    std::wstring searchExe = fullPath + L"\\*.exe";
                    WIN32_FIND_DATAW fd;
                    HANDLE h = FindFirstFileW(searchExe.c_str(), &fd);
                    if (h != INVALID_HANDLE_VALUE) {
                        exePath = fullPath + L"\\" + fd.cFileName;
                        FindClose(h);
                    } else {
                        continue;
                    }
                }
            }

            result.exePath = exePath;
            result.displayName = packagePrefix;

            std::wstring appId = TryParseAppIdFromManifest(fullPath);
            if (!appId.empty()) {
                std::wstring packageFamilyName = BuildAppxPackageFamilyNameFromDirectory(dirName);
                if (!packageFamilyName.empty()) {
                    result.appUserModelId = packageFamilyName + L"!" + appId;
                }
            }

            break;
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return result;
}

std::wstring AppFinder::TryParseAppIdFromManifest(const std::wstring& installLocation) {
    std::wstring manifestPath = installLocation + L"\\AppxManifest.xml";

    HANDLE hFile = CreateFileW(manifestPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return L"";
    }

    DWORD size = GetFileSize(hFile, nullptr);
    if (size == INVALID_FILE_SIZE || size == 0 || size > 1024 * 1024) {
        CloseHandle(hFile);
        return L"";
    }

    std::string xml(static_cast<size_t>(size), 0);
    DWORD read = 0;
    ReadFile(hFile, xml.data(), size, &read, nullptr);
    CloseHandle(hFile);

    std::string search = "Application";
    size_t pos = xml.find(search);
    if (pos == std::string::npos) return L"";

    search = "Id=\"";
    pos = xml.find(search, pos);
    if (pos == std::string::npos) return L"";

    size_t end = xml.find('"', pos + search.length());
    if (end == std::string::npos) return L"";

    return Utf8ToWide(xml.substr(pos + search.length(), end - pos - search.length()));
}
