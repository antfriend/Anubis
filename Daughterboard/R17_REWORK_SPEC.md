# r17 Rework Spec — three faults found on the fabricated r16 board

Written 2026-07-29. Supersedes the r16 release for `Daughterboard/`.

Schematic, netlist, BOM CSVs, design doc and topology verifier are **already updated** in this
commit. What remains is PCB placement and routing, which is done by hand in the KiCad GUI.

Verify at any point with:

```
%LOCALAPPDATA%\Programs\KiCad\10.0\bin\python.exe verify_r17_power_topology.py
```

It currently reports `FAIL` with 28 items — that list *is* the rework checklist. When it prints
`PASS`, the board matches the corrected schematic.

---

## Fault 1 — J7 battery connector polarity (placement fix)

**Symptom:** pack plugs in reverse-polarity.

**Cause:** `J7` (`JST_XH_S3B-XH-A_1x03_P2.50mm_Horizontal`) sits on the **front** edge at
`(70.6, 16.5) rot 180`, which puts pin 1 (`BATT_RAW_N`) at X=70.6 and pin 3 (`BATT_RAW_P`) at
X=65.6 — the opposite of the pack lead's handedness.

**Fix — move it to the rear edge and rotate 180°.** Moving the connector reverses the pin order
along X, which is what corrects the polarity. Pin 2 (`CELL_MID`) is centred and unaffected.

| | r16 | r17 |
|---|---|---|
| origin | `(70.6, 16.5)` rot 180 | **`(65.6, 84.11)` rot 0** |
| pin 1 `BATT_RAW_N` | X = 70.60 | **X = 65.60** |
| pin 2 `CELL_MID` | X = 68.10 | X = 68.10 |
| pin 3 `BATT_RAW_P` | X = 65.60 | **X = 70.60** |

`(65.6, 84.11)` mirrors the r16 geometry exactly: same three pad X coordinates, and the same
3.19 mm body overhang past the board edge (courtyard Y 81.29–93.80 against the rear edge at
Y = 90.61, matching the front-edge case of 6.80–19.31 against Y = 10.00). The whole rear strip
above Y = 76 is empty, so there is nothing to displace.

> **Do not also swap the pin 1 / pin 3 nets.** The netlist assignment
> (`1 = BATT_RAW_N`, `2 = CELL_MID`, `3 = BATT_RAW_P`) is deliberately unchanged. The physical
> flip *is* the fix; doing both cancels out and reintroduces the fault.

**Routing:** `BATT_RAW_P` (J7.3 → F4.2) and `BATT_RAW_N` (J7.1 → R46/R48/U2 VSS) now run from the
rear edge instead of the front. Keep the r16 rule: **1.0 mm trace throughout with 1.0/0.5 mm power
vias** on `BATT_RAW_P`; narrow branches are allowed only for voltage-sense and test points.
`CELL_MID` (J7.2 → BT2.2 / U1.9 / R15) is a sense-level net.

---

## Fault 2 — D2 / D3 status LEDs reversed (net fix, already in the schematic)

**Symptom:** neither status LED lights.

**Cause:** the schematic drives both LEDs from a generic `Daughterboard:TWO_PIN` symbol and put
the anode net on **pad 1** — but KiCad's `LED_SMD:LED_0603_1608Metric` footprint has **pad 1 as the
CATHODE** (confirmed in the footprint: the F.SilkS polarity bar and the F.Fab cathode bar are both
on the pad-1 side, at local X = −0.7875). Both LEDs were therefore reverse-biased and could never
conduct.

**Fix (already applied to `generate_pinlevel_schematic.py` and `daughterboard.kicad_sch`):**

| ref | r16 pad 1 | r16 pad 2 | **r17 pad 1 (K)** | **r17 pad 2 (A)** |
|---|---|---|---|---|
| D2 (green, CHG) | `LED_CHG_A` | `CHG_STAT` | **`CHG_STAT`** | **`LED_CHG_A`** |
| D3 (blue, PGOOD) | `LED_PGOOD_A` | `PG_STAT` | **`PG_STAT`** | **`LED_PGOOD_A`** |

Current path is now correct: `REGN` → R6/R7 → anode (pad 2) → cathode (pad 1) → U1 `STAT` / `PG`
open-drain pin, which pulls low to light the LED.

**Routing:** D2 `(19.81, 75.30)` and D3 `(24.11, 75.40)` do not move — only the two short local
traces at each part swap ends. Signal-level, no width requirement.

---

## Fault 3 — Q1 / Q2 driven incorrectly by U2 (the blocker)

**Symptom:** the BQ28Z610 cannot command the protection FETs.

**Cause: r14–r16 wired the pair common-SOURCE. The BQ28Z610 requires common-DRAIN.**

The device's two gate drivers are referenced to two *different* nodes. TI SLUSAS3D §8.3.13: when
drive is disabled, "an internal circuit discharges **CHG to VC2** and **DSG to PACK**". Two
different off-state gate references means the two FETs must have two different sources — which
forces a common-drain pair with each source facing its own outer terminal. §7.25 then specifies
on-state `V(FETON)` as 8.75–10.25 V **above VC2**, which only yields a sane Vgs if each source sits
at ≈VC2.

On the r16 board both sources were tied together at `FET_SRC_COMMON`, and that node is **undriven** —
both body diodes point outward from it, so nothing defines its potential. Consequences:

- **Off never happens.** "Off" parks `CHG` at VC2 (≈8.4 V) while the source floats near 0 V, so
  Vgs stays far above threshold.
- **On is not reliable either.** The only DC paths to the shared node are the two 10 MΩ bleeds,
  which drag it toward whatever the gates are doing, so Vgs collapses toward 0.

Either way the protection is non-functional. This is the same class of error flagged as F1 in
`q1_power_path_review.md`; the r14 FET swap fixed the *part* but re-introduced the orientation
error, and `R16_REVIEW_NOTES.txt` then codified "common-source high-side topology" as if intended.

### Corrected topology

```
FET_BATT_P ──[ Q1  S..D ]── FET_MID_COMMON ──[ Q2  D..S ]── BAT_SYS
(raw cells, post-F4)         (common drain)                 (protected system,
                                                             = U2 PACK sense via R16)

Q1 = CHG FET   source FET_BATT_P (≈VC2)   gate CO_GATE ← R22 ← U2.9 CHG
Q2 = DSG FET   source BAT_SYS   (≈PACK)   gate DO_GATE ← R21 ← U2.7 DSG
```

Body-diode check: Q1's diode conducts `FET_BATT_P → MID`, Q2's conducts `BAT_SYS → MID`. Both point
inward, so discharge (battery → system) requires Q2 on and charge (system → battery) requires Q1 on —
each FET gates exactly the direction its name implies.

### Pad-level net changes

| ref | pads | r16 net | **r17 net** |
|---|---|---|---|
| Q1 | 1, 2, 3 (S) | `FET_SRC_COMMON` | **`FET_BATT_P`** |
| Q1 | 5, 6, 7, 8 (D) | `BAT_SYS` | **`FET_MID_COMMON`** |
| Q1 | 4 (G) | `CO_GATE` | `CO_GATE` (unchanged) |
| Q2 | 1, 2, 3 (S) | `FET_SRC_COMMON` | **`BAT_SYS`** |
| Q2 | 5, 6, 7, 8 (D) | `FET_BATT_P` | **`FET_MID_COMMON`** |
| Q2 | 4 (G) | `DO_GATE` | `DO_GATE` (unchanged) |
| R50 | 2 | `FET_SRC_COMMON` | **`FET_BATT_P`** (Q1's own source) |
| R49 | 2 | `FET_SRC_COMMON` | **`BAT_SYS`** (Q2's own source) |
| C32 | 1 / 2 | `BAT_SYS` / `FET_SRC_COMMON` | **`FET_BATT_P` / `FET_MID_COMMON`** |
| C33 | 1 / 2 | `FET_BATT_P` / `FET_SRC_COMMON` | **`BAT_SYS` / `FET_MID_COMMON`** |

The 10 MΩ bleeds splitting to two different nodes is not cosmetic — it is what gives each FET a
defined off state, and it is the load condition under which TI specifies the §7.25 drive levels.

`R16` (`BAT_SYS` → `BQ_PACK`) is unchanged and is now correct by construction: U2's `PACK` pin
senses `BAT_SYS`, which is exactly the DSG FET's source, so the DSG-off gate discharge references
the right potential.

### Placement — nothing needs to move

This is the convenient part. The existing placement already suits the corrected topology, because
each part stays adjacent to the FET it belongs to:

| ref | position | role after fix |
|---|---|---|
| F4 | `(20.80, 47.80)` | feeds `FET_BATT_P` |
| Q1 | `(32.70, 48.51)` rot 0 | ~12 mm from F4 — battery-side source ✔ |
| R50 | `(28.10, 48.62)` rot 90 | beside Q1, gate bleed to `FET_BATT_P` ✔ |
| C32 | `(38.50, 48.58)` rot 90 | across Q1 ✔ |
| Q2 | `(17.30, 29.50)` rot 180 | beside U1 `(30, 34)` — system-side source ✔ |
| R49 | `(12.40, 29.43)` rot 90 | beside Q2, gate bleed to `BAT_SYS` ✔ |
| C33 | `(22.80, 29.88)` rot 90 | across Q2 ✔ |

Power flow is actually cleaner than r16: battery → F4 → Q1 → mid → Q2 → `BAT_SYS` → U1/U3/U4,
instead of r16's 18 mm out-and-back on `FET_BATT_P`.

### Routing

Rip up and re-route `FET_BATT_P`, `BAT_SYS`, and the new `FET_MID_COMMON` around Q1/Q2. All three
are full pack-current nets — apply the r16 standard: **1.0 mm throughout, 1.0/0.5 mm power vias**.

`FET_MID_COMMON` is new and carries full current across roughly 24 mm between the two drain
islands `(32.70, 48.51)` → `(17.30, 29.50)`; give it the same treatment as `FET_BATT_P`, and use the
drain clip islands as copper pours rather than a single trace where you can.

---

## Verification before releasing r17

1. `verify_r17_power_topology.py` prints **PASS**.
2. Schematic ERC clean. (Headless `kicad-cli` reports ~157 `footprint_link_issues` warnings because
   the global footprint library table isn't visible to it — those are environment noise, not design
   violations. Run ERC in the GUI for a clean read.)
3. PCB DRC: 0 errors, 0 unconnected. Note the long-standing 6 U1/BQ25887 clearance items
   (~0.20–0.225 mm against the 0.25 mm project rule, within JLC capability) — still non-blocking.
4. `scripts\pcb-release.ps1 -Rev r17`, then confirm all 6 gates pass.
5. Bench-validate the protection trips via BQStudio data-flash configuration before trusting a
   live pack — this gate was never closed on r14/r15/r16 and would have caught Fault 3.

---

## Can the existing r16 boards be salvaged?

- **J7 polarity — yes, no board change.** Re-pin the pack lead (swap the crimps in positions 1 and
  3) or make a reversing pigtail. Label it clearly; it will not be interchangeable with r17 boards.
- **D2 / D3 — yes, trivial.** Install the 0603 LEDs rotated 180° from the silkscreen.
- **Q1 / Q2 — not practically.** The fix needs the shared source node cut and both FETs re-oriented
  under VSON-CLIP packages. For bench bringup of the *rest* of the board you can bypass the
  protection stage entirely by strapping `FET_BATT_P` to `BAT_SYS` (jumper F4.1 to the Q1 drain
  island) and leaving Q1/Q2 unpopulated.

> Bypassing the FETs removes all pack overcurrent, overvoltage and undervoltage protection. Do that
> only on the bench, and only with a current-limited supply or a pack that has its own protection
> board. Do not leave a bypassed board connected to a bare lithium pack unattended.
