from __future__ import annotations

import argparse
from pathlib import Path
from typing import Sequence

from pdp.io.network_parser import parse_network_file
from pdp.io.pattern_parser import parse_pattern_file
from pdp.io.template_parser import TemplateEntry, parse_template_file
from pdp.models.iac import IACModel
from pdp.runtime.command_stream import CommandTokenStream


class IACSession:
    def __init__(self, data_dir: Path) -> None:
        self.data_dir = data_dir
        self.model = IACModel()

    # ------------------------------------------------------------------
    # Token-stream execution (matches C get_command() token-at-a-time)
    # ------------------------------------------------------------------

    def run_stream(self, stream: CommandTokenStream) -> bool:
        """Consume tokens from *stream* and dispatch commands.

        Returns True normally; False if 'quit y' was received.
        """
        while not stream.is_empty():
            tok = stream.next()
            if tok is None:
                break
            keep = self._dispatch(tok, stream)
            if not keep:
                return False
        return True

    def _dispatch(self, verb: str, stream: CommandTokenStream) -> bool:
        """Handle a single command verb, pulling further tokens from *stream*."""
        v = verb.lower()

        if v == "quit":
            confirm = stream.next()
            if confirm and confirm.lower().startswith("y"):
                return False
            # no confirm token — ask interactively only if at TTY
            return True

        if v == "set":
            key = stream.next_required("set")
            self._handle_set_tokens(key, stream)
            return True

        if v == "get":
            target = stream.next_required("get")
            self._handle_get_tokens(target, stream)
            return True

        if v == "cycle":
            count_tok = stream.peek()
            count: int | None = None
            if count_tok and count_tok.lstrip("-").isdigit():
                stream.next()
                count = int(count_tok)
            self.model.cycle(count)
            print(f"cycle complete: cycleno={self.model.cycleno}")
            return True

        if v == "test":
            pattern_ref = stream.next_required("test")
            index = self.model.test_pattern(pattern_ref)
            print(
                f"test complete: pattern={self.model.pattern_names[index]}"
                f" cycleno={self.model.cycleno}"
            )
            return True

        if v == "reset":
            self.model.reset_state()
            print("state reset")
            return True

        if v in {"state", "display"}:
            self.render_template_state()
            return True

        if v == "input":
            unit = stream.next_required("input")
            strength = float(stream.next_required("input strength"))
            self.model.set_input(unit, strength)
            print(f"input set: {unit}={strength:.4f}")
            return True

        # Silently ignore display/UI-only commands
        if v in {"newstart", "step", "printout"}:
            return True

        print(f"unsupported command: {verb}")
        return True

    # ------------------------------------------------------------------
    # Backward-compat: line-oriented interface (used by tests / old code)
    # ------------------------------------------------------------------

    def run_line(self, line: str) -> bool:
        """Run a single pre-tokenised line (space-separated tokens)."""
        stream = CommandTokenStream()
        stream.feed_string(line)
        return self.run_stream(stream)


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

    def load_template(self, template_path: str | Path) -> None:
        spec = parse_template_file(str(template_path), base_dir=self.data_dir)
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

    def _handle_set_tokens(self, key: str, stream: CommandTokenStream) -> None:
        k = key.lower()
        if k in {"dlevel", "slevel"}:
            stream.next_required("set dlevel/slevel value")
            return
        if k == "param":
            param_name = stream.next_required("set param name")
            param_val = stream.next_required("set param value")
            self.model.set_param(param_name, float(param_val))
            return
        # bare: set <param> <value>
        val = stream.next_required(f"set {key} value")
        try:
            self.model.set_param(key, float(val))
        except ValueError:
            pass  # silently ignore unknown bare set keys

    def _handle_get_tokens(self, target: str, stream: CommandTokenStream) -> None:
        t = target.lower()
        if t == "network":
            filename = stream.next_required("get network filename")
            spec = parse_network_file(filename, base_dir=self.data_dir)
            self.model.load_network(spec)
            print(f"network loaded: nunits={self.model.nunits}")
            return
        if t == "unames":
            names = stream.consume_until("end")
            self.model.set_unit_names(names)
            print(f"unit names loaded: {len(names)}")
            return
        if t == "patterns":
            filename = stream.next_required("get patterns filename")
            if self.model.nunits <= 0:
                raise ValueError("Load network before patterns")
            pattern_set = parse_pattern_file(
                filename, expected_inputs=self.model.nunits, base_dir=self.data_dir
            )
            self.model.load_patterns(pattern_set)
            print(f"patterns loaded: {len(pattern_set.names)}")
            return
        print(f"unsupported get target: {target}")

    # Keep old dict-of-parts helpers for backward compat with tests
    def _handle_set(self, parts: list[str]) -> None:
        stream = CommandTokenStream()
        stream.feed_string(" ".join(parts[1:]))
        self._handle_set_tokens(stream.next_required("set key"), stream)

    def _handle_get(self, parts: list[str]) -> None:
        stream = CommandTokenStream()
        stream.feed_string(" ".join(parts[1:]))
        self._handle_get_tokens(stream.next_required("get target"), stream)


def _run_script(session: IACSession, script_path: Path) -> bool:
    stream = CommandTokenStream()
    stream.feed_file(script_path)
    try:
        keep_running = session.run_stream(stream)
    except Exception as exc:
        print(f"error: {exc}")
        return False
    return True


def _interactive_loop(session: IACSession) -> int:
    """REPL / piped-stdin runner that mirrors C get_command() behaviour.

    When stdin is a pipe/file, read all input at once and run as a script.
    When stdin is a real terminal, use a persistent stream so multi-line
    commands work across prompts.
    """
    import sys

    if not sys.stdin.isatty():
        text = sys.stdin.read()
        stream = CommandTokenStream()
        stream.feed_string(text)
        try:
            session.run_stream(stream)
        except Exception as exc:
            print(f"error: {exc}")
            return 1
        return 0

    print("pythonPDP iac. Type 'quit y' to exit.")
    stream = CommandTokenStream()
    while True:
        try:
            line = input("iac: ")
        except EOFError:
            break
        except KeyboardInterrupt:
            print()
            break

        stream.feed_string(line)

        try:
            keep_running = session.run_stream(stream)
        except Exception as exc:
            print(f"error: {exc}")
            stream = CommandTokenStream()  # reset on error
            continue

        if not keep_running:
            break

    return 0


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Python port of the PDP iac model.\n\n"
        "Invocation matches the C binary:\n"
        "  cd iac/  &&  pdp-iac JETS.TEM JETS.STR"
    )
    parser.add_argument(
        "template", nargs="?",
        help="Template file (.TEM), resolved relative to cwd",
    )
    parser.add_argument(
        "command_file", nargs="?",
        help="Command/script file (.STR), resolved relative to cwd",
    )
    parser.add_argument(
        "--data-dir", default=".",
        help="Base directory for data file lookups (default: cwd)",
    )
    parser.add_argument(
        "--interactive", action="store_true",
        help="Enter interactive mode after running command file",
    )
    return parser


def main() -> int:
    parser = build_arg_parser()
    args = parser.parse_args()

    # data_dir: explicit flag wins; otherwise the directory containing the
    # template (or cwd if no template given) — same as the C binary behaviour
    # where all files are resolved relative to the working directory.
    if args.data_dir != ".":
        data_dir = Path(args.data_dir)
    elif args.template:
        data_dir = Path(args.template).resolve().parent
    else:
        data_dir = Path(".")

    session = IACSession(data_dir=data_dir)

    if args.template:
        try:
            session.load_template(Path(args.template))
        except Exception as exc:
            print(f"error loading template '{args.template}': {exc}")
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
