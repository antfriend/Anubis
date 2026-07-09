# Daughterboard Fabrication Review (v2)

Evaluated: 2026-07-09 · Project: `Daughterboard/daughterboard.kicad_pcb` (KiCad 10.0.3) · Package reviewed: `fabrication/jlcpcb_2026-07-06_r12/` (exported 2026-07-08 03:45)

**Verdict: do NOT order r12.** The package is well-constructed — complete 4-layer Gerber set with paste layers, nested JLCPCB upload zip, single-sided CPL, fully LCSC-matched BOM, and DRC/ERC reports bundled in — but it is **stale**, carries **6 real DRC clearance errors**, and the design doc's own **FET power-path release gate** is still open. A fresh r13 export after the fixes below is the path to ordering.

## Board summary

- 4-layer, 155.1 × 80.8 mm, 1.6 mm FR4, rounded-rectangle outline with a 41 mm filleted notch.
- Single-sided SMD assembly: 108 top-side placements in the CPL; bottom paste layer is empty.
- Hand-soldered exceptions (by design): 18650 holders, 2× Pololu #2808 switch modules, deep-sleep header, Hosyond wire pads.
- Drill: 231 plated holes, min 0.30 mm, plus 4× 0.6 mm slots — all within JLCPCB 4-layer capability.
- ERC: 0 errors, 0 warnings.

## Blockers (fix before ordering)

1. **r12 is stale.** The PCB was edited *after* the r12 export: package exported 2026-07-08 03:45; commit `1a8d2fb` changed `daughterboard.kicad_pcb` (+291/−178 lines) at 06:22 the same morning. The committed Gerbers do not reflect the current board. Any order must come from a fresh export (r13).
2. **6 DRC clearance errors around U1 (BQ25887 charger).** PACK_P pads 13–16 and CHG_IN pad 23 sit 0.20–0.225 mm from GND pad 25 against a 0.25 mm rule. Note: 0.20 mm is still within JLCPCB's 4-layer capability (~0.127 mm), so this violates the project's own rule, not the fab's — resolve it deliberately, either by adjusting the layout or by consciously relaxing the rule in that region. Don't ship with red errors in the report.
3. **Open power-path release gate.** `power_board_design.md` states the final charge/discharge FET topology (Q1) must be reviewed against the TI BQ28Z610 reference before fabrication, and the BOM note for Q1 (PT8810, LCSC C3019811) says "verify datasheet pinout before fab." This is the highest-consequence item on the board — a wrong FET topology can prevent charging or defeat pack protection.
4. **No mounting holes.** 0 NPTH holes, largest PTH is 1.02 mm, and the Edge.Cuts outline contains only corner fillets and the notch — nothing a standoff screw can pass through. The README says the Hosyond board sits above this board on standoffs; confirm the mechanical plan (shell captive mounting?) or add mounting holes before r13.

## Secondary items (cheap to fix in the same spin)

5. **Board metadata:** `Revision: "rev?"` and `Finish: "None"` in the `.gbrjob`. Set the title-block revision (r13) and the stackup copper finish so the files carry what is ordered.
6. **BOM open verifications** (flagged in the BOM's own AssemblyNote column): L1 body vs `L_APV_ANR4020` footprint; L3 ANR5040 dimensions; U5/U6 ADS1115IDGSR are VSSOP-10 at JLC vs the TSSOP-10-class KiCad footprint (same 3×3 mm 0.5 mm-pitch class, but verify).
7. **Silkscreen warnings** (32 of the 38 DRC findings): text heights 0.65–0.7 mm vs the 0.8 mm project rule, some overlapping reference fields, and silk clipped by solder mask. Cosmetic — JLCPCB will print them fine — but small text may be illegible.

## Settled design intent (context for reviewers)

- ADS1115 ALERT/RDY pins (U5/U6 pin 2) are **deliberate no-connects** — firmware polls over I2C at 0x48/0x49.
- Pololu #2808 ON/OFF pads are **deliberately unused** — rail control is via CTRL nets (`CTRL_5V_SW` ← GPIO2, `CTRL_3V3_SW` ← GPIO3); each module's own pushbutton remains usable.
- GPIO14/GPIO21 form the two-pin deep-sleep/wake button header.

## Reusable test plan

### Phase 1 — Toolchain (one-time)

`kicad-cli` is not installed on the dev machine (the bundled reports were generated elsewhere). Install KiCad 10 (`winget install KiCad.KiCad`) and add a `Resolve-KicadCli` helper to `scripts/_common.ps1`.

### Phase 2 — Release checks: `scripts/pcb-check.ps1`

Runnable before every order, targeting `Daughterboard/` and the newest `fabrication/jlcpcb_*` package:

1. **ERC gate** — `kicad-cli sch erc`, fail on errors.
2. **DRC gate** — `kicad-cli pcb drc`, fail on errors and unconnected pads (silk warnings reported but non-fatal).
3. **Gerber freshness** — re-export Gerbers/drill to a temp dir, strip date-stamp lines, diff against the packaged `gerbers/`; fail if the board has drifted from the package. *(Would have caught the r12 staleness.)*
4. **Package integrity** — nested upload zip matches loose files; required 4-layer set present (4× copper, 2× mask, 2× paste, 2× silk, edge cuts, drill, gbrjob).
5. **Fab metadata lint** — parse the `.gbrjob`: 4 layers, expected size, revision ≠ "rev?", finish ≠ "None".
6. **BOM/CPL consistency** — every CPL designator appears in the BOM with a non-empty LCSC part number; BOM quantities match designator counts; CPL is single-sided.
7. **Netlist parity** — schematic-vs-PCB node comparison.

### Phase 3 — Release script: `scripts/pcb-release.ps1 -Rev r13`

One deterministic command that exports Gerbers + drill + position files, rebuilds both zips into `fabrication/jlcpcb_<date>_<rev>/`, runs `pcb-check.ps1`, and refuses to package if any gate fails — so a stale or failing package can never be produced again.

---

*The earlier review of the superseded v1 prototype board lives at `Hosyond_apr24b/Daughterboard/FABRICATION_REVIEW.md`.*
