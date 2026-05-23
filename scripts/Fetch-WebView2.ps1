param(
    [string]$Version = "1.0.3967.48"
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$targetRoot = Join-Path $repoRoot "build\_deps\webview2"
$packageDir = Join-Path $targetRoot $Version
$packageFile = Join-Path $targetRoot "microsoft.web.webview2.$Version.nupkg"

New-Item -ItemType Directory -Force -Path $targetRoot | Out-Null

if (-not (Test-Path -LiteralPath $packageFile)) {
    $url = "https://api.nuget.org/v3-flatcontainer/microsoft.web.webview2/$Version/microsoft.web.webview2.$Version.nupkg"
    Write-Host "[INFO] Downloading Microsoft.Web.WebView2 $Version"
    Invoke-WebRequest -Uri $url -OutFile $packageFile
}

if (-not (Test-Path -LiteralPath $packageDir)) {
    Write-Host "[INFO] Extracting Microsoft.Web.WebView2 $Version"
    Expand-Archive -LiteralPath $packageFile -DestinationPath $packageDir -Force
}

$header = Join-Path $packageDir "build\native\include\WebView2.h"
$loader = Join-Path $packageDir "build\native\x64\WebView2LoaderStatic.lib"
if (-not (Test-Path -LiteralPath $header) -or -not (Test-Path -LiteralPath $loader)) {
    throw "WebView2 SDK package is incomplete: $packageDir"
}

Write-Host "[INFO] WebView2 SDK ready: $packageDir"
