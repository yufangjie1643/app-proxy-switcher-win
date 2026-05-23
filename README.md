# App Proxy Switcher for Windows

[中文](README.md) | [English](README.en.md)

Windows 原生代理启动器，用于为常见 AI 编程工具选择“直连启动”或“代理启动”。程序会自动识别已安装的桌面端、npm CLI 和 VS Code 生态插件，并在启动目标程序时注入代理环境变量，避免手动修改系统环境。

本项目源码来源于 [hloolx/codex-proxy-switcher-win](https://github.com/hloolx/codex-proxy-switcher-win)。当前仓库在其 MIT 许可代码基础上进行了重构和轻量化：从原有实现迁移为 C++17 Win32 原生程序，移除 .NET 运行时依赖，支持生成静态编译 x64 exe，并扩展了更多 Code Agent 软件识别逻辑。

![C++17](https://img.shields.io/badge/C++-17-00599C.svg)
![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-blue.svg)
![Runtime](https://img.shields.io/badge/runtime-static%20MSVC-brightgreen.svg)

## 功能

- 一键直连启动或代理启动。
- 支持环境变量代理模式和系统代理模式。
- 自动识别 OpenAI Codex Desktop、Claude Desktop、npm CLI、VS Code / Cursor / Windsurf / VSCodium 扩展。
- 代理地址可配置，默认 `http://127.0.0.1:7897`。
- 配置保存到 `%APPDATA%\CodexProxySwitcher\settings.ini`。
- 高 DPI Win32 界面，程序路径支持多行完整显示。

## 已内置识别的工具

桌面端：

- OpenAI Codex Desktop (MSIX)
- Claude Desktop (开始菜单快捷方式解析 EXE)

npm CLI：

- `@openai/codex`
- `@anthropic-ai/claude-code`
- `@google/gemini-cli`
- `@qwen-code/qwen-code`
- `opencode-ai`
- `@sourcegraph/amp`

VS Code 生态扩展：

- OpenAI Codex / ChatGPT (`openai.chatgpt`)
- GitHub Copilot Chat / Copilot
- Continue
- Cline
- Roo Code
- Kilo Code
- Gemini Code Assist
- Amazon Q
- Augment Code
- Windsurf / Codeium
- Sourcegraph Cody
- Tabnine

## 下载与运行

本仓库不直接提交编译好的 exe。普通用户推荐下载 GitHub Release 中的静态 x64 版本：

- [CodexProxySwitcher-win32-x64.exe](https://github.com/yufangjie1643/app-proxy-switcher-win/releases/download/v2.2.0/CodexProxySwitcher-win32-x64.exe)
- [CodexProxySwitcher-webview2-x64.exe](https://github.com/yufangjie1643/app-proxy-switcher-win/releases/download/v2.2.0/CodexProxySwitcher-webview2-x64.exe)

Win32 版体积最小，WebView2 版界面更现代。下载后直接运行对应文件即可。首次启动会读取或创建配置，并显示当前识别到的目标程序路径。

## 从源码构建

要求：

- Windows 10/11 x64
- Visual Studio 2022，安装“使用 C++ 的桌面开发”
- CMake 3.16+
- WebView2 版需要 Microsoft Edge WebView2 Runtime；构建脚本会自动下载 WebView2 SDK 到 `build/_deps/`

一键构建：

```bat
build.bat
```

`build.bat` 会自动准备 WebView2 SDK、配置 CMake，并同时编译 Win32 和 WebView2 两个 Release 版本。

手动构建：

```bat
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\Fetch-WebView2.ps1
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_WEBVIEW2=ON
cmake --build build --config Release
```

如果只需要 Win32 原生控件版，可配置 `-DBUILD_WEBVIEW2=OFF`。

输出：

```text
build/bin/CodexProxySwitcher.exe          Win32 原生控件版
build/bin/CodexProxySwitcherWebView2.exe  WebView2 界面版
```

编译好的程序位于 `build/bin/`。项目默认使用 MSVC 静态运行库 `/MT`，Release 产物不依赖额外 VC++ Runtime。

## 测试

```bat
cmake --build build --config Release --target app_finder_tests
ctest --test-dir build -C Release --output-on-failure
```

测试覆盖 Appx 包解析、npm 命令名映射、VS Code 扩展身份解析、内置工具清单和旧配置迁移。

## 项目结构

```text
src/                 Win32/C++17 源码
tests/               轻量回归测试
scripts/             PowerShell 辅助启动脚本
docs/                文档和分析材料
CMakeLists.txt       CMake 构建配置
build.bat            Windows 构建脚本
```

## 安全说明

环境变量代理模式只影响被启动的子进程，不会持久修改系统代理。系统代理模式会修改 Windows Internet Options 代理设置，程序提供“关闭系统代理”按钮恢复。

本项目不会主动联网、不会收集遥测，也不会上传用户配置。项目与 OpenAI、Anthropic、Microsoft、Google 等工具厂商无官方关联。

## 来源与许可

本项目基于 [hloolx/codex-proxy-switcher-win](https://github.com/hloolx/codex-proxy-switcher-win) 的 MIT License 源码重构。请在分发或二次开发时保留上游版权与许可说明。
