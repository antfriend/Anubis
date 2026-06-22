<#
  _common.ps1 — shared settings + helpers for the Anubis build/flash scripts.
  Dot-source this from other scripts:  . "$PSScriptRoot\_common.ps1"
#>

$script:RepoRoot   = Split-Path -Parent $PSScriptRoot
$script:SketchDir  = Join-Path $RepoRoot "Hosyond_apr24b"
# Hosyond 2.8" ESP32-S3 (N16R8): 16 MB flash, 3 MB app + 9 MB FAT partition scheme.
$script:Fqbn       = "esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB"
# Prebuilt output that flash.ps1 uses (self-contained 16 MB image, all offsets baked in).
$script:MergedBin  = Join-Path $RepoRoot "build\Hosyond_apr24b\Hosyond_apr24b.ino.merged.bin"

# Auto-detect the ESP32 serial port. Pass -Port to override in the calling script.
function Get-Esp32Port {
    param([string]$Port)
    if ($Port) { return $Port }
    $line = arduino-cli board list | Select-String -Pattern "esp32" | Select-Object -First 1
    if (-not $line) {
        throw "No ESP32 board detected. Plug in the Hosyond via USB-C (hold BOOT, tap RESET, release BOOT to force download mode), then retry. Override with -Port COMx."
    }
    $com = ($line.ToString() -split "\s+")[0]
    Write-Host "Detected board on $com" -ForegroundColor DarkGray
    return $com
}
