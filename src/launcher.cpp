#include "launcher.hpp"
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <vector>
#include <string>

Launcher::Launcher() : sysProxy_(std::make_unique<SystemProxyManager>()) {}
Launcher::~Launcher() = default;

std::wstring Launcher::GetProcessNameFromExePath(const std::wstring& exePath) {
    size_t pos = exePath.find_last_of(L"\\/");
    std::wstring name = (pos != std::wstring::npos) ? exePath.substr(pos + 1) : exePath;
    return name;
}

bool Launcher::StopApp(const std::wstring& processName) {
    if (processName.empty()) return false;

    bool anyKilled = false;

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, processName.c_str()) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess) {
                    TerminateProcess(hProcess, 0);
                    WaitForSingleObject(hProcess, 3000);
                    CloseHandle(hProcess);
                    anyKilled = true;
                }
            }
        } while (Process32NextW(hSnap, &pe));
    }

    CloseHandle(hSnap);
    return anyKilled;
}

LaunchResult Launcher::Launch(const ProxySettings& settings, ProxyMode mode, const AppInstallInfo& info) {
    if (info.exePath.empty()) {
        return LaunchResult::Fail(L"应用程序路径为空");
    }

    std::wstring processName = GetProcessNameFromExePath(info.exePath);
    StopApp(processName);

    std::wstring proxyUrl = settings.scheme + L"://" + settings.host + L":" + std::to_wstring(settings.port);
    std::wstring noProxy = L"localhost,127.0.0.1,::1";
    std::wstring proxyHostPort = settings.host + L":" + std::to_wstring(settings.port);

    if (mode == ProxyMode::SystemProxy) {
        // System proxy mode: set Windows Internet Options registry
        if (!sysProxy_->Enable(settings.host, settings.port)) {
            return LaunchResult::Fail(L"设置系统代理失败。请检查是否有权限修改注册表。");
        }

        // Give system a moment to apply proxy settings
        Sleep(200);

        // Start app without environment variables (system proxy handles everything)
        bool ok = StartAppDirect(info, L"", L"");

        // Note: we intentionally do NOT disable system proxy immediately here,
        // because the app needs the proxy to remain active while running.
        // The proxy will be disabled when:
        // 1. User clicks "Disable System Proxy" button
        // 2. Program exits (SystemProxyManager destructor)

        if (!ok) {
            return LaunchResult::Fail(L"启动应用程序失败。请检查应用是否已正确安装。");
        }
        return LaunchResult::Ok();
    }

    // EnvVar mode: inject HTTP_PROXY/HTTPS_PROXY into process environment
    if (StartAppDirect(info, proxyUrl, noProxy)) {
        return LaunchResult::Ok();
    }

    if (!info.appUserModelId.empty()) {
        if (StartAppViaActivation(info)) {
            return LaunchResult::Ok();
        }
    }

    return LaunchResult::Fail(L"启动应用程序失败。请检查应用是否已正确安装。");
}

bool Launcher::StartAppDirect(const AppInstallInfo& info, const std::wstring& proxyUrl, const std::wstring& noProxy) {
    std::wstring workingDir = info.exePath;
    size_t lastSlash = workingDir.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos) {
        workingDir = workingDir.substr(0, lastSlash);
    }

    if (workingDir.find(L"WindowsApps") != std::wstring::npos) {
        wchar_t sysDir[MAX_PATH];
        GetSystemDirectoryW(sysDir, MAX_PATH);
        workingDir = sysDir;
    }

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWDEFAULT;

    PROCESS_INFORMATION pi = {};

    void* envBlock = nullptr;
    if (!proxyUrl.empty()) {
        envBlock = BuildEnvironmentBlock(proxyUrl, noProxy);
    }

    std::wstring appPath = info.exePath;
    std::wstring cmdLine;
    const wchar_t* exeToCreate = nullptr;

    // Check if it's a batch/cmd file - needs cmd.exe /c
    bool isCmdFile = (appPath.length() > 4 &&
        (_wcsicmp(appPath.substr(appPath.length() - 4).c_str(), L".cmd") == 0 ||
         _wcsicmp(appPath.substr(appPath.length() - 4).c_str(), L".bat") == 0));

    if (isCmdFile) {
        wchar_t sysDir[MAX_PATH];
        GetSystemDirectoryW(sysDir, MAX_PATH);
        std::wstring cmdExe = std::wstring(sysDir) + L"\\cmd.exe";
        exeToCreate = cmdExe.c_str();
        cmdLine = L"cmd.exe /c \"" + appPath + L"\"";
        // For cmd files, working dir should be where the cmd file is
        if (workingDir.empty() || workingDir == appPath) {
            workingDir = sysDir;
        }
    } else {
        exeToCreate = appPath.c_str();
        cmdLine = L"\"" + appPath + L"\"";
    }

    BOOL created = CreateProcessW(
        exeToCreate,
        cmdLine.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_PROCESS_GROUP,
        envBlock,
        workingDir.c_str(),
        &si,
        &pi
    );

    if (envBlock) {
        FreeEnvironmentBlock(envBlock);
    }

    if (created) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }

    return false;
}

bool Launcher::StartAppViaActivation(const AppInstallInfo& info) {
    HINSTANCE result = ShellExecuteW(nullptr, L"open", info.exePath.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
    return reinterpret_cast<INT_PTR>(result) > 32;
}

void* Launcher::BuildEnvironmentBlock(const std::wstring& proxyUrl, const std::wstring& noProxy) {
    void* currentEnv = GetEnvironmentStringsW();
    if (!currentEnv) return nullptr;

    std::vector<std::wstring> envVars;

    wchar_t* env = static_cast<wchar_t*>(currentEnv);
    while (*env) {
        envVars.push_back(env);
        env += wcslen(env) + 1;
    }

    FreeEnvironmentStringsW(static_cast<wchar_t*>(currentEnv));

    auto SetOrAddVar = [&](const std::wstring& name, const std::wstring& value) {
        std::wstring fullVar = name + L"=" + value;
        bool found = false;
        for (auto& var : envVars) {
            size_t eqPos = var.find(L'=');
            if (eqPos != std::wstring::npos) {
                std::wstring varName = var.substr(0, eqPos);
                if (_wcsicmp(varName.c_str(), name.c_str()) == 0) {
                    var = fullVar;
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            envVars.push_back(fullVar);
        }
    };

    SetOrAddVar(L"HTTP_PROXY", proxyUrl);
    SetOrAddVar(L"HTTPS_PROXY", proxyUrl);
    SetOrAddVar(L"ALL_PROXY", proxyUrl);
    SetOrAddVar(L"NO_PROXY", noProxy);
    SetOrAddVar(L"http_proxy", proxyUrl);
    SetOrAddVar(L"https_proxy", proxyUrl);
    SetOrAddVar(L"all_proxy", proxyUrl);
    SetOrAddVar(L"no_proxy", noProxy);

    size_t totalSize = 1;
    for (const auto& var : envVars) {
        totalSize += var.length() + 1;
    }

    wchar_t* block = new wchar_t[totalSize];
    wchar_t* ptr = block;

    for (const auto& var : envVars) {
        wcscpy_s(ptr, totalSize - static_cast<size_t>(ptr - block), var.c_str());
        ptr += var.length() + 1;
    }
    *ptr = L'\0';

    return block;
}

void Launcher::FreeEnvironmentBlock(void* block) {
    delete[] static_cast<wchar_t*>(block);
}
