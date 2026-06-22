# Anubis firmware build & flash scripts

PowerShell helpers for building and uploading the **Hosyond transmitter**
firmware ([../Hosyond_apr24b/](../Hosyond_apr24b/)) to a Hosyond 2.8" ESP32-S3
(N16R8) board with `arduino-cli` + `esptool`.

## One-time setup (per machine)

```powershell
scripts\setup-libraries.ps1
```

Installs the ESP32 core and the three required libraries, then copies the repo's
[../User_Setup.h](../User_Setup.h) into the TFT_eSPI library.

| Dependency | Source | Provides |
|---|---|---|
| `esp32:esp32` core | Boards Manager | ESP32-S3 toolchain |
| `TFT_eSPI` | registry | ILI9341 display driver (configured via `User_Setup.h`) |
| `RAK14014-FT6336U` | registry | FT6336U capacitive touch |
| `audio-driver` | Git ([pschatzmann](https://github.com/pschatzmann/arduino-audio-driver)) | `AudioBoard.h`, `AudioDriverES8311` |

> TFT_eSPI is configured by a header **inside the library**, not per-sketch.
> `setup-libraries.ps1` overwrites `libraries/TFT_eSPI/User_Setup.h` with the
> repo copy (backing up the original to `User_Setup.h.orig`). Re-run it if you
> ever update the library or change pin config.

## Everyday use

| Task | Command |
|---|---|
| Compile only | `scripts\build.ps1` |
| Flash prebuilt image (no recompile) | `scripts\flash.ps1` |
| Compile **and** upload (normal dev loop) | `scripts\build-and-flash.ps1` |

All flash/upload scripts auto-detect the ESP32 COM port. Override with
`-Port COM7` if detection picks the wrong device.

## Board / build settings (in `_common.ps1`)

- **FQBN:** `esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB`
- **Sketch:** `Hosyond_apr24b/`
- **Flash mode:** dio @ 80 MHz, 16 MB
- Merged image written at `0x0`; serial monitor baud `115200`.

## Download mode

If the board isn't detected: hold **BOOT**, tap **RESET**, release BOOT to force
ROM download mode, then retry. After flashing, the USB-CDC port may re-enumerate
(possibly to a new COM number) a few seconds after reset.
