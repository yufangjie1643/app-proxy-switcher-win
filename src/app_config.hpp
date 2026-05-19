#pragma once
#include <string>
#include <vector>

enum class AppType {
    Msix = 0,       // Windows Store / MSIX 包应用
    Exe = 1,        // 普通可执行文件
    Npm = 2,        // npm 全局安装的 CLI
    VsCodeExt = 3   // VSCode 扩展
};

struct AppConfig {
    std::wstring id;           // 唯一标识
    std::wstring displayName;  // 显示名称
    AppType type = AppType::Msix;
    std::wstring packageName;  // MSIX 包名 / npm 包名 / VSCode 扩展 ID
    std::wstring exePath;      // 手动指定的 exe 路径 (Exe 类型用) 或检测到的路径
    bool useProxy = true;      // 默认是否使用代理启动

    bool IsValid() const {
        switch (type) {
            case AppType::Msix:
            case AppType::Npm:
            case AppType::VsCodeExt:
                return !packageName.empty();
            case AppType::Exe:
                return !exePath.empty() || !packageName.empty();
        }
        return false;
    }

    std::wstring GetTypeLabel() const {
        switch (type) {
            case AppType::Msix: return L"MSIX";
            case AppType::Exe: return L"EXE";
            case AppType::Npm: return L"NPM";
            case AppType::VsCodeExt: return L"VSCode";
        }
        return L"UNKNOWN";
    }
};

struct AppInstallInfo {
    std::wstring exePath;
    std::wstring appUserModelId;
    std::wstring displayName;
    bool IsFound() const { return !exePath.empty(); }
};
