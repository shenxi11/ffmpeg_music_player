param(
    [string]$BaseUrl = "http://127.0.0.1:8081/v1"
)

$ErrorActionPreference = "Stop"

$modelsUrl = $BaseUrl.TrimEnd("/") + "/models"

Write-Host "Checking local model service: $modelsUrl" -ForegroundColor Cyan

try {
    $response = Invoke-WebRequest -UseBasicParsing -Uri $modelsUrl -TimeoutSec 5
    Write-Host "Local model service is available, HTTP $($response.StatusCode)" -ForegroundColor Green
    if ($response.Content) {
        Write-Host $response.Content
    }
} catch {
    Write-Error ("Local model service is unavailable: {0}" -f $_.Exception.Message)
    exit 1
}
