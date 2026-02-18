from __future__ import annotations

import argparse
import shlex
from pathlib import Path
from typing import Sequence

from pdp.io.network_parser import parse_network_file
from pdp.io.pattern_parser import parse_pattern_file
from pdp.io.template_parser import TemplateEntry, parse_template_file
from pdp.models.iac import IACModel


class IACSession:
    def __init__(self, data_dir: Path) -> None:
        self.data_dir = data_dir
        self.model = IACModel()
        self.pending_quit_confirm = False

    def run_line(self, line: str) -> bool:
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            return True

        if self.pending_quit_confirm:
            self.pending_quit_confirm = False
            if stripped.lower().startswith("y"):
                return False
            return True

        parts = shlex.split(stripped)
        if not parts:
            return True

        cmd = parts[0].lower()

        if cmd == "quit":
            if len(parts) >= 2 and parts[1].lower().startswith("y"):
                return False
            self.pending_quit_confirm = True
            return True

        if cmd == "set":
            self._handle_set(parts)
            return True

        if cmd == "get":
            self._handle_get(parts)
            return True

        if cmd == "cycle":
            count = int(parts[1]) if len(parts) > 1 else None
            self.model.cycle(count)
            print(f"cycle complete: cycleno={self.model.cycleno}")
            return True

        if cmd == "test":
            if len(parts) < 2:
                raise ValueError("test requires a pattern name or index")
            index = self.model.test_pattern(parts[1])
            print(
                f"test complete: pattern={self.model.pattern_names[index]} cycleno={self.model.cycleno}"
            )
            return True

        if cmd == "reset":
            self.model.reset_state()
            print("state reset")
            return True

        if cmd in {"state", "display"}:
            self.render_template_state()
            return True

        if cmd == "input" and len(parts) >= 3:
            self.model.set_input(parts[1], float(parts[2]))
            print(f"input set: {parts[1]}={float(parts[2]):.4f}")
            return True

        print(f"unsupported command (initial port): {stripped}")
        return True

    def load_template(self, template_path: str) -> None:
        spec = parse_template_file(template_path, base_dir=self.data_dir)
        self.model.load_template(spec)
        print(f"template loaded: entries={len(spec.entries)}")

    def _resolve_template_index(self, token: str) -> int | None:
        if token is None:
            return None
        stripped = token.strip()
        if not stripped:
            return None
        if stripped.lstrip("+-").isdigit():
            value = int(stripped)
            if 0 <= value < self.model.nunits:
                return value
        return None

    def _format_legacy_number(self, value: float, digits: int, scale: float) -> str:
        scaled = value * scale
        scaled += 0.0000001 if scaled >= 0 else -0.0000001
        intval = int(scaled)

        abs_val = abs(intval)
        max_abs = (10**digits) - 1 if digits > 0 else 0
        if abs_val > max_abs:
            return "*" * max(1, digits)

        return f"{abs_val:>{max(1, digits)}d}"

    def _format_legacy_text(self, value: str, digits: int) -> str:
        width = max(1, digits)
        return f"{value:<{width}.{width}}"

    def _cell_to_string(
        self,
        values: Sequence[float] | Sequence[str],
        index: int,
        *,
        digits: int | None,
        scale: float | None,
    ) -> str:
        if index >= len(values):
            return "?"

        value = values[index]
        if isinstance(value, str):
            return self._format_legacy_text(value, digits or 1)
        return self._format_legacy_number(value, digits or 1, scale or 1.0)

    def _values_for_variable(self, variable: str) -> list[float] | list[str] | None:
        key = variable.lower()
        if key == "activation":
            return self.model.activation
        if key == "extinput":
            return self.model.extinput
        if key == "netinput":
            return self.model.netinput
        if key == "uname":
            return self.model.unit_names
        return None

    def _render_look_entry(self, entry: TemplateEntry) -> None:
        table = entry.look_table
        if table is None or entry.variable is None:
            return

        values = self._values_for_variable(entry.variable)
        if values is None:
            return

        print(f"[{entry.name}] {entry.template_type} -> {entry.variable}")
        spacing = entry.spacing or 1
        for row in range(table.rows):
            display_cells: list[str] = []
            for col in range(table.cols):
                cell_token = table.cells[row * table.cols + col]
                if cell_token is None:
                    display_cells.append(".".ljust(spacing))
                    continue

                index = self._resolve_template_index(cell_token)
                if index is None:
                    display_cells.append(cell_token.ljust(spacing))
                    continue

                text = self._cell_to_string(
                    values,
                    index,
                    digits=entry.digits,
                    scale=entry.scale,
                )
                display_cells.append(text.ljust(spacing))

            print("  " + "".join(display_cells).rstrip())

    def render_template_state(self) -> None:
        template = self.model.template
        if template is None:
            print("no template loaded")
            return

        print(f"template state: {template.source_path.name}")
        for entry in template.entries:
            if entry.template_type in {"look", "label_look"}:
                self._render_look_entry(entry)

    def _handle_set(self, parts: list[str]) -> None:
        if len(parts) < 3:
            raise ValueError("set command requires arguments")

        if parts[1].lower() in {"dlevel", "slevel"}:
            return

        if parts[1].lower() == "param" and len(parts) >= 4:
            self.model.set_param(parts[2], float(parts[3]))
            return

        self.model.set_param(parts[1], float(parts[2]))

    def _handle_get(self, parts: list[str]) -> None:
        if len(parts) < 2:
            raise ValueError("get command requires a target")

        target = parts[1].lower()
        if target == "network":
            if len(parts) < 3:
                raise ValueError("get network requires a file")
            spec = parse_network_file(parts[2], base_dir=self.data_dir)
            self.model.load_network(spec)
            print(f"network loaded: nunits={self.model.nunits}")
            return

        if target == "unames":
            if len(parts) < 3:
                raise ValueError("get unames requires names ending with 'end'")
            names = []
            for token in parts[2:]:
                if token.lower() == "end":
                    break
                names.append(token)
            self.model.set_unit_names(names)
            print(f"unit names loaded: {len(names)}")
            return

        if target == "patterns":
            if len(parts) < 3:
                raise ValueError("get patterns requires a file")
            if self.model.nunits <= 0:
                raise ValueError("Load network before patterns")
            pattern_set = parse_pattern_file(
                parts[2], expected_inputs=self.model.nunits, base_dir=self.data_dir
            )
            self.model.load_patterns(pattern_set)
            print(f"patterns loaded: {len(pattern_set.names)}")
            return

        print(f"unsupported get target (initial port): {target}")


def _run_script(session: IACSession, script_path: Path) -> bool:
    for line in script_path.read_text(encoding="utf-8", errors="ignore").splitlines():
        try:
            keep_running = session.run_line(line)
        except Exception as exc:
            print(f"error: {exc}")
            return False
        if not keep_running:
            return True
    return True


def _interactive_loop(session: IACSession) -> int:
    print("pythonPDP iac (initial port). Type 'quit' then 'y' to exit.")
    while True:
        try:
            line = input("iac: ")
        except EOFError:
            break
        except KeyboardInterrupt:
            print()
            break

        try:
            keep_running = session.run_line(line)
        except Exception as exc:
            print(f"error: {exc}")
            continue

        if not keep_running:
            break

    return 0


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Initial Python port of IAC")
    parser.add_argument("template", nargs="?", help="Template file path (accepted for compatibility)")
    parser.add_argument("command_file", nargs="?", help="Command file (.STR style)")
    parser.add_argument("--data-dir", default="iac", help="Base directory for relative data file lookups")
    parser.add_argument("--interactive", action="store_true", help="Enter interactive mode after command file")
    return parser


def main() -> int:
    parser = build_arg_parser()
    args = parser.parse_args()

    data_dir = Path(args.data_dir)
    session = IACSession(data_dir=data_dir)

    if args.template:
        try:
            session.load_template(args.template)
        except Exception as exc:
            print(f"error: failed to load template '{args.template}': {exc}")
            return 1

    if args.command_file:
        ok = _run_script(session, Path(args.command_file))
        if not ok:
            return 1

    if args.interactive or not args.command_file:
        return _interactive_loop(session)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
