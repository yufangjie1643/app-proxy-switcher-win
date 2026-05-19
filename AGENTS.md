# Repository Guidelines

## Project Structure & Module Organization

This is a Windows-native C++17 Win32 application built with CMake. Source files live in `src/`; keep paired declarations and implementations together as `name.hpp` and `name.cpp`. Current modules include `app_window`, `app_finder`, `config_store`, `launcher`, `system_proxy`, proxy and app selection dialogs, resources in `resource.rc`/`resource.h`, and the entry point in `main.cpp`. PowerShell helper launchers are in `scripts/`. `docs/` contains supporting analysis. `build/` is generated output and should not be edited by hand.

## Build, Test, and Development Commands

- `build.bat`: configures CMake for Visual Studio 2022 x64 and builds Release.
- `cmake -S . -B build -A x64`: manually configure the build directory.
- `cmake --build build --config Release`: build the release executable.
- `.\build\bin\Release\CodexProxySwitcher.exe`: run the built GUI.
- `.\scripts\Start-Codex-Native.ps1` and `.\scripts\Start-Codex-VPN.ps1`: smoke-test the script launch paths.

Run these from a Visual Studio Developer Command Prompt or an environment with MSVC and CMake on `PATH`.

## Coding Style & Naming Conventions

Use C++17 and Win32 APIs already present in the project; avoid new runtime dependencies unless clearly justified. Use four-space indentation and keep the existing brace style, with opening braces on function and control lines. File names are snake_case. Types use PascalCase, methods use PascalCase, private members use a trailing underscore such as `hwnd_`, and Win32 control IDs use uppercase macro names such as `IDC_BTN_NATIVE`. Preserve Unicode-aware APIs (`W` variants, `std::wstring`) and MSVC `/utf-8` compatibility.

## Testing Guidelines

There is no automated test suite in this checkout. Before submitting changes, at minimum run a clean Release build and manually verify the affected paths: native launch, proxy launch, proxy configuration persistence, and app selection if touched. For configuration changes, inspect `%APPDATA%\CodexProxySwitcher\settings.ini` and confirm legacy migration behavior still works when relevant.

## Commit & Pull Request Guidelines

No Git history is available in this checkout, so use concise imperative commit messages, for example `Fix proxy mode persistence` or `Add app selection dialog`. Pull requests should describe the user-visible change, list manual validation performed, link related issues, and include screenshots or short screen recordings for UI changes.

## Security & Configuration Tips

Do not persist system-wide proxy environment variables. Keep proxy changes scoped to the launched process or the existing temporary user-level fallback. Avoid logging proxy URLs that may contain credentials.
