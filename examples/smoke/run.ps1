[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Resolve-ComponentSuperprojectRoot {
    param([string]$ModuleRoot, [string]$ComponentName)

    $modulesRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $ModuleRoot))
    if ((Split-Path -Leaf $modulesRoot) -ne "modules") { return $null }
    $candidateRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $modulesRoot))
    $manifestPath = Join-Path $modulesRoot "components.json"
    if (-not (Test-Path -LiteralPath (Join-Path $candidateRoot ".git")) -or
        -not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { return $null }
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $component = @($manifest.components | Where-Object { $_.name -eq $ComponentName }) | Select-Object -First 1
    if ($null -eq $component) { return $null }
    $expectedMount = [System.IO.Path]::GetFullPath((Join-Path $modulesRoot ([string]$component.path)))
    if ([System.StringComparer]::OrdinalIgnoreCase.Equals(
            [System.IO.Path]::GetFullPath($ModuleRoot), $expectedMount)) { return $candidateRoot }
    return $null
}

$moduleRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$architecture = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString().ToLowerInvariant()
$platformTag = "windows-$architecture"
$sdkRelease = Join-Path (Join-Path $moduleRoot "release") $platformTag
$superprojectRoot = Resolve-ComponentSuperprojectRoot -ModuleRoot $moduleRoot -ComponentName "SCP-SolverSDK"
$testRoot = if ($null -ne $superprojectRoot) { Join-Path $superprojectRoot ".tests" } else { Join-Path $moduleRoot ".tests" }
$smokeRoot = Join-Path $testRoot "smoke\SCP-SolverSDK"
$buildDirectory = Join-Path (Join-Path $smokeRoot "build") $platformTag
$smokeRelease = Join-Path (Join-Path $smokeRoot "release") $platformTag

& (Join-Path $moduleRoot "install.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "SCP-SolverSDK installation failed with exit code $LASTEXITCODE."
}

if (Test-Path -LiteralPath $buildDirectory) {
    Remove-Item -LiteralPath $buildDirectory -Recurse -Force
}
& cmake -S $PSScriptRoot -B $buildDirectory `
    -G "Visual Studio 17 2022" `
    -A x64 `
    -T v143 `
    "-DCMAKE_TRY_COMPILE_CONFIGURATION=Release" `
    "-DCMAKE_PREFIX_PATH=$sdkRelease" `
    "-DCMAKE_INSTALL_PREFIX=$smokeRelease"
if ($LASTEXITCODE -ne 0) {
    throw "SDK smoke configure failed with exit code $LASTEXITCODE."
}

& cmake --build $buildDirectory --config Release --parallel
if ($LASTEXITCODE -ne 0) {
    throw "SDK smoke build failed with exit code $LASTEXITCODE."
}

if (Test-Path -LiteralPath $smokeRelease) {
    Remove-Item -LiteralPath $smokeRelease -Recurse -Force
}
& cmake --install $buildDirectory --config Release --prefix $smokeRelease
if ($LASTEXITCODE -ne 0) {
    throw "SDK smoke install failed with exit code $LASTEXITCODE."
}

$executables = @(Get-ChildItem -LiteralPath (Join-Path $smokeRelease "bin\unstructured") `
    -File -Filter "SDKSmoke_solver_*.exe")
if ($executables.Count -ne 1) {
    throw "SDK smoke executable was not installed at the fixed location."
}
$executable = $executables[0]

$identity = (& $executable.FullName --solver-info | Out-String)
if ($LASTEXITCODE -ne 0 -or $identity -notmatch "streamcenterplus_solver_identity=1" -or
    $identity -notmatch "type=unstructuredmesh" -or
    $identity -notmatch "mesh_features=single_static,two_zone_static") {
    throw "SDK smoke solver identity validation failed.`n$identity"
}
& $executable.FullName
if ($LASTEXITCODE -ne 0) {
    throw "SDK smoke executable returned exit code $LASTEXITCODE."
}

Write-Host $identity.Trim()
Write-Host "SCP-SolverSDK smoke passed: $($executable.FullName)"
