@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ========================================
echo   CodexProxySwitcher C++ Build Script
echo ========================================
echo.

where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake not found.
    echo.
    echo Please install CMake from https://cmake.org/download/
    echo Or use: winget install Kitware.CMake
    exit /b 1
)

echo [INFO] CMake found.
cmake --version
echo.

where powershell >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] PowerShell not found. WebView2 SDK bootstrap requires PowerShell.
    exit /b 1
)

echo [INFO] Preparing WebView2 SDK...
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\Fetch-WebView2.ps1
if %errorlevel% neq 0 (
    echo [ERROR] WebView2 SDK preparation failed.
    exit /b 1
)
echo.

if not exist build mkdir build

echo [1/3] Configuring...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_WEBVIEW2=ON
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    echo.
    echo Trying fallback without explicit generator...
    cmake -S . -B build
    if %errorlevel% neq 0 (
        echo [ERROR] CMake configuration failed.
        exit /b 1
    )
)

echo.
echo [2/3] Building Release...
cmake --build build --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo.
echo ========================================
echo   Build successful!
echo   Output: build\bin\CodexProxySwitcher.exe
echo   WebUI:  build\bin\CodexProxySwitcherWebView2.exe
echo ========================================

endlocal
