"""r17 power-path topology audit.

Asserts the CORRECTED BQ28Z610 high-side protection topology on the PCB.

Background: r16 (fabricated) wired Q1/Q2 common-SOURCE with the shared node
floating between them. That is wrong for the BQ28Z610, which is a high-side
N-channel FET driver whose two gate outputs are referenced to two DIFFERENT
nodes (SLUSAS3D 8.3.13: when drive is disabled "an internal circuit discharges
CHG to VC2 and DSG to PACK"). Two different off-state gate references means the
two FETs must have two different sources, so the pair has to be common-DRAIN
with each source facing its own outer terminal:

    FET_BATT_P --[Q1 S..D]-- FET_MID_COMMON --[Q2 D..S]-- BAT_SYS
    (raw cells,             (common drain)              (protected system,
     post-F4)                                            = U2 PACK sense)

    Q1 = CHG FET, source at FET_BATT_P (~VC2), gate CO_GATE  <- R22 <- U2.CHG
    Q2 = DSG FET, source at BAT_SYS   (~PACK), gate DO_GATE  <- R21 <- U2.DSG

Each 10 M gate bleed must return to its OWN FET's source, otherwise the
off-state Vgs is not defined:

    R50: CO_GATE <-> FET_BATT_P
    R49: DO_GATE <-> BAT_SYS

Run with the KiCad Python interpreter:
    %LOCALAPPDATA%\\Programs\\KiCad\\10.0\\bin\\python.exe verify_r17_power_topology.py
"""

from __future__ import annotations

from pathlib import Path

import pcbnew


BOARD_PATH = Path("daughterboard.kicad_pcb")


EXPECTED_PAD_NETS = {
    ("J1", "A12"): "GND",
    ("J1", "B12"): "GND",
    ("J1", "SH"): "GND",
    ("U1", "13"): "BAT_SYS",
    ("U1", "14"): "BAT_SYS",
    ("U1", "15"): "BAT_SYS",
    ("U1", "16"): "BAT_SYS",
    ("U1", "9"): "CELL_MID",
    ("U1", "19"): "GND",
    ("U1", "20"): "GND",
    ("U1", "23"): "CHG_IN",
    ("U2", "1"): "BATT_RAW_N",
    ("U2", "2"): "BQ_SRN",
    ("U2", "3"): "BQ_SRP",
    ("U2", "7"): "BQ_DSG",
    ("U2", "8"): "BQ_PACK",
    ("U2", "9"): "BQ_CHG",
    ("U2", "11"): "PROT_VDD",
    ("U2", "12"): "PROT_VC",
    ("U2", "13"): "BATT_RAW_N",
    ("R46", "1"): "BATT_RAW_N",
    ("R46", "2"): "GND",
    ("R47", "1"): "GND",
    ("R47", "2"): "BQ_SRN",
    ("R48", "1"): "BATT_RAW_N",
    ("R48", "2"): "BQ_SRP",
    # U2 PACK sense must tap the DSG FET's source (protected system node).
    ("R16", "1"): "BAT_SYS",
    ("R16", "2"): "BQ_PACK",
    ("R21", "1"): "BQ_DSG",
    ("R21", "2"): "DO_GATE",
    ("R22", "1"): "BQ_CHG",
    ("R22", "2"): "CO_GATE",
    ("F3", "2"): "BATT_RAW_P",
    ("F4", "1"): "FET_BATT_P",
    ("F4", "2"): "BATT_RAW_P",
    ("J7", "1"): "BATT_RAW_N",
    ("J7", "2"): "CELL_MID",
    ("J7", "3"): "BATT_RAW_P",
    # Q1 = charge FET: source on the raw-cell side, drain on the common mid node.
    ("Q1", "1"): "FET_BATT_P",
    ("Q1", "2"): "FET_BATT_P",
    ("Q1", "3"): "FET_BATT_P",
    ("Q1", "4"): "CO_GATE",
    ("Q1", "5"): "FET_MID_COMMON",
    ("Q1", "6"): "FET_MID_COMMON",
    ("Q1", "7"): "FET_MID_COMMON",
    ("Q1", "8"): "FET_MID_COMMON",
    # Q2 = discharge FET: source on the protected system side, drain on the mid node.
    ("Q2", "1"): "BAT_SYS",
    ("Q2", "2"): "BAT_SYS",
    ("Q2", "3"): "BAT_SYS",
    ("Q2", "4"): "DO_GATE",
    ("Q2", "5"): "FET_MID_COMMON",
    ("Q2", "6"): "FET_MID_COMMON",
    ("Q2", "7"): "FET_MID_COMMON",
    ("Q2", "8"): "FET_MID_COMMON",
    # Gate bleeds must return to their own FET's source.
    ("R50", "1"): "CO_GATE",
    ("R50", "2"): "FET_BATT_P",
    ("R49", "1"): "DO_GATE",
    ("R49", "2"): "BAT_SYS",
    # Transient caps sit across each FET (source to common drain).
    ("C32", "1"): "FET_BATT_P",
    ("C32", "2"): "FET_MID_COMMON",
    ("C33", "1"): "BAT_SYS",
    ("C33", "2"): "FET_MID_COMMON",
    # Status LEDs: KiCad LED_0603_1608Metric pad 1 is the CATHODE.
    # Anode (via the REGN series resistor) must land on pad 2.
    ("D2", "1"): "CHG_STAT",
    ("D2", "2"): "LED_CHG_A",
    ("D3", "1"): "PG_STAT",
    ("D3", "2"): "LED_PGOOD_A",
    ("R6", "1"): "REGN",
    ("R6", "2"): "LED_CHG_A",
    ("R7", "1"): "REGN",
    ("R7", "2"): "LED_PGOOD_A",
}

FORBIDDEN_NET_NAMES = {
    "Q1_DRAIN_COMMON",
    "PACK_P",
    # r16 common-source node: its presence means the broken topology is back.
    "FET_SRC_COMMON",
}

# J7 must sit on the REAR board edge. The r16 board had it on the front edge at
# y = 16.5 mm, which presented the pin order that produced reversed polarity.
J7_MIN_Y_MM = 60.0


def pad_net_name(pad: pcbnew.PAD) -> str:
    net = pad.GetNet()
    return net.GetNetname() if net else ""


def footprint(board: pcbnew.BOARD, ref: str) -> pcbnew.FOOTPRINT | None:
    fp = board.FindFootprintByReference(ref)
    return fp if isinstance(fp, pcbnew.FOOTPRINT) else None


def matching_pads(board: pcbnew.BOARD, ref: str, pad_num: str) -> list[pcbnew.PAD]:
    fp = footprint(board, ref)
    if fp is None:
        return []
    return [pad for pad in fp.Pads() if pad.GetNumber() == pad_num]


def check_pad(board: pcbnew.BOARD, ref: str, pad_num: str, expected: str) -> list[str]:
    fp = footprint(board, ref)
    if fp is None:
        return [f"{ref}: missing footprint"]
    pads = matching_pads(board, ref, pad_num)
    if not pads:
        return [f"{ref}.{pad_num}: missing pad"]
    errors: list[str] = []
    for pad in pads:
        actual = pad_net_name(pad)
        if actual != expected:
            errors.append(f"{ref}.{pad_num}: expected {expected}, got {actual or '<none>'}")
    return errors


def main() -> int:
    board = pcbnew.LoadBoard(str(BOARD_PATH))
    errors: list[str] = []

    for key, expected in EXPECTED_PAD_NETS.items():
        errors.extend(check_pad(board, key[0], key[1], expected))

    net_names = {net.GetNetname() for net in board.GetNetInfo().NetsByName().values()}
    for forbidden in sorted(FORBIDDEN_NET_NAMES):
        if forbidden in net_names:
            errors.append(f"forbidden legacy net still present: {forbidden}")

    for ref in ("Q1", "Q2"):
        fp = footprint(board, ref)
        if fp is None:
            continue
        if "CSD18502Q5B_DNK0008A_VSON-CLIP_5x6mm" not in str(fp.GetFPID().GetLibItemName()):
            errors.append(f"{ref}: expected CSD18502Q5B_DNK0008A_VSON-CLIP_5x6mm footprint")

    j7 = footprint(board, "J7")
    if j7 is None:
        errors.append("J7: missing footprint")
    else:
        y_mm = pcbnew.ToMM(j7.GetPosition().y)
        if y_mm < J7_MIN_Y_MM:
            errors.append(
                f"J7 is at y={y_mm:.2f} mm, still on the front edge; "
                f"it must move to the rear edge (y >= {J7_MIN_Y_MM:.0f} mm) to correct polarity"
            )

    print("R17 topology audit")
    print(f"Board: {BOARD_PATH}")
    if errors:
        print("FAIL")
        for error in errors:
            print(f"- {error}")
        return 1

    print("PASS")
    print("- USB-C A12/B12/shield are GND.")
    print("- Charger BAT/SNS pins are protected BAT_SYS.")
    print("- Q1/Q2 are common-DRAIN high side: Q1 source at FET_BATT_P, Q2 source at BAT_SYS.")
    print("- Each 10M gate bleed returns to its own FET's source (R50->FET_BATT_P, R49->BAT_SYS).")
    print("- U2 PACK senses BAT_SYS, the DSG FET source, per SLUSAS3D 8.3.13.")
    print("- D2/D3 anodes are on pad 2 (KiCad LED_0603 pad 1 is the cathode).")
    print("- J7 is on the rear edge.")
    print("- R46 is in series between BATT_RAW_N and GND.")
    print("- Legacy PACK_P / Q1_DRAIN_COMMON / FET_SRC_COMMON nets are absent.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
