from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from .compat import resolve_compat_path


@dataclass
class NetworkSpec:
    nunits: int
    ninputs: int | None
    noutputs: int | None
    constraints: dict[str, float]
    weights: list[list[float]]


def parse_network_file(path: str | Path, base_dir: str | Path | None = None) -> NetworkSpec:
    resolved = resolve_compat_path(path, base_dir=base_dir)
    text = resolved.read_text(encoding="utf-8", errors="ignore")

    definitions: dict[str, int] = {}
    constraints: dict[str, float] = {}
    network_rows: list[str] = []

    section: str | None = None
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue

        if line.endswith(":"):
            section = line[:-1].lower()
            continue

        if line.lower() == "end":
            section = None
            continue

        if section == "definitions":
            parts = line.split()
            if len(parts) >= 2:
                definitions[parts[0].lower()] = int(parts[1])
            continue

        if section == "constraints":
            parts = line.split()
            if len(parts) >= 2 and len(parts[0]) == 1:
                constraints[parts[0]] = float(parts[1])
            continue

        if section == "network":
            network_rows.append(line)

    if "nunits" not in definitions:
        raise ValueError(f"Missing nunits in network file: {resolved}")

    nunits = definitions["nunits"]
    if len(network_rows) != nunits:
        raise ValueError(
            f"Expected {nunits} network rows, got {len(network_rows)} in {resolved}"
        )

    weights: list[list[float]] = []
    for row in network_rows:
        if len(row) != nunits:
            raise ValueError(
                f"Network row has length {len(row)}; expected {nunits} in {resolved}"
            )

        weight_row: list[float] = []
        for symbol in row:
            if symbol == ".":
                weight_row.append(0.0)
                continue
            if symbol not in constraints:
                raise ValueError(f"Unknown constraint symbol '{symbol}' in {resolved}")
            weight_row.append(constraints[symbol])
        weights.append(weight_row)

    return NetworkSpec(
        nunits=nunits,
        ninputs=definitions.get("ninputs"),
        noutputs=definitions.get("noutputs"),
        constraints=constraints,
        weights=weights,
    )
