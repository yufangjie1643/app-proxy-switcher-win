# App Proxy Switcher for Windows

[中文](README.md) | [English](README.en.md)

A native Windows proxy launcher for common AI coding tools. It lets you start supported tools in either direct mode or proxy mode, injecting proxy environment variables only for the launched process instead of editing the global system environment.

This repository is derived from [hloolx/codex-proxy-switcher-win](https://github.com/hloolx/codex-proxy-switcher-win). Based on that MIT-licensed source, this version has been refactored and slimmed down into a C++17 Win32 native application, removes the .NET runtime dependency, keeps a statically linked x64 executable, and expands discovery for Code Agent tools.

![C++17](https://img.shields.io/badge/C++-17-00599C.svg)
![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-blue.svg)
![Runtime](https://img.shields.io/badge/runtime-static%20MSVC-brightgreen.svg)

## Features

- Launch tools in direct mode or proxy mode.
- Supports environment-variable proxy mode and system proxy mode.
- Detects OpenAI Codex Desktop, Claude Desktop, npm CLIs, and VS Code / Cursor / Windsurf / VSCodium extensions.
- Configurable proxy URL, defaulting to `http://127.0.0.1:7897`.
- Stores settings in `%APPDATA%\CodexProxySwitcher\settings.ini`.
- High-DPI Win32 UI with full multi-line path display.

## Built-in Tool Detection

Desktop apps:

- OpenAI Codex Desktop (MSIX)
- Claude Desktop (resolved from Start Menu shortcut to EXE)

npm CLIs:

- `@openai/codex`
- `@anthropic-ai/claude-code`
- `@google/gemini-cli`
- `@qwen-code/qwen-code`
- `opencode-ai`
- `@sourcegraph/amp`

VS Code ecosystem extensions:

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

## Download and Run

Download the static x64 build from GitHub Releases:

- [CodexProxySwitcher-x64.exe](https://github.com/yufangjie1643/app-proxy-switcher-win/releases/download/v2.1.0/CodexProxySwitcher-x64.exe)

The repository also keeps the same build artifact at:

```text
release/CodexProxySwitcher-x64.exe
```

Run the executable directly. On first launch, it reads or creates the configuration file and shows the detected target program paths.

## Build from Source

Requirements:

- Windows 10/11 x64
- Visual Studio 2022 with Desktop development with C++
- CMake 3.16+

One-command build:

```bat
build.bat
```

Manual build:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output:

```text
build/bin/CodexProxySwitcher.exe
```

The project uses the MSVC static runtime `/MT` by default, so the Release executable does not require an extra VC++ Runtime installation.

## Testing

```bat
cmake --build build --config Release --target app_finder_tests
ctest --test-dir build -C Release --output-on-failure
```

Tests cover Appx package parsing, npm command mapping, VS Code extension identity parsing, the built-in tool list, and legacy configuration migration.

## Project Structure

```text
src/                 Win32/C++17 source
tests/               Lightweight regression tests
scripts/             PowerShell helper launchers
docs/                Notes and analysis
release/             Committed static x64 executable
CMakeLists.txt       CMake build configuration
build.bat            Windows build script
```

## Security Notes

Environment-variable proxy mode affects only launched child processes and does not persistently modify system proxy settings. System proxy mode changes Windows Internet Options proxy settings, and the app provides a button to turn it off.

This project does not actively connect to the network, collect telemetry, or upload user configuration. It is not officially affiliated with OpenAI, Anthropic, Microsoft, Google, or other tool vendors.

## Source and License

This project is a refactored derivative of the MIT-licensed [hloolx/codex-proxy-switcher-win](https://github.com/hloolx/codex-proxy-switcher-win). Keep the upstream copyright and license notice when redistributing or modifying it.
