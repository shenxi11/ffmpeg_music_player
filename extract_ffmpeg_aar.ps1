# Extract FFmpeg .aar file
param(
    [string]$AarFile = ""
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "FFmpeg .aar Extractor" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$ProjectRoot = $PSScriptRoot
$OutputDir = Join-Path $ProjectRoot "android_libs"

if ($AarFile -eq "") {
    $AarFile = Join-Path $ProjectRoot "temp_ffmpeg_download\mobile-ffmpeg-min-4.4.LTS.aar"
    
    if (-not (Test-Path $AarFile)) {
        Write-Host "[ERROR] .aar file not found at: $AarFile" -ForegroundColor Red
        Write-Host ""
        Write-Host "Please:" -ForegroundColor Yellow
        Write-Host "1. Download: https://github.com/tanersener/mobile-ffmpeg/releases/download/v4.4.LTS/mobile-ffmpeg-min-4.4.LTS.aar" -ForegroundColor Gray
        Write-Host "2. Save to: $AarFile" -ForegroundColor Gray
        Write-Host "3. Run this script again" -ForegroundColor Gray
        Write-Host ""
        Write-Host "Or specify .aar file path:" -ForegroundColor Yellow
        Write-Host "  .\extract_ffmpeg_aar.ps1 -AarFile `"path\to\file.aar`"" -ForegroundColor Gray
        exit 1
    }
}

if (-not (Test-Path $AarFile)) {
    Write-Host "[ERROR] File not found: $AarFile" -ForegroundColor Red
    exit 1
}

Write-Host "[1] Found .aar file" -ForegroundColor Green
Write-Host "    Path: $AarFile" -ForegroundColor Gray
$sizeMB = [math]::Round((Get-Item $AarFile).Length / 1MB, 2)
Write-Host "    Size: $sizeMB MB" -ForegroundColor Gray
Write-Host ""

$TempDir = Join-Path $ProjectRoot "temp_extract"
if (Test-Path $TempDir) {
    Remove-Item -Recurse -Force $TempDir
}
New-Item -ItemType Directory -Force -Path $TempDir | Out-Null

Write-Host "[2] Extracting .aar file..." -ForegroundColor Yellow
try {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::ExtractToDirectory($AarFile, $TempDir)
    Write-Host "    [OK] Extracted" -ForegroundColor Green
}
catch {
    Write-Host "    [ERROR] Extraction failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "[3] Copying .so libraries..." -ForegroundColor Yellow

$JniDir = Join-Path $TempDir "jni"
if (-not (Test-Path $JniDir)) {
    Write-Host "    [ERROR] jni directory not found in .aar" -ForegroundColor Red
    Write-Host "    Extracted contents:" -ForegroundColor Gray
    Get-ChildItem $TempDir | ForEach-Object { Write-Host "      - $($_.Name)" -ForegroundColor Gray }
    exit 1
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$abis = @("arm64-v8a", "armeabi-v7a", "x86", "x86_64")
$copiedCount = 0

foreach ($abi in $abis) {
    $srcDir = Join-Path $JniDir $abi
    if (Test-Path $srcDir) {
        $destDir = Join-Path $OutputDir $abi
        New-Item -ItemType Directory -Force -Path $destDir | Out-Null
        
        $soFiles = Get-ChildItem -Path $srcDir -Filter "*.so"
        foreach ($soFile in $soFiles) {
            Copy-Item -Path $soFile.FullName -Destination $destDir -Force
            $copiedCount++
        }
        
        Write-Host "    [OK] $abi : $($soFiles.Count) libraries copied" -ForegroundColor Green
    }
    else {
        Write-Host "    [SKIP] $abi : not found" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "[4] Verifying libraries..." -ForegroundColor Yellow

$requiredLibs = @("libavcodec.so", "libavformat.so", "libavutil.so", "libswresample.so")
$allOk = $true

foreach ($abi in @("arm64-v8a", "armeabi-v7a")) {
    $abiDir = Join-Path $OutputDir $abi
    Write-Host "    Checking $abi..." -ForegroundColor Gray
    
    foreach ($lib in $requiredLibs) {
        $libPath = Join-Path $abiDir $lib
        if (Test-Path $libPath) {
            $sizeKB = [math]::Round((Get-Item $libPath).Length / 1KB, 0)
            Write-Host "      [OK] $lib ($sizeKB KB)" -ForegroundColor Green
        }
        else {
            Write-Host "      [MISSING] $lib" -ForegroundColor Red
            $allOk = $false
        }
    }
}

Write-Host ""
Write-Host "[5] Cleaning up..." -ForegroundColor Yellow
Remove-Item -Recurse -Force $TempDir
Write-Host "    [OK] Temp files removed" -ForegroundColor Green

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
if ($allOk) {
    Write-Host "SUCCESS: FFmpeg libraries extracted!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Location: $OutputDir" -ForegroundColor Cyan
    Write-Host "Total libraries: $copiedCount" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Yellow
    Write-Host "1. Verify: .\verify_ffmpeg_libs.bat" -ForegroundColor Gray
    Write-Host "2. Modify CMakeLists.txt for Android" -ForegroundColor Gray
    Write-Host "3. Configure Qt Creator Android Kit" -ForegroundColor Gray
}
else {
    Write-Host "WARNING: Some libraries are missing" -ForegroundColor Yellow
    Write-Host "The .aar file may not contain all required libraries" -ForegroundColor Gray
}
Write-Host "========================================" -ForegroundColor Cyan
