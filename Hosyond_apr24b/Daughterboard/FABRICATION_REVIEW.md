# Daughterboard Fabrication Review & Plan

Evaluated: 2026-07-09 · Project: `daughterboard/daughterboard.kicad_pcb` (KiCad 10.0.3) · Gerbers exported 2026-05-28, committed with the board in `5d1a20b`

**Bottom line:** the Gerber package is structurally sound and any budget fab (JLCPCB, PCBWay, OSH Park) will accept it, but three things should be resolved before ordering — the unrouted ALRT/ON/OFF nets, the missing revision/finish metadata, and the fact that nothing currently proves the committed Gerbers match the current board file.

## What checks out

- **Complete, consistent output set.** `GERBER/` has both copper layers, both masks, both silkscreens, edge cuts, a merged Excellon drill, and the `.gbrjob`. `Daughterboard_Gerber.zip` contents byte-match the loose files, and the Gerbers were committed in the same commit (`5d1a20b`) as the `.kicad_pcb`.
- **Clean verification reports.** DRC: 0 violations (`DRC_current_cli.rpt`). ERC: 0 errors, 38 cosmetic warnings. The existing `quote_readiness_check.txt` shows 0 schematic-vs-PCB node mismatches across 36 footprints and 50 nets.
- **Conservative geometry.** 2-layer, 1.6 mm FR4, 0.2 mm min trace, 0.15 mm clearances, 0.3 mm min via drill — nowhere near any fab's limits. Board is 163 × 42 mm with a notched outline (over the 100×100 mm cheap tier, so expect a modest price bump, but no capability issue).
- **All through-hole** (27 connectors, 2 resistors, 7 test points, 4 module sockets U1–U4), so the absent paste layers are correct and no assembly files are needed.
- **Mounting holes exist** — four 3.5 mm circles drawn on Edge.Cuts (routed cutouts), which is why the NPTH drill file is empty. Fabs handle this fine, though NPTH drills are the more conventional encoding.

## Issues found (ranked)

1. **Unrouted named nets — the only real design question.** ALRT (U1 pin 6 ↔ U2 pin 6), ON (J24 ↔ J25 pin 5), and OFF (J24 ↔ J25 pin 6) are in the netlist with zero tracks, producing 11 "unconnected pads" DRC errors. The fab won't care, but the board as built won't have those signals connected. If intentional, they should be no-connects or DRC exclusions — right now every DRC run reports errors that get ignored, which will mask a genuinely missed route someday.
2. **No staleness guarantee.** If anyone edits the `.kicad_pcb` and forgets to re-export, the zip silently goes stale. This is the highest-value reusable test to add.
3. **Ordering metadata gaps.** The job file says `Revision: "rev?"` and `Finish: "None"`. Set the title-block revision and the copper finish (e.g., HASL lead-free or ENIG) in Board Setup so the files carry what is actually ordered.
4. **Tooling gap.** `kicad-cli` is not installed on the dev machine (checked PATH, Program Files, registry) — the `*_current_cli` reports were made elsewhere. Any reusable test needs to bootstrap it.
5. **Minor hygiene.** A 2.8 MB file named just `.step` (no basename); 38 ERC unconnected-wire-endpoint warnings; CC1/CC2 single-node labels (fine if the USB-C module handles CC pulldowns itself — worth a one-time confirm).

## Plan

### Phase 1 — Toolchain (one-time)

Install KiCad 10 (`winget install KiCad.KiCad`) and add a `Resolve-KicadCli` helper to `scripts/_common.ps1` that finds `kicad-cli.exe` or fails with install instructions.

### Phase 2 — Design/metadata fixes

- Decide ALRT/ON/OFF intent: route them, or convert to no-connects/exclusions so DRC returns to a true zero. **(Open decision — needs the designer's call.)**
- Set title-block revision (e.g., "A") and stackup copper finish in Board Setup.
- Rename `.step` → `daughterboard.step`.
- Re-export Gerbers/drill/zip after the above.

### Phase 3 — Reusable tests: `scripts/pcb-check.ps1`

Following the existing `scripts/` conventions, runnable before every order:

1. **ERC gate** — `kicad-cli sch erc`, fail on errors.
2. **DRC gate** — `kicad-cli pcb drc`, fail on errors *and* unconnected pads (meaningful once Phase 2 zeroes them).
3. **Netlist parity** — export netlist from the schematic, compare node-by-node against the PCB (automates the manual `quote_readiness_check.txt`).
4. **Gerber freshness** — re-export to a temp dir, strip date-stamp lines, diff against committed `GERBER/`; fail if the board file has drifted from the shipped outputs.
5. **Package integrity** — zip contents match `GERBER/`, required layer set present, drill file non-trivial, NPTH included if it ever gains holes.
6. **Fab metadata lint** — parse the `.gbrjob`: 2 layers, expected board size, revision ≠ "rev?", finish ≠ "None".

### Phase 4 — Regeneration script: `scripts/pcb-release.ps1`

One deterministic command that exports Gerbers + drill, rebuilds the zip, and runs `pcb-check.ps1`, so outputs can never drift from the board file again.
