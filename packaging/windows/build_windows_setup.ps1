[CmdletBinding()]
param(
    [string]$BuildDir = "",
    [ValidateSet("Release", "RelWithDebInfo")]
    [string]$Config = "Release",
    [string]$Version = "",
    [string]$StageDir = "",
    [string]$OutputDir = "",
    [string]$IsccPath = "",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

function Write-Step([string]$Message) {
    Write-Host "[PACK] $Message" -ForegroundColor Cyan
}

function Invoke-External([string]$File, [string[]]$Arguments) {
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed: $File $($Arguments -join ' ') (exit=$LASTEXITCODE)"
    }
}

function Resolve-IsccPath([string]$GivenPath) {
    if (-not [string]::IsNullOrWhiteSpace($GivenPath)) {
        if (Test-Path $GivenPath) { return (Resolve-Path $GivenPath).Path }
        throw "ISCC not found at: $GivenPath"
    }

    if (-not [string]::IsNullOrWhiteSpace($env:ISCC_EXE) -and (Test-Path $env:ISCC_EXE)) {
        return (Resolve-Path $env:ISCC_EXE).Path
    }

    $candidates = @(
        "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
        "$env:ProgramFiles(x86)\Inno Setup 6\ISCC.exe",
        "$env:ProgramFiles\Inno Setup 6\ISCC.exe"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw "Inno Setup Compiler (ISCC.exe) not found. Install Inno Setup 6 or pass -IsccPath."
}

function Resolve-Version([string]$RepoRoot, [string]$GivenVersion) {
    if (-not [string]::IsNullOrWhiteSpace($GivenVersion)) {
        return $GivenVersion.Trim()
    }

    $cmakeFile = Join-Path $RepoRoot "CMakeLists.txt"
    $content = Get-Content -Raw -Path $cmakeFile
    $m = [regex]::Match($content, "project\s*\([^\)]*VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)")
    if ($m.Success) {
        return $m.Groups[1].Value
    }
    return "1.0.0"
}

function Copy-DistributionTree([string]$FromDir, [string]$ToDir) {
    $excludeExt = @(".pdb", ".ilk", ".exp", ".lib", ".ipdb", ".iobj")
    $excludeFiles = @("debug.log")

    Get-ChildItem -Path $FromDir -Recurse -File | ForEach-Object {
        $relative = $_.FullName.Substring($FromDir.Length).TrimStart('\', '/')
        $ext = $_.Extension.ToLowerInvariant()
        if ($excludeExt -contains $ext) { return }
        if ($excludeFiles -contains $_.Name) { return }

        $targetPath = Join-Path $ToDir $relative
        $targetDir = Split-Path -Parent $targetPath
        if (-not (Test-Path $targetDir)) {
            New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
        }
        Copy-Item -Path $_.FullName -Destination $targetPath -Force
    }
}

function Assert-ReleaseSanity([string]$Dir) {
    $debugMarkers = @(
        "Qt5Cored.dll",
        "Qt5Guid.dll",
        "Qt5Widgetsd.dll",
        "Qt5Networkd.dll",
        "msvcp140d.dll",
        "vcruntime140d.dll"
    )
    foreach ($marker in $debugMarkers) {
        if (Test-Path (Join-Path $Dir $marker)) {
            throw "Staging directory contains debug runtime: $marker. Please package Release build."
        }
    }
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $repoRoot "build"
}
if (Test-Path -LiteralPath $BuildDir) {
    $BuildDir = (Resolve-Path -LiteralPath $BuildDir).Path
} else {
    $BuildDir = [System.IO.Path]::GetFullPath($BuildDir)
}

if ([string]::IsNullOrWhiteSpace($StageDir)) {
    $StageDir = Join-Path $BuildDir "package\staging\$Config"
}
if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $OutputDir = Join-Path $BuildDir "package\output"
}

$Version = Resolve-Version -RepoRoot $repoRoot -GivenVersion $Version
$isccExe = Resolve-IsccPath -GivenPath $IsccPath
$issFile = Join-Path $PSScriptRoot "cloudmusic_installer.iss"

Write-Step "RepoRoot: $repoRoot"
Write-Step "BuildDir: $BuildDir"
Write-Step "Config: $Config"
Write-Step "Version: $Version"
Write-Step "ISCC: $isccExe"

if (-not $SkipBuild) {
    Write-Step "Configuring CMake..."
    Invoke-External -File "cmake" -Arguments @("-S", $repoRoot, "-B", $BuildDir)

    Write-Step "Building all targets ($Config)..."
    Invoke-External -File "cmake" -Arguments @("--build", $BuildDir, "--config", $Config)
}

$runtimeDir = Join-Path $BuildDir $Config
$exePath = Join-Path $runtimeDir "ffmpeg_music_player.exe"
if (-not (Test-Path $exePath)) {
    throw "Executable not found: $exePath"
}

Write-Step "Preparing staging directory..."
if (Test-Path $StageDir) {
    Remove-Item -Path $StageDir -Recurse -Force
}
New-Item -ItemType Directory -Path $StageDir -Force | Out-Null

Copy-DistributionTree -FromDir $runtimeDir -ToDir $StageDir
Assert-ReleaseSanity -Dir $StageDir

$pluginDir = Join-Path $runtimeDir "plugin"
if (-not (Test-Path $pluginDir)) {
    Write-Warning "Plugin directory not found: $pluginDir"
} else {
    $pluginDllCount = (Get-ChildItem -Path $pluginDir -Filter "*.dll" -ErrorAction SilentlyContinue | Measure-Object).Count
    if ($pluginDllCount -eq 0) {
        Write-Warning "No plugin DLL found in $pluginDir. Installer plugin components will be empty."
    }
}

if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

Write-Step "Building installer..."
$isccArgs = @(
    "/DSourceDir=$StageDir",
    "/DMyAppVersion=$Version",
    "/DOutputDir=$OutputDir",
    $issFile
)
Invoke-External -File $isccExe -Arguments $isccArgs

$setup = Get-ChildItem -Path $OutputDir -Filter "*.exe" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($null -eq $setup) {
    throw "Setup exe not found in output dir: $OutputDir"
}

Write-Step "Installer created: $($setup.FullName)"
Write-Host ""
Write-Host "Done." -ForegroundColor Green

