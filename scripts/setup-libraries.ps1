<#
  setup-libraries.ps1 — one-time toolchain setup for the Anubis Hosyond firmware.

  Installs the ESP32 board core + the three libraries the sketch needs, then
  copies the repo's User_Setup.h into the TFT_eSPI library so the display
  config is correct (TFT_eSPI is configured via a header inside the library,
  not per-sketch).

  Run once per machine (or after wiping the Arduino libraries folder).
#>

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot

Write-Host "==> Installing ESP32 core (esp32:esp32)..." -ForegroundColor Cyan
arduino-cli core update-index
arduino-cli core install esp32:esp32

Write-Host "==> Installing registry libraries..." -ForegroundColor Cyan
arduino-cli lib install "TFT_eSPI" "RAK14014-FT6336U"

Write-Host "==> Installing audio-driver from Git (provides AudioBoard.h / AudioDriverES8311)..." -ForegroundColor Cyan
arduino-cli config set library.enable_unsafe_install true
arduino-cli lib install --git-url https://github.com/pschatzmann/arduino-audio-driver.git

Write-Host "==> Configuring TFT_eSPI with repo User_Setup.h..." -ForegroundColor Cyan
$userDir = (arduino-cli config get directories.user).Trim()
$tft = Join-Path $userDir "libraries\TFT_eSPI"
if (-not (Test-Path "$tft\User_Setup.h.orig")) {
    Copy-Item "$tft\User_Setup.h" "$tft\User_Setup.h.orig"
    Write-Host "    backed up library default -> User_Setup.h.orig"
}
Copy-Item "$RepoRoot\User_Setup.h" "$tft\User_Setup.h" -Force
Write-Host "    repo User_Setup.h installed into TFT_eSPI"

Write-Host "`nSetup complete. You can now run scripts\build.ps1 or scripts\build-and-flash.ps1" -ForegroundColor Green
