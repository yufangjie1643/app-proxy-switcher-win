# Security Notes

This project is a local Windows launcher. It does not make outbound network requests, collect telemetry, or upload user configuration.

## Proxy Scope

- Environment-variable mode injects proxy variables only into the launched child process.
- System-proxy mode modifies the current user's Windows Internet Options proxy settings and exposes a button to disable them.
- The app does not write machine-wide environment variables.

## Local Configuration

Settings are stored in:

```text
%APPDATA%\CodexProxySwitcher\settings.ini
```

The file may contain proxy host and port values. Avoid storing proxy credentials in URLs.

## Application Discovery

Discovery uses local-only mechanisms:

- `Get-AppxPackage` and WindowsApps directory inspection for MSIX apps.
- Start Menu `.lnk` parsing for desktop EXE apps such as Claude Desktop.
- PATH and npm global directories for CLI tools.
- VS Code-compatible extension folders and `package.json` identity checks for editor agents.

## Runtime Dependencies

Release builds use the MSVC static runtime (`/MT`). The published x64 executable depends only on Windows system DLLs.
