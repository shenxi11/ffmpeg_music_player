# FFmpeg for Android Downloader
param(
    [string]$Version = "4.4.LTS",
    [string]$Variant = "min"
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "FFmpeg for Android Downloader" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$ProjectRoot = $PSScriptRoot
$TempDir = Join-Path $ProjectRoot "temp_ffmpeg_download"
$OutputDir = Join-Path $ProjectRoot "android_libs"

$BaseUrl = "https://github.com/tanersener/mobile-ffmpeg/releases/download/v$Version"
$FileName = "mobile-ffmpeg-$Variant-$Version.aar"
$DownloadUrl = "$BaseUrl/$FileName"
$AarFile = Join-Path $TempDir $FileName

Write-Host "[1] Preparing directories..." -ForegroundColor Yellow
if (Test-Path $TempDir) {
    Remove-Item -Recurse -Force $TempDir
}
New-Item -ItemType Directory -Force -Path $TempDir | Out-Null
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

Write-Host "    Temp: $TempDir" -ForegroundColor Gray
Write-Host "    Output: $OutputDir" -ForegroundColor Gray
Write-Host ""

if (Test-Path $OutputDir\arm64-v8a\libavcodec.so) {
    Write-Host "[?] FFmpeg libs already exist" -ForegroundColor Yellow
    $response = Read-Host "Re-download? (y/n)"
    if ($response -ne "y") {
        Write-Host "Using existing files" -ForegroundColor Green
        exit 0
    }
}

Write-Host "[2] Downloading mobile-ffmpeg-$Variant-$Version..." -ForegroundColor Yellow
Write-Host "    URL: $DownloadUrl" -ForegroundColor Gray
Write-Host ""

try {
    $webClient = New-Object System.Net.WebClient
    
    Register-ObjectEvent -InputObject $webClient -EventName DownloadProgressChanged -SourceIdentifier WebClient.DownloadProgressChanged -Action {
        $percent = $EventArgs.ProgressPercentage
        Write-Progress -Activity "Downloading $FileName" -Status "$percent% Complete" -PercentComplete $percent
    } | Out-Null
    
    $webClient.DownloadFile($DownloadUrl, $AarFile)
    
    Unregister-Event -SourceIdentifier WebClient.DownloadProgressChanged
    Write-Progress -Activity "Downloading $FileName" -Completed
    
    $sizeMB = [math]::Round((Get-Item $AarFile).Length / 1MB, 2)
    Write-Host "    [OK] Downloaded: $sizeMB MB" -ForegroundColor Green
}
catch {
    Write-Host "    [ERROR] Download failed: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please download manually:" -ForegroundColor Yellow
    Write-Host "  URL: $DownloadUrl" -ForegroundColor Gray
    Write-Host "  Save to: $AarFile" -ForegroundColor Gray
    exit 1
}

Write-Host ""
Write-Host "[3] Extracting .aar file..." -ForegroundColor Yellow

$ExtractDir = Join-Path $TempDir "extracted"
try {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::ExtractToDirectory($AarFile, $ExtractDir)
    Write-Host "    [OK] Extracted" -ForegroundColor Green
}
catch {
    Write-Host "    [ERROR] Extraction failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "[4] Copying .so libraries..." -ForegroundColor Yellow

$JniDir = Join-Path $ExtractDir "jni"
if (-not (Test-Path $JniDir)) {
    Write-Host "    [ERROR] jni directory not found" -ForegroundColor Red
    exit 1
}

$abis = @("arm64-v8a", "armeabi-v7a", "x86", "x86_64")
$copiedCount = 0

foreach ($abi in $abis) {
    $srcDir = Join-Path $JniDir $abi
    if (Test-Path $srcDir) {
        $destDir = Join-Path $OutputDir $abi
        New-Item -ItemType Directory -Force -Path $destDir | Out-Null
        
        $soFiles = Get-ChildItem -Path $srcDir -Filter "*.so"
        Copy-Item -Path "$srcDir\*" -Destination $destDir -Force
        
        Write-Host "    [OK] $abi : $($soFiles.Count) libraries" -ForegroundColor Green
        $copiedCount += $soFiles.Count
    }
    else {
        Write-Host "    [SKIP] $abi : not found" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "[5] Verifying libraries..." -ForegroundColor Yellow

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
Write-Host "[6] Cleaning up..." -ForegroundColor Yellow
Remove-Item -Recurse -Force $TempDir
Write-Host "    [OK] Cleaned" -ForegroundColor Green

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
if ($allOk) {
    Write-Host "SUCCESS: FFmpeg for Android downloaded!" -ForegroundColor Green
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
    Write-Host "ERROR: Some libraries are missing" -ForegroundColor Red
}
Write-Host "========================================" -ForegroundColor Cyan
