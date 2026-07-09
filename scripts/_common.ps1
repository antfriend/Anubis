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

# KiCad project for the v2 daughterboard (PCB checks / fab exports).
$script:PcbProjectDir = Join-Path $RepoRoot "Daughterboard"

# Locate kicad-cli.exe: PATH first, then versioned Program Files installs (newest wins).
function Resolve-KicadCli {
    $cmd = Get-Command kicad-cli -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $roots = @("$env:LOCALAPPDATA\Programs\KiCad", "$env:ProgramFiles\KiCad", "${env:ProgramFiles(x86)}\KiCad")
    foreach ($root in $roots) {
        if (-not (Test-Path $root)) { continue }
        $hit = Get-ChildItem $root -Directory | Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName "bin\kicad-cli.exe" } |
            Where-Object { Test-Path $_ } | Select-Object -First 1
        if ($hit) { return $hit }
    }
    return $null
}

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
