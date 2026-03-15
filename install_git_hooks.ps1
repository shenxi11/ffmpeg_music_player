$ErrorActionPreference = "Stop"

$repoRoot = git rev-parse --show-toplevel
if (-not $repoRoot) {
    throw "Not inside a git repository."
}

Set-Location $repoRoot
git config core.hooksPath .githooks

Write-Host "Git hooks installed."
Write-Host "core.hooksPath -> .githooks"
Write-Host "pre-commit hook -> .githooks/pre-commit"
Write-Host "pre-push hook -> .githooks/pre-push"
