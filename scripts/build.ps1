<#
  build.ps1 — compile the Hosyond firmware from source (no upload).
  Outputs binaries to build\Hosyond_apr24b\ (including the merged image flash.ps1 uses).

  Usage:  scripts\build.ps1
#>

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\_common.ps1"

Write-Host "==> Compiling $SketchDir" -ForegroundColor Cyan
Write-Host "    FQBN: $Fqbn" -ForegroundColor DarkGray

arduino-cli compile `
    --fqbn $Fqbn `
    --output-dir (Join-Path $RepoRoot "build\Hosyond_apr24b") `
    $SketchDir

if ($LASTEXITCODE -ne 0) { throw "Compile failed (exit $LASTEXITCODE)." }
Write-Host "Build complete -> build\Hosyond_apr24b\" -ForegroundColor Green
