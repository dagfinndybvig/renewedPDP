#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PYTHONPATH="$repo_root/pythonPDP/src" REPO_ROOT="$repo_root" python - << 'PY'
import os
import subprocess
import tempfile
from pathlib import Path

from pdp.io.network_parser import parse_network_file
from pdp.io.template_parser import TemplateEntry, parse_template_file
from pdp.models.iac import IACModel

repo_root = Path(os.environ["REPO_ROOT"]).resolve()
iac_dir = repo_root / "iac"


def unit_names() -> list[str]:
    return (
        "Jets Sharks in20s in30s in40s JH HS College Single Married Divorced "
        "Pusher Burglar Bookie Art Al Sam Clyde Mike Jim Greg John Doug Lance "
        "George Pete Fred Gene Ralph Phil Ike Nick Don Ned Karl Ken Earl Rick "
        "Ol Neal Dave _Art _Al _Sam _Clyde _Mike _Jim _Greg _John _Doug _Lance "
        "_George _Pete _Fred _Gene _Ralph _Phil _Ike _Nick _Don _Ned _Karl _Ken "
        "_Earl _Rick _Ol _Neal _Dave"
    ).split()


def render_entry_values(model: IACModel, entry: TemplateEntry) -> list[float]:
    if entry.template_type == "variable":
        if entry.variable and entry.variable.lower() == "cycleno":
            return [float(model.cycleno)]
        return []

    if entry.template_type != "look" or entry.look_table is None or not entry.variable:
        return []

    var = entry.variable.lower()
    if var == "activation":
        source = model.activation
    elif var == "extinput":
        source = model.extinput
    elif var == "netinput":
        source = model.netinput
    else:
        return []

    values: list[float] = []
    for token in entry.look_table.cells:
        if token is None:
            continue
        idx = int(token)
        if 0 <= idx < len(source):
            values.append(float(source[idx]))
    return values


def python_rows(save_level: int, setters: list[tuple[str, float]]) -> list[list[float]]:
    spec = parse_network_file("JETS.NET", base_dir=iac_dir)
    template = parse_template_file("JETS.TEM", base_dir=iac_dir)

    model = IACModel()
    model.load_network(spec)
    model.load_template(template)
    model.set_unit_names(unit_names())

    for name, value in setters:
        model.set_param(name, value)

    entries = [e for e in template.entries if e.level > 0 and e.level <= save_level]

    rows: list[list[float]] = []
    for _ in range(model.ncycles):
        model.cycle(1)
        row: list[float] = []
        for entry in entries:
            row.extend(render_entry_values(model, entry))
        rows.append(row)
    return rows


def parse_numeric_rows(path: Path) -> list[list[float]]:
    rows: list[list[float]] = []
    for line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        tokens = line.split()
        if not tokens:
            continue
        try:
            rows.append([float(token) for token in tokens])
        except ValueError:
            continue
    return rows


def max_abs_diff(a_rows: list[list[float]], b_rows: list[list[float]]) -> tuple[int, int, float]:
    compared = min(len(a_rows), len(b_rows))
    max_abs = 0.0
    for i in range(compared):
        a, b = a_rows[i], b_rows[i]
        limit = min(len(a), len(b))
        for j in range(limit):
            diff = abs(a[j] - b[j])
            if diff > max_abs:
                max_abs = diff
    return compared, len(a_rows), max_abs


regimes = [
    {
        "name": "default-activation",
        "save_level": 1,
        "legacy_set": [],
        "python_set": [],
    },
    {
        "name": "extinput-and-activation",
        "save_level": 2,
        "legacy_set": [
            "set param estr .4",
            "set param alpha .15",
            "set param gamma .2",
        ],
        "python_set": [("estr", 0.4), ("alpha", 0.15), ("gamma", 0.2)],
    },
    {
        "name": "gb-mode",
        "save_level": 2,
        "legacy_set": [
            "exam mode gb 1",
            "set param estr .4",
            "set param decay .12",
            "set param rest -.08",
        ],
        "python_set": [("gb", 1.0), ("estr", 0.4), ("decay", 0.12), ("rest", -0.08)],
    },
]

threshold = 1e-4
all_ok = True

with tempfile.TemporaryDirectory() as tmp:
    tmpdir = Path(tmp)
    for regime in regimes:
        name = regime["name"]
        save_level = int(regime["save_level"])
        legacy_log = tmpdir / f"legacy-{name}.log"
        cmd_file = tmpdir / f"{name}.com"

        commands = [
            f"set dlevel {save_level}",
            f"set slevel {save_level}",
            f"log {legacy_log}",
            "get network JETS.NET",
            "get unames " + " ".join(unit_names()) + " end",
        ]
        commands.extend(regime["legacy_set"])
        commands.extend(["cycle", "quit y"])
        cmd_file.write_text("\n".join(commands) + "\n", encoding="utf-8")

        subprocess.run(
            ["./iac", "JETS.TEM", str(cmd_file)],
            cwd=iac_dir,
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

        legacy_rows = parse_numeric_rows(legacy_log)
        py_rows = python_rows(save_level, regime["python_set"])
        compared, legacy_len, max_abs = max_abs_diff(legacy_rows, py_rows)

        print(f"[{name}] compared_rows={compared} legacy_rows={legacy_len} python_rows={len(py_rows)} max_abs_diff={max_abs:.8f}")

        if compared == 0 or max_abs > threshold:
            all_ok = False

if not all_ok:
    raise SystemExit(2)

print("iac parity check passed (all regimes)")
PY
