param(
    [string]$LlamaServerPath = "llama-server",
    [string]$ModelPath = "E:\models\llm\Qwen2.5-3B-Instruct\qwen2.5-3b-instruct-q4_k_m.gguf",
    [string]$BindHost = "127.0.0.1",
    [int]$Port = 8081,
    [int]$CtxSize = 16384,
    [int]$Parallel = 1
)

$ErrorActionPreference = "Stop"

function Resolve-LlamaServerPath {
    param([string]$InputPath)

    if ([string]::IsNullOrWhiteSpace($InputPath)) {
        throw "llama-server path cannot be empty."
    }

    if (Test-Path $InputPath) {
        return (Resolve-Path $InputPath).Path
    }

    $command = Get-Command $InputPath -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Path
    }

    $wingetPackageRoot = Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages'
    if (Test-Path $wingetPackageRoot) {
        $wingetBinary = Get-ChildItem -Path $wingetPackageRoot -Recurse -Filter 'llama-server.exe' -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($wingetBinary) {
            return $wingetBinary.FullName
        }
    }

    throw "llama-server was not found. Install llama.cpp first."
}

if (-not (Test-Path $ModelPath)) {
    throw "Model file was not found: $ModelPath"
}

$resolvedServer = Resolve-LlamaServerPath -InputPath $LlamaServerPath

Write-Host 'Starting local model server...' -ForegroundColor Cyan
Write-Host "llama-server: $resolvedServer"
Write-Host "model:        $ModelPath"
Write-Host "endpoint:     http://$BindHost`:$Port/v1"
Write-Host "ctx-size:     $CtxSize"
Write-Host "parallel:     $Parallel"

& $resolvedServer `
    -m $ModelPath `
    --host $BindHost `
    --port $Port `
    --ctx-size $CtxSize `
    --parallel $Parallel
