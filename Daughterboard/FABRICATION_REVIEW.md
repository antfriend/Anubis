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

1. **r12 is stale.** The PCB was edited *after* the r12 export: package exported 2026-07-08 03:45; commit `1a8d2fb` changed `daughterboard.kicad_pcb` (+291/−178 lines) at 06:22 the same morning. The committed Gerbers do not reflect the current board. Any order must come from a fresh export (r13). *Superseded by design: run `scripts\pcb-release.ps1 -Rev r13` once items 3–4 are settled.*
2. ~~**6 DRC clearance errors around U1 (BQ25887 charger).**~~ **Resolved 2026-07-09:** the violations were the VQFN-24 footprint's intrinsic 0.20–0.225 mm pad-to-exposed-pad spacing against the project's 0.25 mm rule (JLCPCB's 4-layer capability is ~0.127 mm, so no fab risk). `daughterboard.kicad_dru` now scopes a deliberate 0.19 mm minimum to items touching U1; DRC + schematic parity passes clean.
3. **Open power-path release gate — first-pass review done 2026-07-09, rework required.** See `q1_power_path_review.md`: the BQ28Z610 is a high-side FET-drive device but the board implements Q1 low-side with the sense resistor in parallel; the charger bypasses the FETs; USB-C pin A12 is mis-netted to the FET midpoint. Protection is non-functional as wired. Net-level rework plan is in that document; PT8810 pinout/VGS verification remains open.
4. **No mounting holes.** 0 NPTH holes, largest PTH is 1.02 mm, and the Edge.Cuts outline contains only corner fillets and the notch — nothing a standoff screw can pass through. The README says the Hosyond board sits above this board on standoffs; confirm the mechanical plan (shell captive mounting?) or add mounting holes before r13.

## Secondary items (cheap to fix in the same spin)

5. ~~**Board metadata.**~~ **Resolved 2026-07-09:** title block added to the board (rev "r13", dated) and stackup copper finish set to HAL lead-free; both verified to flow into the exported `.gbrjob`.
6. **BOM open verifications** (flagged in the BOM's own AssemblyNote column): L1 body vs `L_APV_ANR4020` footprint; L3 ANR5040 dimensions; U5/U6 ADS1115IDGSR are VSSOP-10 at JLC vs the TSSOP-10-class KiCad footprint (same 3×3 mm 0.5 mm-pitch class, but verify).
7. **Silkscreen warnings** (32 of the 38 DRC findings): text heights 0.65–0.7 mm vs the 0.8 mm project rule, some overlapping reference fields, and silk clipped by solder mask. Cosmetic — JLCPCB will print them fine — but small text may be illegible.
8. ~~**Stale BOM ref C8.**~~ **Resolved 2026-07-09:** removed from the r12 JLC BOM draft (the carry-forward source); BOM/CPL gate now reconciles 108 = 108.

## Settled design intent (context for reviewers)

- ADS1115 ALERT/RDY pins (U5/U6 pin 2) are **deliberate no-connects** — firmware polls over I2C at 0x48/0x49.
- Pololu #2808 ON/OFF pads are **deliberately unused** — rail control is via CTRL nets (`CTRL_5V_SW` ← GPIO2, `CTRL_3V3_SW` ← GPIO3); each module's own pushbutton remains usable.
- GPIO14/GPIO21 form the two-pin deep-sleep/wake button header.

## Reusable tests (implemented 2026-07-09)

Both scripts exist and are verified against this project — see `scripts/README.md` for usage. KiCad 10.0.4 is installed on the dev machine (`winget install KiCad.KiCad`; found via `Resolve-KicadCli` in `scripts/_common.ps1`).

- **`scripts/pcb-check.ps1`** — six release gates: ERC; DRC + schematic parity; Gerber/drill freshness (re-export and diff, ignoring timestamp/version headers — this gate confirmed the r12 staleness: 7 layers differ from the current board, while edge cuts, drill, silks, and bottom paste are unchanged); package integrity (all layers present, upload zip matches loose files); fab metadata (revision/finish set); BOM/CPL consistency. Exits nonzero on any failure.
- **`scripts/pcb-release.ps1 -Rev r13`** — builds a complete `fabrication/jlcpcb_<date>_<rev>/` package (gerbers, drill + map, ERC/DRC reports, position file, JLC CPLs, JLC BOM carried forward with a designator reconcile report), zips it, and gates it with `pcb-check.ps1`. It refuses to bless a package that fails any gate.

Running the gates against r12 also surfaced one BOM data issue not in the list above: **C8 is in the JLC BOM (100 nF row, qty 6) but no longer exists on the board** — drop it from the BOM row (and quantity) when cutting r13.

---

*The earlier review of the superseded v1 prototype board lives at `Hosyond_apr24b/Daughterboard/FABRICATION_REVIEW.md`.*
