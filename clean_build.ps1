# 清理构建输出和 Qt 部署文件
param(
    [string]$BuildDir = "E:\QT_CMAKE\ffmpeg_musicer_player_build"
)

Write-Host "清理构建目录: $BuildDir" -ForegroundColor Cyan

# 移除所有文件的只读属性
Write-Host "移除只读属性..." -ForegroundColor Yellow
Get-ChildItem -Path $BuildDir -Recurse -File -ErrorAction SilentlyContinue | ForEach-Object {
    $_.IsReadOnly = $false
}

# 清理 Qt 相关目录
$qtDirs = @(
    "$BuildDir\Debug\QtQuick",
    "$BuildDir\Debug\Qt",
    "$BuildDir\Debug\platforms",
    "$BuildDir\Debug\styles",
    "$BuildDir\Debug\imageformats",
    "$BuildDir\Release\QtQuick",
    "$BuildDir\Release\Qt",
    "$BuildDir\Release\platforms",
    "$BuildDir\Release\styles",
    "$BuildDir\Release\imageformats"
)

foreach ($dir in $qtDirs) {
    if (Test-Path $dir) {
        Write-Host "删除目录: $dir" -ForegroundColor Yellow
        Remove-Item -Path $dir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Write-Host "清理完成!" -ForegroundColor Green
