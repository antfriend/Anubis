# Q1 / BQ28Z610 Power-Path Review — First Pass

Reviewed: 2026-07-09 · Sources: TI BQ28Z610 datasheet SLUSAS3D (rev. June 2021), CSD83325L datasheet (in repo), board netlist (`daughterboard.net`), `power_board_design.md`

**Verdict: the power-path release gate was justified — as wired, the battery protection is non-functional and the protection FETs would be gate-overstressed. Do not build or connect a pack until the rework below is done.** The BQ28Z610 is a *high-side* FET-drive device (confirmed in the datasheet block diagram, §8.3.13, and §7.25), but the board implements Q1 as a *low-side* switch pair with the sense resistor wired in parallel with it.

## What checks out

- **U2 (BQ28Z610, SON-12 DRZ) pin mapping is correct** — all 12 pins plus the PWPD pad (→ VSS) match datasheet Table 6-1 exactly.
- **SRP/SRN input filter** (R47/R48 100 Ω + C31 0.1 µF) matches the datasheet recommendation.
- **PBI capacitor** C30 = 2.2 µF to VSS matches the CPBI spec.
- **Cell-sense series resistors** R14/R15 = 470 Ω are within the recommended 100 Ω–1 kΩ balancing range; VC2←PACK_P (top cell), VC1←CELL_MID, VSS←BATT_RAW_N are correct.
- **A common-drain dual NFET is the right device structure** for the back-to-back pair (both the original CSD83325L and the PT8810 substitute are common-drain).

## Findings

**F1 — FETs are on the wrong side of the pack (blocker).**
The BQ28Z610 drives *high-side* N-channel FETs: §7.25 specifies the on-state gate voltage V(FETON) as **8.75–10.25 V above VC2** (the top of the battery stack), and §8.3.13 says that when drive is disabled, "an internal circuit discharges CHG to VC2 and DSG to PACK" — i.e., gate pulled to the *source of a high-side FET*. The board instead places Q1 low-side: S1 → BATT_RAW_N, S2 → GND, gates from DSG/CHG via R21/R22. Consequences with this wiring:
- **Off never happens:** "off" leaves the gates at VC2/PACK potential (≈6–8.4 V for 2S), which is far above the FETs' ~1 V threshold when their sources sit at pack-negative. The protection FETs can never be turned off.
- **On destroys the gates:** "on" drives the gates to VC2 + ~9.5 V ≈ up to ~18 V relative to a low-side source — beyond any 8205-class or CSD83325L (±10 V) gate rating.

**F2 — Sense resistor is in parallel with the FETs, not in series (blocker).**
R46 (2 mΩ) connects BATT_RAW_N ↔ GND, and Q1 (S1…S2) *also* connects BATT_RAW_N ↔ GND. In the TI topology the sense resistor is in series in the PACK− line. As wired: even if the FETs could open, current continues through R46 (protection cannot interrupt anything), and when the FETs conduct they shunt the sense resistor, corrupting coulomb counting.

**F3 — Charger bypasses the protection FETs (blocker).**
BQ25887 BAT1/BAT2/SNS1/SNS2 (U1 pins 13–16) tie directly to PACK_P (cell positive). In the TI application, the charger sits on the *system side* of the FET pair so the CHG FET can interrupt charging on a fault (SOV, overcurrent-in-charge). As wired, charge current never passes through Q1 regardless of which side Q1 is on.

**F4 — USB-C ground pin A12 is tied to the FET midpoint (blocker, likely a net-label slip).**
J1.A12 (a GND pin of the GCT USB4125) is on net Q1_DRAIN_COMMON; J1.B12 and the shield are on GND. All ground pins are commoned inside any attached plug/cable, so plugging in USB-C shorts the FET common-drain node to GND. A12 must be on GND.

**F5 — C10 references the FET midpoint.**
The lower-cell filter cap C10 (100 nF) connects CELL_MID ↔ Q1_DRAIN_COMMON; it should reference BATT_RAW_N.

**F6 — SRP/SRN orientation is probably swapped (verify).**
Datasheet pin table: "SRP is the top of the sense resistor" (battery side in Figure 9-1). The board has SRN←BATT_RAW_N and SRP←GND. A swap inverts the charge/discharge sign convention and misdirects the directional protections (SCC vs. SCD1/SCD2). Check against Figure 9-1 during rework.

**F7 — Reference gate network is missing.**
Figure 9-1 includes 10 MΩ gate-bleed resistors (PACK↔DSG and VC2↔CHG — also the load condition under which the §7.25 drive specs are defined) and zener clamps. The board has only the 100 Ω series resistors R21/R22.

**F8 — FET part selection needs closing (open item).**
- **PT8810** (PUOLOP, LCSC C3019811): confirmed 20 V dual N-channel *common-drain*, 22 mΩ @ 4.5 V — electrically the right class, but the **pinout could not be retrieved online** in this pass. The schematic symbol assumes 1=S1, 2=G1, 3=S2, 4=G2, 5/6=D1, 7/8=D2, while the common 8205A TSSOP-8 convention is 1=G1, 2=S1 — a gate/source swap risk. Pull the PDF from the LCSC product page (C3019811) and verify before fab.
- **Gate rating margin:** V(FETON) can reach 10.25 V. The original CSD83325L is only ±10 V VGS absolute max — marginal even before the substitution. Whichever part is used should have **VGS(max) ≥ 12 V** (and VDS ≥ 12 V is ample for 2S). Verify PT8810's actual VGS rating; if it is ±8–10 V like most 8205-class parts, choose a different FET.

## Proposed rework (net level)

1. **Move Q1 high-side:** PACK_P → [CHG FET: source at PACK_P, gate ← R22 ← CHG] → common drain → [DSG FET: gate ← R21 ← DSG, source at new SYS+ net] → SYS+ → F4 → BAT_SYS. (CHG FET source faces the battery because CHG-off discharges to VC2; DSG FET source faces the system because DSG-off discharges to PACK.)
2. **Move the charger to the system side:** U1 BAT1/BAT2/SNS1/SNS2 → SYS+.
3. **PACK pin sense:** R16 (100 Ω to U2.8) taps SYS+ directly (pre-fuse), so PACK sensing reflects the system node and the DSG-off gate discharge references the right potential.
4. **Negative path:** BATT_RAW_N —R46— GND becomes the *only* negative link (falls out of step 1). C10 → CELL_MID↔BATT_RAW_N. J1.A12 → GND.
5. **SRP/SRN:** orient per Figure 9-1 (SRP to the battery side, pending figure check).
6. **Gate network:** add 10 MΩ bleeds (SYS+↔DSG gate, PACK_P↔CHG gate) and consider the Figure 9-1 zener clamps.
7. **Close F8:** verify PT8810 pinout + VGS(max) from the LCSC datasheet, or select a ≥±12 V VGS common-drain dual.
8. **After rework:** update `power_board_design.md`, re-run `scripts\pcb-check.ps1`, and bench-validate protection trips (BQStudio data-flash configuration, which the design doc already notes is required) before trusting a live pack.

A side benefit of the correct high-side topology: BATT_RAW_N and GND stay within millivolts of each other (joined by the 2 mΩ sense resistor), so the gauge's I2C — referenced to its VSS — remains valid to the MCU even during a protection fault, which is exactly why TI uses high-side drive here.
