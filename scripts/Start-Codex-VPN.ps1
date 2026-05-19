#requires -Version 5.1
<#
.SYNOPSIS
    VPN模式启动 Codex Desktop（注入代理环境变量）

.DESCRIPTION
    启动 OpenAI Codex Desktop 应用程序，同时注入代理环境变量。
    适用于需要通过 HTTP/HTTPS 代理访问 OpenAI API 的场景。
    会设置 HTTP_PROXY、HTTPS_PROXY、ALL_PROXY 等环境变量。

.PARAMETER CodexPath
    Codex Desktop 可执行文件的完整路径。
    如果未指定，将自动扫描常见安装位置。

.PARAMETER ProxyUrl
    代理服务器地址。默认: http://127.0.0.1:7890 (Clash 默认端口)

.PARAMETER NoProxy
    不需要代理的地址列表。默认: localhost,127.0.0.1,::1

.EXAMPLE
    .\Start-Codex-VPN.ps1
    # 使用默认代理 http://127.0.0.1:7890 启动

.EXAMPLE
    .\Start-Codex-VPN.ps1 -ProxyUrl "http://192.168.1.100:8080"
    # 使用自定义代理地址

.EXAMPLE
    .\Start-Codex-VPN.ps1 -CodexPath "C:\Program Files\OpenAI Codex\Codex.exe" -ProxyUrl "socks5://127.0.0.1:1080"
    # 指定路径和 SOCKS5 代理

.NOTES
    模式: VPN (代理注入)
    特点: 注入代理环境变量后启动
    适用: 需要通过代理访问 OpenAI API
#>
[CmdletBinding()]
param(
    [string]$CodexPath = "",

    [string]$ProxyUrl = "http://127.0.0.1:7890",

    [string]$NoProxy = "localhost,127.0.0.1,::1,.local"
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
Write-Host "  Codex Desktop - VPN模式启动" -ForegroundColor $Colors.Accent
Write-Host "========================================" -ForegroundColor $Colors.Accent
Write-Host "  模式: VPN (代理注入)" -ForegroundColor $Colors.Info
Write-Host "  代理: $ProxyUrl" -ForegroundColor $Colors.Info
Write-Host ""

# ============================================
# 验证代理地址格式
# ============================================
if (-not ($ProxyUrl -match '^https?://' -or $ProxyUrl -match '^socks5?://')) {
    Write-Host "[警告] 代理地址格式可能不正确，建议格式:" -ForegroundColor $Colors.Warning
    Write-Host "       http://127.0.0.1:7890" -ForegroundColor $Colors.Info
    Write-Host "       https://proxy.example.com:8080" -ForegroundColor $Colors.Info
    Write-Host "       socks5://127.0.0.1:1080" -ForegroundColor $Colors.Info
    Write-Host ""
}

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
        Write-Host "  .\Start-Codex-VPN.ps1 -CodexPath \"C:\Your\Path\Codex.exe\"" -ForegroundColor $Colors.Info
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
# 检查代理可用性
# ============================================
Write-Host ""
Write-Host "[检查] 正在验证代理连接..." -ForegroundColor $Colors.Info

# 尝试解析代理地址
$proxyUri = $null
if ([Uri]::TryCreate($ProxyUrl, [UriKind]::Absolute, [ref]$proxyUri)) {
    $proxyHost = $proxyUri.Host
    $proxyPort = $proxyUri.Port

    # 尝试连接代理端口
    try {
        $tcpClient = New-Object System.Net.Sockets.TcpClient
        $connect = $tcpClient.BeginConnect($proxyHost, $proxyPort, $null, $null)
        $success = $connect.AsyncWaitHandle.WaitOne(2000, $false)

        if ($success -and $tcpClient.Connected) {
            Write-Host "[成功] 代理服务器 $proxyHost`:$proxyPort 连接正常" -ForegroundColor $Colors.Success
            $tcpClient.Close()
        } else {
            Write-Host "[警告] 无法连接到代理服务器 $proxyHost`:$proxyPort" -ForegroundColor $Colors.Warning
            Write-Host "       请确认代理服务是否已启动" -ForegroundColor $Colors.Warning
            Write-Host "       如果代理在其他位置运行，可忽略此警告" -ForegroundColor $Colors.Warning
        }
    }
    catch {
        Write-Host "[警告] 代理检测失败: $_" -ForegroundColor $Colors.Warning
    }
}

# ============================================
# 显示注入的环境变量
# ============================================
Write-Host ""
Write-Host "[配置] 将注入以下代理环境变量:" -ForegroundColor $Colors.Info
Write-Host "       HTTP_PROXY   = $ProxyUrl" -ForegroundColor $Colors.Accent
Write-Host "       HTTPS_PROXY  = $ProxyUrl" -ForegroundColor $Colors.Accent
Write-Host "       ALL_PROXY    = $ProxyUrl" -ForegroundColor $Colors.Accent
Write-Host "       NO_PROXY     = $NoProxy" -ForegroundColor $Colors.Accent
Write-Host ""

# ============================================
# 启动 Codex (VPN模式)
# ============================================
Write-Host "[启动] 正在启动 Codex Desktop (VPN模式)..." -ForegroundColor $Colors.Info
Write-Host "       程序: $CodexPath" -ForegroundColor $Colors.Info

# 创建启动信息 - 注入代理环境变量
$startInfo = New-Object System.Diagnostics.ProcessStartInfo
$startInfo.FileName = $CodexPath
$startInfo.UseShellExecute = $true
$startInfo.WorkingDirectory = Split-Path $CodexPath -Parent

# 注入代理环境变量 (Electron/Node.js 会读取这些变量)
$startInfo.EnvironmentVariables["HTTP_PROXY"] = $ProxyUrl
$startInfo.EnvironmentVariables["HTTPS_PROXY"] = $ProxyUrl
$startInfo.EnvironmentVariables["ALL_PROXY"] = $ProxyUrl
$startInfo.EnvironmentVariables["NO_PROXY"] = $NoProxy

# 小写版本 (某些应用读取小写变量)
$startInfo.EnvironmentVariables["http_proxy"] = $ProxyUrl
$startInfo.EnvironmentVariables["https_proxy"] = $ProxyUrl
$startInfo.EnvironmentVariables["all_proxy"] = $ProxyUrl
$startInfo.EnvironmentVariables["no_proxy"] = $NoProxy

# 额外: WebSocket 代理 (Codex 可能需要)
if ($ProxyUrl -match '^http') {
    $startInfo.EnvironmentVariables["WS_PROXY"] = $ProxyUrl
    $startInfo.EnvironmentVariables["WSS_PROXY"] = $ProxyUrl
}

try {
    $process = [System.Diagnostics.Process]::Start($startInfo)

    if ($process) {
        Write-Host ""
        Write-Host "[成功] Codex Desktop 已启动!" -ForegroundColor $Colors.Success
        Write-Host "       进程ID: $($process.Id)" -ForegroundColor $Colors.Success
        Write-Host "       启动时间: $($process.StartTime)" -ForegroundColor $Colors.Success
        Write-Host ""
        Write-Host "========================================" -ForegroundColor $Colors.Success
        Write-Host "  Codex 正在使用代理: $ProxyUrl" -ForegroundColor $Colors.Accent
        Write-Host "========================================" -ForegroundColor $Colors.Success
        Write-Host ""
        Write-Host "  提示: Codex 现在会通过代理连接到 OpenAI API" -ForegroundColor $Colors.Info
        Write-Host "  如需无代理启动，请运行:" -ForegroundColor $Colors.Info
        Write-Host "  .\Start-Codex-Native.ps1" -ForegroundColor $Colors.Accent
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
