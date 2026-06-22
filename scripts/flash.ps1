<#
  flash.ps1 — flash the prebuilt merged firmware image to the board via esptool.
  Does NOT recompile; it writes build\Hosyond_apr24b\Hosyond_apr24b.ino.merged.bin.

  The merged image contains bootloader + partitions + boot_app0 + app at their
  correct offsets, so this is a single, self-consistent write at 0x0.

  Usage:  scripts\flash.ps1 [-Port COM7]
#>

param([string]$Port)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\_common.ps1"

if (-not (Test-Path $MergedBin)) {
    throw "Merged binary not found: $MergedBin`nRun scripts\build.ps1 first."
}

$com = Get-Esp32Port -Port $Port

Write-Host "==> Flashing $MergedBin to $com" -ForegroundColor Cyan
esptool --chip esp32s3 -p $com -b 460800 write_flash 0x0 $MergedBin
if ($LASTEXITCODE -ne 0) { throw "Flash failed (exit $LASTEXITCODE)." }

Write-Host "Flash complete. Board hard-reset into new firmware." -ForegroundColor Green
Write-Host "Note: USB-CDC may re-enumerate the COM port a few seconds after reset." -ForegroundColor DarkGray
