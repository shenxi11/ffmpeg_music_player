# 强制清理 Qt 部署文件
param(
    [string]$TargetDir
)

$ErrorActionPreference = "SilentlyContinue"

# 要清理的目录列表
$dirsToClean = @(
    "Qt",
    "QtQml",
    "QtQuick",
    "QtQuick.2",
    "QtGraphicalEffects",
    "QtMultimedia"
)

foreach ($dir in $dirsToClean) {
    $fullPath = Join-Path $TargetDir $dir
    if (Test-Path $fullPath) {
        Write-Host "Cleaning: $fullPath"
        try {
            # 递归移除只读属性
            Get-ChildItem -Path $fullPath -Recurse -Force -ErrorAction SilentlyContinue | ForEach-Object {
                try {
                    $_.Attributes = 'Normal'
                } catch {}
            }
            # 强制删除
            Remove-Item -Path $fullPath -Recurse -Force -ErrorAction SilentlyContinue
        } catch {
            # 忽略错误
        }
    }
}

exit 0
