# Download mobile-ffmpeg from Maven Central
$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "FFmpeg AAR Downloader (Maven Central)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$ProjectRoot = "E:\FFmpeg_whisper\ffmpeg_music_player"
$TempDir = Join-Path $ProjectRoot "temp_ffmpeg_download"
$AarFile = Join-Path $TempDir "mobile-ffmpeg-min-4.4.LTS.aar"

# Maven Central URL
$MavenUrl = "https://repo1.maven.org/maven2/com/arthenica/mobile-ffmpeg-min/4.4.LTS/mobile-ffmpeg-min-4.4.LTS.aar"

Write-Host "[INFO] Downloading from Maven Central" -ForegroundColor Yellow
Write-Host "  Source: Maven Central Repository" -ForegroundColor Gray
Write-Host "  Package: com.arthenica:mobile-ffmpeg-min:4.4.LTS" -ForegroundColor Gray
Write-Host "  Size: ~10-15 MB" -ForegroundColor Gray
Write-Host ""

# Create directory
New-Item -ItemType Directory -Force -Path $TempDir | Out-Null

if (Test-Path $AarFile) {
    $sizeMB = [math]::Round((Get-Item $AarFile).Length / 1MB, 2)
    Write-Host "[?] File already exists ($sizeMB MB)" -ForegroundColor Yellow
    $response = Read-Host "Re-download? (y/n)"
    if ($response -ne "y") {
        Write-Host "[OK] Using existing file" -ForegroundColor Green
        Write-Host ""
        Write-Host "Next step: .\extract_ffmpeg_aar.ps1" -ForegroundColor Cyan
        exit 0
    }
}

Write-Host "[1] Downloading..." -ForegroundColor Yellow
Write-Host "    URL: $MavenUrl" -ForegroundColor Gray
Write-Host ""

try {
    $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest -Uri $MavenUrl -OutFile $AarFile -UseBasicParsing
    $ProgressPreference = 'Continue'
    
    $sizeMB = [math]::Round((Get-Item $AarFile).Length / 1MB, 2)
    Write-Host ""
    Write-Host "[SUCCESS] Download completed!" -ForegroundColor Green
    Write-Host "    File: $AarFile" -ForegroundColor Gray
    Write-Host "    Size: $sizeMB MB" -ForegroundColor Gray
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Next steps:" -ForegroundColor Yellow
    Write-Host "1. Extract: .\extract_ffmpeg_aar.ps1" -ForegroundColor White
    Write-Host "2. Verify: .\verify_ffmpeg_libs.bat" -ForegroundColor White
    Write-Host "========================================" -ForegroundColor Cyan
}
catch {
    Write-Host ""
    Write-Host "[ERROR] Download failed!" -ForegroundColor Red
    Write-Host "    Error: $($_.Exception.Message)" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Trying alternative sources..." -ForegroundColor Yellow
    Write-Host ""
    
    # Try alternative: GitHub
    $GitHubUrl = "https://github.com/tanersener/mobile-ffmpeg/releases/download/v4.4.LTS/mobile-ffmpeg-min-4.4.LTS.aar"
    Write-Host "[2] Trying GitHub Releases..." -ForegroundColor Yellow
    Write-Host "    URL: $GitHubUrl" -ForegroundColor Gray
    
    try {
        $ProgressPreference = 'SilentlyContinue'
        Invoke-WebRequest -Uri $GitHubUrl -OutFile $AarFile -UseBasicParsing
        $ProgressPreference = 'Continue'
        
        $sizeMB = [math]::Round((Get-Item $AarFile).Length / 1MB, 2)
        Write-Host ""
        Write-Host "[SUCCESS] Download completed!" -ForegroundColor Green
        Write-Host "    File: $AarFile" -ForegroundColor Gray
        Write-Host "    Size: $sizeMB MB" -ForegroundColor Gray
        Write-Host ""
        Write-Host "Next step: .\extract_ffmpeg_aar.ps1" -ForegroundColor Cyan
    }
    catch {
        Write-Host ""
        Write-Host "[ERROR] All download attempts failed!" -ForegroundColor Red
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Yellow
        Write-Host "MANUAL DOWNLOAD REQUIRED" -ForegroundColor Yellow
        Write-Host "========================================" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "Please try one of these sources:" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "Source 1 - Maven Central:" -ForegroundColor White
        Write-Host "  $MavenUrl" -ForegroundColor Green
        Write-Host ""
        Write-Host "Source 2 - GitHub Releases:" -ForegroundColor White
        Write-Host "  $GitHubUrl" -ForegroundColor Green
        Write-Host ""
        Write-Host "Source 3 - ffmpeg-kit (newer version):" -ForegroundColor White
        Write-Host "  https://github.com/arthenica/ffmpeg-kit/releases" -ForegroundColor Green
        Write-Host ""
        Write-Host "Save downloaded file to:" -ForegroundColor White
        Write-Host "  $AarFile" -ForegroundColor Green
        Write-Host ""
        Write-Host "Then run: .\extract_ffmpeg_aar.ps1" -ForegroundColor Cyan
        exit 1
    }
}
