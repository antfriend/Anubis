# Anubis firmware build & flash scripts

PowerShell helpers for building and uploading the **Hosyond transmitter**
firmware ([../Hosyond_apr24b/](../Hosyond_apr24b/)) to a Hosyond 2.8" ESP32-S3
(N16R8) board with `arduino-cli` + `esptool`, plus PCB fabrication checks for
the **Daughterboard** KiCad project (see [PCB scripts](#pcb-fabrication-scripts)).

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

## PCB fabrication scripts

For the [../Daughterboard/](../Daughterboard/) KiCad project. Both need
`kicad-cli` (KiCad 10): `winget install KiCad.KiCad`. The helper in
`_common.ps1` finds it on PATH, in `%LOCALAPPDATA%\Programs\KiCad`, or in
`Program Files\KiCad`.

| Task | Command |
|---|---|
| Gate the newest `fabrication\jlcpcb_*` package | `scripts\pcb-check.ps1` |
| Gate a specific package | `scripts\pcb-check.ps1 -PackageDir <path>` |
| Static gates only (no KiCad needed) | `scripts\pcb-check.ps1 -StaticOnly` |
| Build + gate a new fab package | `scripts\pcb-release.ps1 -Rev r13` |

`pcb-check.ps1` gates: ERC, DRC + schematic parity, Gerber/drill freshness
(re-export and diff against the package, ignoring timestamp/version headers),
package integrity (all layers present, upload zip matches loose files), fab
metadata (revision and copper finish set in the board file), and BOM/CPL
consistency (every placed part has an LCSC number). Exits nonzero if any gate
fails — **run it before uploading anything to JLCPCB.**

`pcb-release.ps1 -Rev rNN` builds `Daughterboard\fabrication\jlcpcb_<date>_rNN\`
(gerbers, drill + map, ERC/DRC reports, position file, JLC CPLs, JLC BOM carried
forward from the previous package with a designator reconcile report), zips the
JLC upload archive and the whole package, then runs `pcb-check.ps1` on the
result. It refuses to bless a package that fails any gate.

Notes:

- Packages from r13 on use KiCad's native `.gbr` layer extensions (older
  packages used Protel `.gtl`/`.gbl`); JLCPCB accepts both.
- The JLC BOM is carried forward, not regenerated — new parts appear in
  `assembly\Daughterboard_bom_reconcile_report.txt` and need LCSC numbers added
  by hand. Hand-soldered parts (18650 holders, Pololu switch modules, headers)
  are expected in the "not in BOM" list.
- JLC's part rotations sometimes differ from KiCad's; always eyeball the
  component preview during JLCPCB order review.
