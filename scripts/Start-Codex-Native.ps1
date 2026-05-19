#requires -Version 5.1
<#
.SYNOPSIS
    原生模式启动 Codex Desktop（无代理注入）

.DESCRIPTION
    直接启动 OpenAI Codex Desktop 应用程序，不注入任何代理环境变量。
    适用于已经使用系统级代理（如 VPN、Clash 系统代理模式）的场景。

.PARAMETER CodexPath
    Codex Desktop 可执行文件的完整路径。
    如果未指定，将自动扫描常见安装位置。

.EXAMPLE
    .\Start-Codex-Native.ps1
    # 自动查找 Codex 并启动

.EXAMPLE
    .\Start-Codex-Native.ps1 -CodexPath "C:\Program Files\OpenAI Codex\Codex.exe"
    # 使用指定路径启动

.NOTES
    模式: Native
    特点: 不注入代理，直接启动
    适用: 系统已配置全局代理
#>
[CmdletBinding()]
param(
    [string]$CodexPath = ""
)

# 颜色定义
$Colors = @{
    Success = 'Green'
    Error   = 'Red'
    Warning = 'Yellow'
    Info    = 'Cyan'
    Accent  = 'Magenta'
}

Write-Host ""
Write-Host "========================================" -ForegroundColor $Colors.Accent
Write-Host "  Codex Desktop - 原生模式启动" -ForegroundColor $Colors.Accent
Write-Host "========================================" -ForegroundColor $Colors.Accent
Write-Host "  模式: Native (无代理注入)" -ForegroundColor $Colors.Info
Write-Host ""

# ============================================
# 自动查找 Codex 路径
# ============================================
if ([string]::IsNullOrWhiteSpace($CodexPath)) {
    Write-Host "[扫描] 正在查找 Codex Desktop 安装位置..." -ForegroundColor $Colors.Info

    $searchPaths = @(
        # Windows 默认安装路径
        "$env:LOCALAPPDATA\Programs\OpenAI Codex\Codex.exe",
        "$env:LOCALAPPDATA\Programs\OpenAI Codex\app.exe",
        "$env:PROGRAMFILES\OpenAI Codex\Codex.exe",
        "${env:PROGRAMFILES(x86)}\OpenAI Codex\Codex.exe",
        # 用户自定义路径
        "$env:USERPROFILE\AppData\Local\Programs\OpenAI Codex\Codex.exe",
        # 其他可能位置
        "$env:LOCALAPPDATA\OpenAI Codex\Codex.exe"
    )

    $found = $false
    foreach ($path in $searchPaths) {
        if (Test-Path $path) {
            $CodexPath = $path
            Write-Host "[发现] $CodexPath" -ForegroundColor $Colors.Success
            $found = $true
            break
        }
    }

    if (-not $found) {
        Write-Host "[错误] 未在常见位置找到 Codex Desktop!" -ForegroundColor $Colors.Error
        Write-Host ""
        Write-Host "请确认已安装 Codex Desktop，或使用 -CodexPath 参数指定路径:" -ForegroundColor $Colors.Warning
        Write-Host "  .\Start-Codex-Native.ps1 -CodexPath \"C:\Your\Path\Codex.exe\"" -ForegroundColor $Colors.Info
        Write-Host ""
        exit 1
    }
} else {
    if (-not (Test-Path $CodexPath)) {
        Write-Host "[错误] 指定的路径不存在: $CodexPath" -ForegroundColor $Colors.Error
        exit 1
    }
}

# ============================================
# 启动 Codex (原生模式)
# ============================================
Write-Host ""
Write-Host "[启动] 正在启动 Codex Desktop (原生模式)..." -ForegroundColor $Colors.Info
Write-Host "       程序: $CodexPath" -ForegroundColor $Colors.Info

# 创建启动信息 - 不注入任何代理变量
$startInfo = New-Object System.Diagnostics.ProcessStartInfo
$startInfo.FileName = $CodexPath
$startInfo.UseShellExecute = $true
$startInfo.WorkingDirectory = Split-Path $CodexPath -Parent

try {
    $process = [System.Diagnostics.Process]::Start($startInfo)

    if ($process) {
        Write-Host ""
        Write-Host "[成功] Codex Desktop 已启动!" -ForegroundColor $Colors.Success
        Write-Host "       进程ID: $($process.Id)" -ForegroundColor $Colors.Success
        Write-Host "       启动时间: $($process.StartTime)" -ForegroundColor $Colors.Success
        Write-Host ""
        Write-Host "========================================" -ForegroundColor $Colors.Success
        Write-Host "  提示: 原生模式未注入代理配置" -ForegroundColor $Colors.Warning
        Write-Host "  如需使用代理，请运行:" -ForegroundColor $Colors.Info
        Write-Host "  .\Start-Codex-VPN.ps1" -ForegroundColor $Colors.Accent
        Write-Host "========================================" -ForegroundColor $Colors.Success
        Write-Host ""
    } else {
        Write-Host "[警告] 进程已启动但无法获取进程信息" -ForegroundColor $Colors.Warning
    }
}
catch {
    Write-Host ""
    Write-Host "[错误] 启动失败!" -ForegroundColor $Colors.Error
    Write-Host "       错误信息: $_" -ForegroundColor $Colors.Error
    Write-Host ""
    exit 1
}
