# Anubis RC Controller — Implementation Status

_Status snapshot generated 2026-06-27 from a read of the firmware source. "Verified"
here means **implemented and wired end-to-end in the code** (init → input → processing →
output), not that every item has been re-confirmed on the bench. The **Bench-tested?**
column tracks physical hardware confirmation — flip ☐ → ✅ as you validate each item on
real hardware (— = not applicable). Update this file as features land or are tested._

Primary sources reviewed:
- Transmitter: [Hosyond_apr24b/Hosyond_apr24b.ino](Hosyond_apr24b/Hosyond_apr24b.ino) (~18k lines)
- Receiver: [ESPNow_S3_Receiver/ESPNow_S3_Receiver.ino](ESPNow_S3_Receiver/ESPNow_S3_Receiver.ino)
- Hardware/pinout: [README.txt](README.txt), [User_Setup.h](User_Setup.h)
- Changelog: [Hosyond_apr24b/ChangeLog.txt](Hosyond_apr24b/ChangeLog.txt)

---

## ✅ Fully implemented & wired end-to-end

These have a complete path in code and are exercised by the main loop.

| Feature | Notes / location | Bench-tested? |
|---|---|---|
| **Display (ILI9341 240×320 SPI)** | TFT_eSPI, configured via repo [User_Setup.h](User_Setup.h); init in `setup()` ([Hosyond_apr24b.ino:7354](Hosyond_apr24b/Hosyond_apr24b.ino#L7354)). Backlight PWM dimming wired on IO45. | ☐ |
| **Capacitive touch (FT6336)** | `touchPanel.begin()` + polled each loop; full touch UI navigation. | ☐ |
| **Gimbal sticks via ADS1115 (external I²C ADC)** | `initAds1115()`; 4 axes read and normalized in `updateStickInputs()`. | ☐ |
| **ESP-NOW transmit link** | Binding, control packets, ping/latency, telemetry, unbind — `sendEspNowControlPacket()` ([Hosyond_apr24b.ino:5433](Hosyond_apr24b/Hosyond_apr24b.ino#L5433)). Paired with the receiver sketch below. | ☐ |
| **ELRS / CRSF transmit** | UART CRSF channel frames, TX power, device ping, half/full-duplex + auto module-profile detection. `sendElrsChannelFrame()` ([Hosyond_apr24b.ino:5911](Hosyond_apr24b/Hosyond_apr24b.ino#L5911)). | ☐ |
| **Channel processing pipeline** | `updateChannelOutputs()` ([Hosyond_apr24b.ino:10252](Hosyond_apr24b/Hosyond_apr24b.ino#L10252)) applies trim → expo → endpoints → **mixes** → reverse. (NOTE: mixer now *does* drive output math — the changelog's "UI only" note is stale.) | ☐ |
| **Drive types** | Tank (1-stick & 2-stick), Car, X-drone — output shaping in `buildProtocolOutputChannels()` ([Hosyond_apr24b.ino:10291](Hosyond_apr24b/Hosyond_apr24b.ino#L10291)). Saved per model. | ☐ |
| **Per-model storage (EEPROM)** | Name, reverse, trim, failsafe flags, mixes, drive type, tank mode, protocol, rates, expo, endpoints, ESC channel map, ESP-NOW output mode. Versioned schemas w/ first-boot migration in `setup()`. | ☐ |
| **Mixer editor UI** | 4 user mix slots, source/dest cycle, rate/offset (+/- and numpad), per-mix reverse-link. | ☐ |
| **Reverse / Trim / Failsafe / Endpoints / Rates / Expo screens** | All present with touch + D-pad paths. | ☐ |
| **Battery monitor** | Real ADC on IO9 w/ 200k/200k divider, `analogReadMilliVolts()` ([Hosyond_apr24b.ino:10369](Hosyond_apr24b/Hosyond_apr24b.ino#L10361)); "No Batt" / X-icon when absent. | ☐ |
| **Audio (ES8311 codec, I²S)** | `initAudio()` detects codec; UI click sounds + volume popup/mute. Falls back gracefully if codec not detected. | ☐ |
| **D-pad input** | 5-way nav via **PCF8575** I²C expander (`readPcf8575()`), wired into all major screens. *(See discrepancy note re: MCP23017.)* | ☐ |
| **OTA update service** | SoftAP + STA HTTP upload server, `updateOtaService()` ([Hosyond_apr24b.ino:4398](Hosyond_apr24b/Hosyond_apr24b.ino#L4398)). | ☐ |
| **Power button / soft power / deep sleep** | Boot hold, auto deep-sleep on inactivity, display wake on stick/touch. | ☐ |
| **Status RGB LED** | IO42 single-wire LED encodes fault/status codes (no-ADS, no-codec, etc.). | ☐ |
| **Startup throttle safety** | Blocks until throttle is safe, with tap-bypass. | ☐ |
| **Misc UI** | Model name keyboard (shift), main-screen swipe-to-timer, "Space Game" easter egg. | ☐ |

### Receiver ([ESPNow_S3_Receiver](ESPNow_S3_Receiver/ESPNow_S3_Receiver.ino))
| Feature | Notes | Bench-tested? |
|---|---|---|
| **ESP-NOW receive + bind** | Matches TX packet protocol; peer add, telemetry reply. | ☐ |
| **PWM servo output** | Up to `RC_OUTPUT_COUNT` channels, 50 Hz, 1000–2000 µs. | ☐ |
| **iBus serial output** | 14-channel iBus frames on D5, selectable via `ESPNOW_FLAG_OUTPUT_IBUS`. | ☐ |
| **Drive-type/tank-mode awareness** | Receives and labels drive/tank mode from packet. | ☐ |

---

## 🟡 Partly wired / needs confirmation

| Feature | Status | Bench-tested? |
|---|---|---|
| **Charging detection** | Voltage reading is real; "charging" is **inferred from rising voltage**, not a charger status pin (board doesn't route one). Best-effort only — see [ChangeLog.txt:163](Hosyond_apr24b/ChangeLog.txt#L163). | ☐ |
| **ELRS RX configuration screen** | `SCREEN_ELRS_RX_CONFIG` + parameter read/write code exists (model ID, failsafe, PWM ch, TX power). Depends on live module param protocol — **bench-verify against real ELRS RX**. | ☐ |
| **ELRS bind** | `sendElrsBindCommand()` implemented; relies on module field index that varies by profile (auto-detect fallback constants present). Confirm on hardware. | ☐ |
| **ELRS module auto-profile detect** | `ELRS_MODULE_PROFILE_AUTO` default; switches half/full duplex & pins at runtime. Logic present, hardware-dependent. | ☐ |

---

## ⛔ Stubbed / disabled / dead code

| Item | Status |
|---|---|
| **Stick calibration screen** | Screen `SCREEN_STICK_CALIBRATION` and capture/sweep logic exist but are **disabled**: `STICK_CAL_SCREEN_ENABLED = false` ([Hosyond_apr24b.ino:210](Hosyond_apr24b/Hosyond_apr24b.ino#L210)). |
| **Stick calibration values** | Uses **hardcoded** min/center/max — `STICK_CAL_USE_HARDCODED_DEFAULTS = true` ([Hosyond_apr24b.ino:801](Hosyond_apr24b/Hosyond_apr24b.ino#L801)), values at [Hosyond_apr24b.ino:940](Hosyond_apr24b/Hosyond_apr24b.ino#L940). Live calibration not in use. |
| **MCP23017 driver** | `initMcp23017()` fully written but **never called** ([Hosyond_apr24b.ino:9823](Hosyond_apr24b/Hosyond_apr24b.ino#L9823)) — D-pad actually runs on PCF8575. Legacy/unused. |
| **`drawControllerPlaceholderScreen()`** | Placeholder controller-settings screen helper ([Hosyond_apr24b.ino:11916](Hosyond_apr24b/Hosyond_apr24b.ino#L11916)); some controller settings noted as hardcoded pending a user-facing menu ([Hosyond_apr24b.ino:209](Hosyond_apr24b/Hosyond_apr24b.ino#L209)). |
| **ELRS passive sniff mode** | `ELRS_PASSIVE_SNIFF_MODE = false` ([Hosyond_apr24b.ino:302](Hosyond_apr24b/Hosyond_apr24b.ino#L302)); diagnostic-only path, off by default (disables ESP-NOW when on). |

---

## ⚠️ Discrepancies worth resolving

1. **D-pad expander: README vs firmware.** Hardware list in [README.txt:23](README.txt#L23) specifies an **MCP23017**, but the active firmware reads a **PCF8575** (same 0x20–0x27 address range). The MCP23017 init exists but is never called. Confirm which chip is actually on the board and either re-enable the MCP path or correct the BOM.
2. **Stale changelog.** [ChangeLog.txt:16](Hosyond_apr24b/ChangeLog.txt#L16) says the mixer "does not yet drive any actual output math." It now does (see `updateChannelOutputs()`).

---

## Build & flash

- Toolchain/process documented in [scripts/README.md](scripts/README.md).
- FQBN: `esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB`.
- `scripts\setup-libraries.ps1` (one-time) → `scripts\build-and-flash.ps1` (dev loop).
- TFT_eSPI is configured by copying repo [User_Setup.h](User_Setup.h) **into the library**.

## Hardware peripherals — wiring status

| Peripheral | Bus / pin | Firmware status | Bench-tested? |
|---|---|---|---|
| ILI9341 display | SPI 10/11/12/13/46, BL 45 | ✅ active | ☐ |
| FT6336 touch | I²C 15/16 (rst 18, int 17) | ✅ active | ☐ |
| ADS1115 (gimbals) | I²C | ✅ active | ☐ |
| PCF8575 (D-pad) | I²C 0x20–0x27 | ✅ active | ☐ |
| MCP23017 (D-pad, per BOM) | I²C | ⛔ code present, unused | — |
| ES8311 codec + I²S audio | I²S 4/5/6/7/8, EN IO1 | ✅ active (graceful fallback) | ☐ |
| Battery ADC | IO9 (200k/200k divider) | ✅ active | ☐ |
| RGB status LED | IO42 | ✅ active | ☐ |
| ELRS module UART | Serial1 (pins 43/44, profile-dependent) | ✅ TX; RX-config 🟡 | ☐ |
| SD card | SDIO 38/39/40/41/47/48 | ⛔ not used — no SD/FS code in firmware | — |
</content>
</invoke>
