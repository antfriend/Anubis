<#
  build-and-flash.ps1 — compile from source AND upload in one step via arduino-cli.
  Use this for the normal edit -> test loop. arduino-cli handles the multi-file
  flash (bootloader/partitions/app at their offsets) using the build output.

  Usage:  scripts\build-and-flash.ps1 [-Port COM7]
#>

param([string]$Port)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\_common.ps1"

$com = Get-Esp32Port -Port $Port

Write-Host "==> Compiling + uploading to $com" -ForegroundColor Cyan
Write-Host "    FQBN: $Fqbn" -ForegroundColor DarkGray

arduino-cli compile `
    --fqbn $Fqbn `
    --output-dir (Join-Path $RepoRoot "build\Hosyond_apr24b") `
    --upload --port $com `
    $SketchDir

if ($LASTEXITCODE -ne 0) { throw "Build/upload failed (exit $LASTEXITCODE)." }
Write-Host "Done. Firmware compiled and uploaded to $com." -ForegroundColor Green
