from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from .compat import resolve_compat_path


@dataclass
class LookTable:
    rows: int
    cols: int
    cells: list[str | None]


@dataclass
class TemplateEntry:
    name: str
    template_type: str
    level: int
    y_token: str
    x_token: str
    variable: str | None
    orientation: str | None
    digits: int | None
    scale: float | None
    spacing: int | None
    extra: list[str]
    look_file: str | None = None
    look_table: LookTable | None = None


@dataclass
class TemplateSpec:
    source_path: Path
    layout_lines: list[str]
    entries: list[TemplateEntry]


def parse_look_file(path: str | Path, *, base_dir: str | Path | None = None) -> LookTable:
    resolved = resolve_compat_path(path, base_dir=base_dir)
    tokens = resolved.read_text(encoding="utf-8", errors="ignore").split()
    if len(tokens) < 2:
        raise ValueError(f"Invalid look file: {resolved}")

    rows = int(tokens[0])
    cols = int(tokens[1])
    expected = rows * cols

    raw_cells = tokens[2 : 2 + expected]
    if len(raw_cells) < expected:
        raise ValueError(
            f"Look file {resolved} has {len(raw_cells)} cells, expected {expected}"
        )

    cells: list[str | None] = [None if token == "." else token for token in raw_cells]
    return LookTable(rows=rows, cols=cols, cells=cells)


def _parse_template_entry(parts: list[str]) -> TemplateEntry:
    if len(parts) < 5:
        raise ValueError(f"Template line too short: {' '.join(parts)}")

    name = parts[0]
    template_type = parts[1].lower()
    level = int(parts[2])
    y_token = parts[3]
    x_token = parts[4]

    rest = parts[5:]

    if template_type in {"variable", "floatvar"}:
        if len(rest) < 3:
            raise ValueError(f"Invalid {template_type} entry for {name}")
        return TemplateEntry(
            name=name,
            template_type=template_type,
            level=level,
            y_token=y_token,
            x_token=x_token,
            variable=rest[0],
            orientation=None,
            digits=int(rest[1]),
            scale=float(rest[2]),
            spacing=None,
            extra=rest[3:],
        )

    if template_type == "label":
        if len(rest) < 2:
            raise ValueError(f"Invalid label entry for {name}")
        return TemplateEntry(
            name=name,
            template_type=template_type,
            level=level,
            y_token=y_token,
            x_token=x_token,
            variable=None,
            orientation=rest[0],
            digits=int(rest[1]),
            scale=None,
            spacing=None,
            extra=rest[2:],
        )

    if template_type in {"vector", "label_array"}:
        if len(rest) < 5:
            raise ValueError(f"Invalid {template_type} entry for {name}")
        return TemplateEntry(
            name=name,
            template_type=template_type,
            level=level,
            y_token=y_token,
            x_token=x_token,
            variable=rest[0],
            orientation=rest[1],
            digits=int(rest[2]),
            scale=float(rest[3]) if template_type == "vector" else None,
            spacing=None,
            extra=rest[4:],
        )

    if template_type == "matrix":
        if len(rest) < 8:
            raise ValueError(f"Invalid matrix entry for {name}")
        return TemplateEntry(
            name=name,
            template_type=template_type,
            level=level,
            y_token=y_token,
            x_token=x_token,
            variable=rest[0],
            orientation=rest[1],
            digits=int(rest[2]),
            scale=float(rest[3]),
            spacing=None,
            extra=rest[4:],
        )

    if template_type == "look":
        if len(rest) < 5:
            raise ValueError(f"Invalid look entry for {name}")
        return TemplateEntry(
            name=name,
            template_type=template_type,
            level=level,
            y_token=y_token,
            x_token=x_token,
            variable=rest[0],
            orientation=None,
            digits=int(rest[1]),
            scale=float(rest[2]),
            spacing=int(rest[3]),
            extra=[],
            look_file=rest[4],
        )

    if template_type == "label_look":
        if len(rest) < 5:
            raise ValueError(f"Invalid label_look entry for {name}")
        return TemplateEntry(
            name=name,
            template_type=template_type,
            level=level,
            y_token=y_token,
            x_token=x_token,
            variable=rest[0],
            orientation=rest[1],
            digits=int(rest[2]),
            scale=None,
            spacing=int(rest[3]),
            extra=[],
            look_file=rest[4],
        )

    raise ValueError(f"Unsupported template type: {template_type}")


def parse_template_file(path: str | Path, *, base_dir: str | Path | None = None) -> TemplateSpec:
    resolved = resolve_compat_path(path, base_dir=base_dir)
    lines = resolved.read_text(encoding="utf-8", errors="ignore").splitlines()

    entries: list[TemplateEntry] = []
    layout_lines: list[str] = []
    in_layout = False

    for raw_line in lines:
        line = raw_line.strip()
        if not line:
            if in_layout:
                layout_lines.append("")
            continue

        parts = line.split()
        if len(parts) >= 2 and parts[0].lower() == "define:" and parts[1].lower() == "layout":
            in_layout = True
            continue

        if in_layout:
            if parts[0].lower() == "end":
                in_layout = False
            else:
                layout_lines.append(raw_line.rstrip("\n"))
            continue

        entry = _parse_template_entry(parts)
        if entry.look_file:
            entry.look_table = parse_look_file(entry.look_file, base_dir=resolved.parent)
        entries.append(entry)

    return TemplateSpec(source_path=resolved, layout_lines=layout_lines, entries=entries)
