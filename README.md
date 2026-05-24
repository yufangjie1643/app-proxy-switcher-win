# App Proxy Switcher for Windows

[中文](README.md) | [English](README.en.md)

一个给 AI 编程工具使用的 Windows 代理启动器。你可以用它一键启动 Codex、Claude、Gemini、Copilot、Cline、Roo Code 等工具，并选择“直连启动”或“代理启动”。

本项目源码来源于 [hloolx/codex-proxy-switcher-win](https://github.com/hloolx/codex-proxy-switcher-win)。当前版本基于其 MIT 许可源码进行了重构和轻量化：迁移为 C++17 Win32 原生程序，移除 .NET 运行时依赖，支持静态 x64 构建，并扩展了桌面端、npm CLI、VS Code 生态 Code Agent 的识别逻辑。

![C++17](https://img.shields.io/badge/C++-17-00599C.svg)
![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-blue.svg)
![Runtime](https://img.shields.io/badge/runtime-static%20MSVC-brightgreen.svg)

## 我应该下载哪个版本？

普通用户推荐下载 **WebView2 版**。它界面更清楚，会显示首次使用引导、代理检测结果、程序路径和最近一次操作状态。

| 版本 | 适合谁 | 说明 |
| --- | --- | --- |
| `AppProxySwitcher-WebView2-x64.zip` | 大多数用户 | 推荐版本，界面更友好；需要 Microsoft Edge WebView2 Runtime |
| `AppProxySwitcher-Win32-x64.zip` | 老机器、追求最小体积 | 轻量 fallback，只保留核心启动功能 |
| 单个 `.exe` | 熟悉 Windows 的用户 | 直接运行即可，但压缩包内 README 更完整 |

> 如果不确定，下 WebView2 版即可。Windows 10/11 大多已经带有 WebView2 Runtime；如果提示缺少 Runtime，请安装 Microsoft Edge WebView2 Runtime。

## 下载与运行

仓库不提交编译好的 exe，请到 GitHub Release 下载静态 x64 版本：

- [AppProxySwitcher-WebView2-x64.zip](https://github.com/yufangjie1643/app-proxy-switcher-win/releases/download/v2.3.2/AppProxySwitcher-WebView2-x64.zip)
- [AppProxySwitcher-Win32-x64.zip](https://github.com/yufangjie1643/app-proxy-switcher-win/releases/download/v2.3.2/AppProxySwitcher-Win32-x64.zip)
- [CodexProxySwitcher-webview2-x64.exe](https://github.com/yufangjie1643/app-proxy-switcher-win/releases/download/v2.3.2/CodexProxySwitcher-webview2-x64.exe)
- [CodexProxySwitcher-win32-x64.exe](https://github.com/yufangjie1643/app-proxy-switcher-win/releases/download/v2.3.2/CodexProxySwitcher-win32-x64.exe)

下载后解压，双击运行 `CodexProxySwitcherWebView2.exe` 或 `CodexProxySwitcher.exe`。首次启动会读取或创建配置，并显示识别到的目标程序路径。

## 界面预览

WebView2 推荐版：

![WebView2 界面版界面](docs/images/webview2-ui.png)

Win32 轻量版：

![Win32 原生控件版界面](docs/images/win32-ui.png)

## 第一次使用

1. 打开你的代理软件，例如 Clash、Mihomo、V2RayN 或其他本地代理工具。
2. 在本程序里点击 **检测代理**。程序只扫描本机常见端口，例如 `7890`、`7897`、`1080`、`10808`、`10809`、`8080`、`8118`、`8888`，不会主动做远程联网测试。
3. 确认“程序路径”已经显示完整路径，然后点击 **代理启动**。

如果检测到其他可用端口，程序会询问是否切换到推荐地址，例如 `http://127.0.0.1:7890`。

## 找不到程序怎么办？

“未找到目标程序”不是死路，可以按下面顺序处理：

1. 点击 **重新扫描**。
2. 点击 **切换应用**，选择已检测到的工具，或手动选择 exe。
3. 点击 **配置目录**，检查当前配置文件。
4. 确认目标工具已经安装，并且 npm / VS Code / Cursor / Windsurf 在当前用户下可用。

程序路径支持多行完整显示，方便确认到底启动的是哪个 exe 或脚本。

## 两种代理模式

| 模式 | 推荐程度 | 影响范围 |
| --- | --- | --- |
| 环境变量模式 | 推荐 | 只影响本工具启动的目标程序，会注入 `HTTP_PROXY`、`HTTPS_PROXY`、`ALL_PROXY`、`NO_PROXY` |
| 系统代理模式 | 谨慎使用 | 会修改 Windows Internet Options，影响浏览器和其他程序；用完请点“关闭系统代理” |

大多数情况下使用环境变量模式即可。只有目标工具完全不读取代理环境变量时，再尝试系统代理模式。

## 已内置识别的工具

- 桌面端：OpenAI Codex Desktop、Claude Desktop。
- npm CLI：`@openai/codex`、`@anthropic-ai/claude-code`、`@google/gemini-cli`、`@qwen-code/qwen-code`、`opencode-ai`、`@sourcegraph/amp`。
- VS Code 生态：OpenAI Codex / ChatGPT、GitHub Copilot Chat、Continue、Cline、Roo Code、Kilo Code、Gemini Code Assist、Amazon Q、Augment Code、Windsurf / Codeium、Sourcegraph Cody、Tabnine。

## 从源码构建

需要准备：

- Windows 10/11 x64
- Visual Studio 2022，并安装“使用 C++ 的桌面开发”
- CMake 3.16+
- WebView2 版需要 Microsoft Edge WebView2 Runtime；构建脚本会自动下载 WebView2 SDK 到 `build/_deps/`

一键编译：

```bat
build.bat
```

手动编译：

```bat
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\Fetch-WebView2.ps1
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_WEBVIEW2=ON
cmake --build build --config Release
```

编译完成后，程序在：

```text
build/bin/CodexProxySwitcher.exe          Win32 轻量版
build/bin/CodexProxySwitcherWebView2.exe  WebView2 推荐版
```

如果只需要 Win32 版，可使用 `-DBUILD_WEBVIEW2=OFF`。项目默认使用 MSVC 静态运行库 `/MT`，Release exe 不需要额外安装 VC++ Runtime。

## 测试

```bat
cmake --build build --config Release --target app_finder_tests
ctest --test-dir build -C Release --output-on-failure
```

测试覆盖 Appx 包解析、npm 命令映射、VS Code 扩展解析、内置工具清单、旧配置迁移、DPI 窗口尺寸和本地代理端口检测。

## 安全与常见提示

- 本程序不会收集遥测，也不会上传用户配置。
- 代理检测只检查本机端口是否可连接，不会访问远程网站。
- 当前发布包未做代码签名，Windows SmartScreen 可能提示风险；这是便携开源程序常见情况。
- 配置文件位于 `%APPDATA%\CodexProxySwitcher\settings.ini`。
- 本项目与 OpenAI、Anthropic、Microsoft、Google 等工具厂商无官方关联。

## 项目结构

```text
src/                 C++17 / Win32 源码
tests/               轻量回归测试
scripts/             PowerShell 辅助脚本
docs/images/         README 截图
CMakeLists.txt       CMake 构建配置
build.bat            一键构建脚本
```

## 来源与许可

本项目基于 [hloolx/codex-proxy-switcher-win](https://github.com/hloolx/codex-proxy-switcher-win) 的 MIT License 源码重构。分发或二次开发时请保留上游版权与许可说明。
