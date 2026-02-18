"""CLI entrypoint for the pa (pattern associator) model.

Invocation mirrors the C binary exactly::

    cd pa/
    pdp-pa JETS.TEM JETS.STR          # run script then exit
    pdp-pa JETS.TEM JETS.STR --interactive  # run script then enter REPL
    pdp-pa JETS.TEM                   # REPL only (template loaded)
    pdp-pa                            # bare REPL

Like the C version, all filenames in command files are resolved relative to the
directory of the template file (i.e., cwd when you ``cd pa/`` first).

Command syntax follows the C ``get_command()`` token-at-a-time model: tokens
may be separated by spaces on one line OR split across multiple lines.

Supported commands
------------------
get network <file>          load a .NET file
get patterns <file>         load an input+target .PAT file
set nepochs <n>
set lflag <0|1>
set param lrate <v>
set param noise <v>
set param temp <v>
set param ecrit <v>
set mode linear <0|1>       linear output
set mode lt <0|1>           threshold-linear output
set mode cs <0|1>           continuous sigmoid
set mode hebb <0|1>         Hebb rule (default: delta)
strain                      sequential training
ptrain                      permuted training
tall                        one pass, no weight update
test <pattern>              single forward pass
reset                       zero weights and state
state / display             print current state
quit [y]                    exit
"""
from __future__ import annotations

import argparse
from pathlib import Path

from pdp.io.network_parser import parse_network_file
from pdp.io.pattern_parser import parse_pattern_pairs_file
from pdp.models.pa import PAModel
from pdp.runtime.command_stream import CommandTokenStream


class PASession:
    def __init__(self, data_dir: Path) -> None:
        self.data_dir = data_dir
        self.model = PAModel()

    # ------------------------------------------------------------------
    # Token-stream execution
    # ------------------------------------------------------------------

    def run_stream(self, stream: CommandTokenStream) -> bool:
        """Consume all tokens in *stream*, dispatching commands.  Returns False on quit."""
        while not stream.is_empty():
            tok = stream.next()
            if tok is None:
                break
            keep = self._dispatch(tok, stream)
            if not keep:
                return False
        return True

    def _dispatch(self, verb: str, stream: CommandTokenStream) -> bool:
        v = verb.lower()

        if v == "quit":
            confirm = stream.next()
            if confirm and confirm.lower().startswith("y"):
                return False
            return True

        if v == "get":
            target = stream.next_required("get")
            self._handle_get_tokens(target, stream)
            return True

        if v == "set":
            key = stream.next_required("set")
            self._handle_set_tokens(key, stream)
            return True

        if v == "strain":
            self.model.strain()
            print(f"strain done: epochno={self.model.epochno} tss={self.model.tss:.6f}")
            return True

        if v == "ptrain":
            self.model.ptrain()
            print(f"ptrain done: epochno={self.model.epochno} tss={self.model.tss:.6f}")
            return True

        if v == "tall":
            self.model.tall()
            print(f"tall done: tss={self.model.tss:.6f}")
            return True

        if v == "test":
            pattern_ref = stream.next_required("test")
            index = self.model.test_pattern(pattern_ref)
            name = self.model.pattern_names[index] if self.model.pattern_names else str(index)
            out = self.model.output[self.model.ninputs :]
            tgt = self.model.target
            print(
                f"test {name!r}: output={[f'{v:.4f}' for v in out]}"
                f"  target={[f'{v:.4f}' for v in tgt]}"
                f"  pss={self.model.pss:.6f}"
            )
            return True

        if v == "reset":
            self.model.reset_weights()
            print("weights and state zeroed")
            return True

        if v in {"state", "display"}:
            self._print_state()
            return True

        if v in {"newstart", "printout"}:
            return True

        if v == "step":
            stream.next()  # consume the step-size argument
            return True

        print(f"unsupported command: {verb}")
        return True

    # ------------------------------------------------------------------

    def _handle_get_tokens(self, target: str, stream: CommandTokenStream) -> None:
        t = target.lower()

        if t in {"network", "net"}:
            filename = stream.next_required("get network filename")
            spec = parse_network_file(filename, base_dir=self.data_dir)
            self.model.load_network(spec)
            print(
                f"network loaded: nunits={self.model.nunits}"
                f" ninputs={self.model.ninputs} noutputs={self.model.noutputs}"
            )
            return

        if t in {"patterns", "pattern", "pat"}:
            filename = stream.next_required("get patterns filename")
            if self.model.nunits <= 0:
                raise ValueError("Load a network before patterns")
            pairs = parse_pattern_pairs_file(
                filename,
                expected_inputs=self.model.ninputs,
                expected_outputs=self.model.noutputs,
                base_dir=self.data_dir,
            )
            self.model.load_patterns(pairs)
            print(f"patterns loaded: {len(pairs.names)} pairs")
            return

        print(f"unsupported get target: {target}")

    def _handle_set_tokens(self, key: str, stream: CommandTokenStream) -> None:
        k = key.lower()

        if k in {"dlevel", "slevel"}:
            stream.next_required(f"set {k} value")
            return

        if k == "lflag":
            val = stream.next_required("set lflag value")
            self.model.lflag = bool(int(val))
            return

        if k == "nepochs":
            val = stream.next_required("set nepochs value")
            self.model.nepochs = int(val)
            return

        if k == "param":
            name = stream.next_required("set param name")
            val = stream.next_required("set param value")
            self._set_param(name.lower(), val)
            return

        if k == "mode":
            mode = stream.next_required("set mode name")
            flag = stream.next_required("set mode flag")
            flag_bool = bool(int(flag))
            m = mode.lower()
            if m == "linear":
                self.model.linear = flag_bool
            elif m == "lt":
                self.model.lt = flag_bool
            elif m in {"cs", "cs_mode"}:
                self.model.cs_mode = flag_bool
            elif m == "hebb":
                self.model.hebb = flag_bool
            else:
                print(f"unsupported mode: {mode}")
            return

        if k == "step":
            stream.next_required("set step value")
            return

        # bare: set <param> <value>
        val = stream.next_required(f"set {key} value")
        try:
            self._set_param(k, val)
        except Exception:
            print(f"unsupported set: {key} {val}")

    def _set_param(self, name: str, raw_value: str) -> None:
        value = float(raw_value)
        if name == "nepochs":
            self.model.nepochs = int(value)
        elif name == "lrate":
            self.model.lrate = value
        elif name == "noise":
            self.model.noise = value
        elif name == "temp":
            self.model.temp = value
        elif name == "ecrit":
            self.model.ecrit = value
        else:
            print(f"unsupported param: {name}")

    # ------------------------------------------------------------------

    def _print_state(self) -> None:
        m = self.model
        if m.nunits == 0:
            print("no network loaded")
            return
        out = m.output[m.ninputs :]
        print(f"epochno={m.epochno}  patno={m.patno}  tss={m.tss:.6f}  pss={m.pss:.6f}")
        print(f"output : {[f'{v:.4f}' for v in out]}")
        print(f"target : {[f'{v:.4f}' for v in m.target]}")
        err = [m.error[m.ninputs + i] for i in range(m.noutputs)]
        print(f"error  : {[f'{v:.4f}' for v in err]}")
        print(f"vcor={m.vcor:.4f}  ndp={m.ndp:.4f}  nvl={m.nvl:.4f}")

    # ------------------------------------------------------------------
    # Backward-compat line-based interface (used by tests)
    # ------------------------------------------------------------------

    def run_line(self, line: str) -> bool:
        stream = CommandTokenStream()
        stream.feed_string(line)
        return self.run_stream(stream)


# ------------------------------------------------------------------

def _run_script(session: PASession, script_path: Path) -> bool:
    stream = CommandTokenStream()
    stream.feed_file(script_path)
    try:
        return session.run_stream(stream)
    except Exception as exc:
        print(f"error: {exc}")
        return False


def _interactive_loop(session: PASession) -> int:
    """REPL / piped-stdin runner that mirrors C get_command() behaviour.

    When stdin is a pipe/file, read all input at once and run as a script.
    When stdin is a real terminal, use a persistent stream so multi-line
    commands work across prompts.
    """
    import sys

    if not sys.stdin.isatty():
        # Piped or redirected input: read everything, run as one stream
        text = sys.stdin.read()
        stream = CommandTokenStream()
        stream.feed_string(text)
        try:
            session.run_stream(stream)
        except Exception as exc:
            print(f"error: {exc}")
            return 1
        return 0

    # Real interactive terminal
    print("pythonPDP pa. Type 'quit y' to exit.")
    stream = CommandTokenStream()
    while True:
        try:
            line = input("pa: ")
        except (EOFError, KeyboardInterrupt):
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
        description="Python port of the PDP pa model.\n\n"
        "Invocation matches the C binary:\n"
        "  cd pa/  &&  pdp-pa JETS.TEM JETS.STR"
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

    if args.data_dir != ".":
        data_dir = Path(args.data_dir)
    elif args.template:
        data_dir = Path(args.template).resolve().parent
    else:
        data_dir = Path(".")

    session = PASession(data_dir=data_dir)

    if args.command_file:
        ok = _run_script(session, Path(args.command_file))
        if not ok:
            return 1

    if args.interactive or not args.command_file:
        return _interactive_loop(session)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
