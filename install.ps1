[CmdletBinding()]
param(
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repoRoot = $PSScriptRoot
$architecture = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString().ToLowerInvariant()
$platformTag = "windows-$architecture"
$BuildDir = Join-Path (Join-Path $repoRoot ".build") $platformTag
$InstallDir = Join-Path (Join-Path $repoRoot "release") $platformTag

if ($Clean) {
    $buildRoot = [System.IO.Path]::GetFullPath((Join-Path $repoRoot ".build"))
    $releaseRoot = [System.IO.Path]::GetFullPath((Join-Path $repoRoot "release"))
    $installRoot = [System.IO.Path]::GetFullPath($InstallDir)
    if (-not $buildRoot.StartsWith($repoRoot + [System.IO.Path]::DirectorySeparatorChar,
                                   [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean a build directory outside SCP-SolverSDK: $buildRoot"
    }
    if (-not $installRoot.StartsWith($releaseRoot + [System.IO.Path]::DirectorySeparatorChar,
                                     [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean an install directory outside SCP-SolverSDK/release: $installRoot"
    }
    if (Test-Path -LiteralPath $buildRoot) {
        Remove-Item -LiteralPath $buildRoot -Recurse -Force
    }
    if (Test-Path -LiteralPath $installRoot) {
        Remove-Item -LiteralPath $installRoot -Recurse -Force
    }
}

$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if ($null -eq $cmake) {
    throw "CMake 3.24 or newer is required and was not found on PATH."
}

& $cmake.Source -S $repoRoot -B $BuildDir `
    -G "Visual Studio 17 2022" `
    -A x64 `
    -T v143 `
    "-DCMAKE_TRY_COMPILE_CONFIGURATION=Release" `
    "-DCMAKE_INSTALL_PREFIX=$InstallDir"
if ($LASTEXITCODE -ne 0) {
    throw "SCP-SolverSDK CMake configure failed with exit code $LASTEXITCODE."
}

& $cmake.Source --build $BuildDir --config Release --parallel
if ($LASTEXITCODE -ne 0) {
    throw "SCP-SolverSDK build failed with exit code $LASTEXITCODE."
}

& $cmake.Source --install $BuildDir --config Release --prefix $InstallDir
if ($LASTEXITCODE -ne 0) {
    throw "SCP-SolverSDK install failed with exit code $LASTEXITCODE."
}

Write-Host "SCP-SolverSDK installed to $InstallDir"
