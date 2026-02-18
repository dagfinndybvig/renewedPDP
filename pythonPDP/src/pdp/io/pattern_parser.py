from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from .compat import resolve_compat_path


@dataclass
class PatternSet:
    names: list[str]
    inputs: list[list[float]]


def _parse_token(token: str) -> float:
    if token == "+":
        return 1.0
    if token == "-":
        return -1.0
    if token == ".":
        return 0.0
    return float(token)


def parse_pattern_file(
    path: str | Path,
    expected_inputs: int,
    *,
    base_dir: str | Path | None = None,
) -> PatternSet:
    if expected_inputs <= 0:
        raise ValueError("expected_inputs must be > 0")

    resolved = resolve_compat_path(path, base_dir=base_dir)
    text = resolved.read_text(encoding="utf-8", errors="ignore")

    names: list[str] = []
    patterns: list[list[float]] = []

    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue

        parts = line.split()
        if len(parts) < 1 + expected_inputs:
            raise ValueError(
                f"Pattern row too short in {resolved}: expected at least {1 + expected_inputs} tokens"
            )

        name = parts[0]
        values = parts[1 : 1 + expected_inputs]
        parsed_values = [_parse_token(token) for token in values]

        names.append(name)
        patterns.append(parsed_values)

    return PatternSet(names=names, inputs=patterns)
