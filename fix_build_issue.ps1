# Qt 构建问题快速修复脚本
# 用于解决 windeployqt 文件占用问题

param(
    [switch]$FullClean = $false,
    [switch]$KillProcesses = $false,
    [string]$BuildDir = "E:\QT_CMAKE\ffmpeg_musicer_player_build"
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Qt Build Issue Quick Fix" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 1. 可选: 结束可能占用文件的进程
if ($KillProcesses) {
    Write-Host "[1/4] Terminating ffmpeg_music_player processes..." -ForegroundColor Yellow
    Get-Process | Where-Object {$_.Name -like "*ffmpeg_music_player*"} | ForEach-Object {
        Write-Host "  Killing process: $($_.Name) (PID: $($_.Id))" -ForegroundColor Gray
        Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue
    }
    Start-Sleep -Seconds 1
    Write-Host "  Done!" -ForegroundColor Green
} else {
    Write-Host "[1/4] Skipping process termination (use -KillProcesses to enable)" -ForegroundColor Gray
}

Write-Host ""

# 2. 删除部署标记文件(强制重新部署)
Write-Host "[2/4] Removing Qt deployment markers..." -ForegroundColor Yellow
$markers = @(
    "$BuildDir\Debug\.qt_deployed_Debug",
    "$BuildDir\Release\.qt_deployed_Release"
)

foreach ($marker in $markers) {
    if (Test-Path $marker) {
        Write-Host "  Removing: $marker" -ForegroundColor Gray
        Remove-Item $marker -Force -ErrorAction SilentlyContinue
    }
}
Write-Host "  Done!" -ForegroundColor Green
Write-Host ""

# 3. 清理有问题的 Qt 部署文件
if ($FullClean) {
    Write-Host "[3/4] Performing FULL clean (this will trigger complete rebuild)..." -ForegroundColor Yellow
    if (Test-Path $BuildDir) {
        Write-Host "  Removing entire build directory: $BuildDir" -ForegroundColor Gray
        Remove-Item "$BuildDir\*" -Recurse -Force -ErrorAction SilentlyContinue
    }
    Write-Host "  Done!" -ForegroundColor Green
} else {
    Write-Host "[3/4] Cleaning Qt deployment files..." -ForegroundColor Yellow
    $qtDirs = @(
        "$BuildDir\Debug\QtQuick",
        "$BuildDir\Debug\platforms",
        "$BuildDir\Debug\styles",
        "$BuildDir\Release\QtQuick",
        "$BuildDir\Release\platforms",
        "$BuildDir\Release\styles"
    )
    
    foreach ($dir in $qtDirs) {
        if (Test-Path $dir) {
            Write-Host "  Removing: $dir" -ForegroundColor Gray
            Remove-Item $dir -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
    Write-Host "  Done!" -ForegroundColor Green
    Write-Host "  Note: Use -FullClean for complete rebuild" -ForegroundColor Gray
}

Write-Host ""

# 4. 验证清理结果
Write-Host "[4/4] Verifying cleanup..." -ForegroundColor Yellow
$issues = @()

# 检查标记文件是否已删除
foreach ($marker in $markers) {
    if (Test-Path $marker) {
        $issues += "Marker file still exists: $marker"
    }
}

# 检查 Qt 目录是否已删除(非完全清理模式)
if (-not $FullClean) {
    foreach ($dir in $qtDirs) {
        if (Test-Path $dir) {
            $issues += "Qt directory still exists: $dir"
        }
    }
}

if ($issues.Count -eq 0) {
    Write-Host "  All checks passed!" -ForegroundColor Green
} else {
    Write-Host "  Warning: Some items could not be cleaned:" -ForegroundColor Red
    foreach ($issue in $issues) {
        Write-Host "    - $issue" -ForegroundColor Red
    }
    Write-Host "  Tip: Try running with -KillProcesses or restart your computer" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Cleanup Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next Steps:" -ForegroundColor Cyan
Write-Host "  1. Open Qt Creator" -ForegroundColor White
Write-Host "  2. Click 'Build' (or press Ctrl+B)" -ForegroundColor White
Write-Host "  3. The project should build successfully" -ForegroundColor White
Write-Host ""

# 提供使用示例
Write-Host "Usage Examples:" -ForegroundColor Cyan
Write-Host "  .\fix_build_issue.ps1                    # Basic cleanup" -ForegroundColor Gray
Write-Host "  .\fix_build_issue.ps1 -KillProcesses     # Kill processes + cleanup" -ForegroundColor Gray
Write-Host "  .\fix_build_issue.ps1 -FullClean         # Complete rebuild" -ForegroundColor Gray
Write-Host "  .\fix_build_issue.ps1 -KillProcesses -FullClean  # Nuclear option" -ForegroundColor Gray
Write-Host ""
