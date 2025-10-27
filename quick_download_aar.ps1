# Quick download script for mobile-ffmpeg .aar
$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Mobile FFmpeg AAR Downloader" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$ProjectRoot = "E:\FFmpeg_whisper\ffmpeg_music_player"
$TempDir = Join-Path $ProjectRoot "temp_ffmpeg_download"
$AarFile = Join-Path $TempDir "mobile-ffmpeg-min-4.4.LTS.aar"
$Url = "https://github.com/tanersener/mobile-ffmpeg/releases/download/v4.4.LTS/mobile-ffmpeg-min-4.4.LTS.aar"

Write-Host "[INFO] This will download:" -ForegroundColor Yellow
Write-Host "  File: mobile-ffmpeg-min-4.4.LTS.aar" -ForegroundColor Gray
Write-Host "  Size: ~10-15 MB" -ForegroundColor Gray
Write-Host "  From: GitHub Releases" -ForegroundColor Gray
Write-Host ""

# Create directory
New-Item -ItemType Directory -Force -Path $TempDir | Out-Null

if (Test-Path $AarFile) {
    Write-Host "[?] File already exists: $AarFile" -ForegroundColor Yellow
    $sizeMB = [math]::Round((Get-Item $AarFile).Length / 1MB, 2)
    Write-Host "    Size: $sizeMB MB" -ForegroundColor Gray
    $response = Read-Host "Use existing file? (y/n)"
    if ($response -eq "y") {
        Write-Host "[OK] Using existing file" -ForegroundColor Green
        Write-Host ""
        Write-Host "Next step: Run .\extract_ffmpeg_aar.ps1" -ForegroundColor Cyan
        exit 0
    }
}

Write-Host "[1] Downloading from GitHub..." -ForegroundColor Yellow
Write-Host "    URL: $Url" -ForegroundColor Gray
Write-Host ""
Write-Host "    If download fails, please:" -ForegroundColor Yellow
Write-Host "    - Use proxy/VPN" -ForegroundColor Gray
Write-Host "    - Download manually from browser" -ForegroundColor Gray
Write-Host "    - Save to: $AarFile" -ForegroundColor Gray
Write-Host ""

try {
    # Method 1: WebClient with progress
    $webClient = New-Object System.Net.WebClient
    
    # Set timeout
    $webClient.Headers.Add("User-Agent", "Mozilla/5.0")
    
    # Progress handler
    Register-ObjectEvent -InputObject $webClient -EventName DownloadProgressChanged -SourceIdentifier Download.ProgressChanged -Action {
        $percent = $EventArgs.ProgressPercentage
        $received = $EventArgs.BytesReceived / 1MB
        $total = $EventArgs.TotalBytesToReceive / 1MB
        Write-Progress -Activity "Downloading mobile-ffmpeg-min-4.4.LTS.aar" `
                       -Status "$([math]::Round($received, 2)) MB / $([math]::Round($total, 2)) MB" `
                       -PercentComplete $percent
    } | Out-Null
    
    # Start download
    Write-Host "    Starting download..." -ForegroundColor Gray
    $webClient.DownloadFile($Url, $AarFile)
    
    # Cleanup
    Unregister-Event -SourceIdentifier Download.ProgressChanged
    Write-Progress -Activity "Downloading" -Completed
    
    $sizeMB = [math]::Round((Get-Item $AarFile).Length / 1MB, 2)
    Write-Host ""
    Write-Host "[SUCCESS] Download completed!" -ForegroundColor Green
    Write-Host "    File: $AarFile" -ForegroundColor Gray
    Write-Host "    Size: $sizeMB MB" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Next step:" -ForegroundColor Cyan
    Write-Host "  .\extract_ffmpeg_aar.ps1" -ForegroundColor White
}
catch {
    Write-Host ""
    Write-Host "[ERROR] Download failed!" -ForegroundColor Red
    Write-Host "    Error: $($_.Exception.Message)" -ForegroundColor Gray
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Yellow
    Write-Host "MANUAL DOWNLOAD REQUIRED" -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Please download manually:" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "1. Open browser and go to:" -ForegroundColor White
    Write-Host "   https://github.com/tanersener/mobile-ffmpeg/releases/tag/v4.4.LTS" -ForegroundColor Green
    Write-Host ""
    Write-Host "2. Scroll down to 'Assets' section" -ForegroundColor White
    Write-Host ""
    Write-Host "3. Click and download:" -ForegroundColor White
    Write-Host "   mobile-ffmpeg-min-4.4.LTS.aar" -ForegroundColor Green
    Write-Host ""
    Write-Host "4. Save to:" -ForegroundColor White
    Write-Host "   $AarFile" -ForegroundColor Green
    Write-Host ""
    Write-Host "5. Then run:" -ForegroundColor White
    Write-Host "   .\extract_ffmpeg_aar.ps1" -ForegroundColor Green
    Write-Host ""
    exit 1
}
