"""CLI entrypoint for the bp (backpropagation) model.

Invocation mirrors the C binary exactly::

    cd bp/
    pdp-bp 424.TEM 424.STR          # run script then exit
    pdp-bp 424.TEM 424.STR --interactive  # run script then enter REPL
    pdp-bp 424.TEM                   # REPL only (template loaded)
    pdp-bp                            # bare REPL

Like the C version, all filenames in command files are resolved relative to
the directory of the template file (i.e. cwd when you ``cd bp/`` first).

Command syntax follows the C ``get_command()`` token-at-a-time model: tokens
may be separated by spaces on one line OR split across multiple lines.

Supported commands
------------------
get network <file>          load a .NET file and initialise weights
get patterns <file>         load input+target pattern pairs from a .PAT file
get weights <file>          load saved weights from a .WTS file
save weights <file>         save current weights to a file
set nepochs <n>
set ecrit <v>
set lflag <0|1>
set param lrate <v>
set param momentum <v>
set param tmax <v>
set param wrange <v>
set mode lgrain epoch|pattern
set mode cascade <0|1>
strain                      sequential training
ptrain                      permuted training
tall                        one pass, no weight update
test <pattern>              single forward pass on a named pattern
cycle [n]                   (cascade mode) run settling cycles
reset                       re-draw random weights
newstart                    pick new seed then reset
state / display             print current state
quit [y]                    exit
"""
from __future__ import annotations

import argparse
from pathlib import Path

from pdp.io.bp_network_parser import parse_bp_network_file
from pdp.io.pattern_parser import parse_pattern_pairs_file
from pdp.models.bp import BPModel
from pdp.runtime.command_stream import CommandTokenStream


class BPSession:
    def __init__(self, data_dir: Path) -> None:
        self.data_dir = data_dir
        self.model = BPModel()

    # ------------------------------------------------------------------
    # Token-stream execution
    # ------------------------------------------------------------------

    def run_stream(self, stream: CommandTokenStream) -> bool:
        """Consume all tokens in *stream*, dispatching commands.

        Returns False when quit is requested."""
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
            self._handle_get(target, stream)
            return True

        if v == "save":
            target = stream.next_required("save")
            self._handle_save(target, stream)
            return True

        if v == "set":
            key = stream.next_required("set")
            self._handle_set(key, stream)
            return True

        if v == "strain":
            self._ensure_ready("strain")
            self.model.strain()
            print(
                f"strain done: epochno={self.model.epochno}"
                f"  tss={self.model.tss:.6f}"
            )
            return True

        if v == "ptrain":
            self._ensure_ready("ptrain")
            self.model.ptrain()
            print(
                f"ptrain done: epochno={self.model.epochno}"
                f"  tss={self.model.tss:.6f}"
            )
            return True

        if v == "tall":
            self._ensure_ready("tall")
            self.model.tall()
            print(f"tall done: tss={self.model.tss:.6f}")
            self._print_output()
            return True

        if v == "test":
            pattern_ref = stream.next_required("test")
            self._ensure_ready("test")
            index = self.model.test_pattern(pattern_ref)
            name = self.model.pattern_names[index] \
                if self.model.pattern_names else str(index)
            out = self.model.output_activations
            tgt = self.model.target
            print(
                f"test {name!r}: output={[f'{v:.4f}' for v in out]}"
                f"  target={[f'{v:.4f}' for v in tgt]}"
                f"  pss={self.model.pss:.6f}"
            )
            return True

        if v == "cycle":
            n_tok = stream.next()
            if n_tok is not None:
                try:
                    self.model.ncycles = int(n_tok)
                except ValueError:
                    pass
            if self.model.cascade:
                self.model.cycle()
                print(f"cycle done: cycleno={self.model.cycleno}")
            else:
                print("(note: cycle only applies in cascade mode)")
            return True

        if v == "reset":
            self.model.reset_weights()
            print("weights re-initialised")
            return True

        if v == "newstart":
            self.model.newstart()
            print(f"new seed drawn; weights re-initialised")
            return True

        if v in {"state", "display"}:
            self._print_state()
            return True

        # Silently ignore C-only display/stepping commands
        if v in {"printout", "step"}:
            if v == "step":
                stream.next()  # consume argument
            return True

        print(f"unsupported command: {verb!r}")
        return True

    # ------------------------------------------------------------------
    # get / save sub-handlers
    # ------------------------------------------------------------------

    def _handle_get(self, target: str, stream: CommandTokenStream) -> None:
        t = target.lower()

        if t in {"network", "net"}:
            filename = stream.next_required("get network filename")
            spec = parse_bp_network_file(filename, base_dir=self.data_dir)
            self.model.load_network(spec)
            print(
                f"network loaded: nunits={self.model.nunits}"
                f" ninputs={self.model.ninputs}"
                f" noutputs={self.model.noutputs}"
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

        if t in {"weights", "weight", "wts"}:
            filename = stream.next_required("get weights filename")
            if self.model.nunits <= 0:
                raise ValueError("Load a network before weights")
            self.model.load_weights(filename, base_dir=self.data_dir)
            print("weights loaded")
            return

        if t in {"unames", "uname"}:
            # Consume unit-name tokens until "end"
            names = []
            while True:
                tok = stream.next()
                if tok is None or tok.lower() == "end":
                    break
                names.append(tok)
            return  # unit names not used in bp

        print(f"unsupported get target: {target!r}")

    def _handle_save(self, target: str, stream: CommandTokenStream) -> None:
        t = target.lower()
        if t in {"weights", "weight", "wts"}:
            filename = stream.next_required("save weights filename")
            out_path = self.data_dir / filename if not Path(filename).is_absolute() \
                else Path(filename)
            self.model.save_weights(out_path)
            print(f"weights saved to {out_path}")
            return
        print(f"unsupported save target: {target!r}")

    # ------------------------------------------------------------------
    # set sub-handler
    # ------------------------------------------------------------------

    def _handle_set(self, key: str, stream: CommandTokenStream) -> None:
        k = key.lower()

        if k in {"dlevel", "slevel"}:
            stream.next_required(f"set {k} value")  # consume, not used
            return

        if k == "lflag":
            val = stream.next_required("set lflag value")
            self.model.lflag = bool(int(val))
            return

        if k == "nepochs":
            val = stream.next_required("set nepochs value")
            self.model.nepochs = int(val)
            return

        if k == "ecrit":
            val = stream.next_required("set ecrit value")
            self.model.ecrit = float(val)
            return

        if k == "param":
            name = stream.next_required("set param name")
            val = stream.next_required("set param value")
            self._set_param(name.lower(), val)
            return

        if k == "mode":
            mode = stream.next_required("set mode name")
            flag = stream.next_required("set mode flag/value")
            self._set_mode(mode.lower(), flag)
            return

        if k == "step":
            stream.next_required("set step value")  # consume, not used
            return

        # Fallback: treat as bare "set <param> <value>"
        val = stream.next_required(f"set {key} value")
        try:
            self._set_param(k, val)
        except Exception:
            print(f"unsupported set: {key!r} {val!r}")

    def _set_param(self, name: str, raw: str) -> None:
        value = float(raw)
        if name == "lrate":
            self.model.lrate = value
        elif name == "momentum":
            self.model.momentum = value
        elif name == "tmax":
            self.model.tmax = value
        elif name == "wrange":
            if self.model._spec is not None:
                self.model._spec.wrange = value
        elif name == "ecrit":
            self.model.ecrit = value
        elif name == "nepochs":
            self.model.nepochs = int(value)
        elif name == "ncycles":
            self.model.ncycles = int(value)
        elif name == "crate":
            self.model.crate = value
        else:
            print(f"unsupported param: {name!r}")

    def _set_mode(self, mode: str, flag_or_val: str) -> None:
        if mode == "lgrain":
            val = flag_or_val.lower()
            if val.startswith("e"):
                self.model.lgrain = "epoch"
            elif val.startswith("p"):
                self.model.lgrain = "pattern"
            else:
                print(f"unknown lgrain value: {flag_or_val!r}")
            return

        if mode == "follow":
            # follow mode: gradient correlation — consume, ignore for now
            return

        if mode == "cascade":
            self.model.cascade = bool(int(flag_or_val))
            return

        print(f"unsupported mode: {mode!r}")

    # ------------------------------------------------------------------
    # Guards
    # ------------------------------------------------------------------

    def _ensure_ready(self, cmd: str) -> None:
        if self.model.nunits <= 0:
            raise ValueError(f"Cannot run '{cmd}': no network loaded")
        if not self.input_patterns_loaded:
            raise ValueError(f"Cannot run '{cmd}': no patterns loaded")

    @property
    def input_patterns_loaded(self) -> bool:
        return len(self.model.input_patterns) > 0

    # ------------------------------------------------------------------
    # Display helpers
    # ------------------------------------------------------------------

    def _print_output(self) -> None:
        """Print output activations for each pattern (used after tall)."""
        m = self.model
        if not m.pattern_names:
            return
        npatterns = len(m.input_patterns)
        # Re-run all patterns to display output
        save_tss = m.tss
        save_pss = m.pss
        for idx in range(npatterns):
            m.patno = idx
            m.setinput()
            m.settarget()
            m.compute_output()
            name = m.pattern_names[idx] if m.pattern_names else str(idx)
            out = [f"{v:.4f}" for v in m.output_activations]
            tgt = [f"{v:.4f}" for v in m.target]
            print(f"  {name:<12} output={out}  target={tgt}")
        m.tss = save_tss
        m.pss = save_pss

    def _print_state(self) -> None:
        m = self.model
        if m.nunits == 0:
            print("no network loaded")
            return
        print(
            f"epochno={m.epochno}  patno={m.patno}  cycleno={m.cycleno}"
            f"  tss={m.tss:.6f}  pss={m.pss:.6f}"
        )
        print(f"output : {[f'{v:.4f}' for v in m.output_activations]}")
        print(f"target : {[f'{v:.4f}' for v in m.target]}")
        first_out = m.nunits - m.noutputs
        err = [m.error[first_out + j] for j in range(m.noutputs)]
        print(f"error  : {[f'{v:.4f}' for v in err]}")
        print(f"delta  : {[f'{v:.6f}' for v in m.delta[m.ninputs:]]}")

    # ------------------------------------------------------------------
    # Backward-compat line-based interface (used by tests)
    # ------------------------------------------------------------------

    def run_line(self, line: str) -> bool:
        stream = CommandTokenStream()
        stream.feed_string(line)
        return self.run_stream(stream)


# ------------------------------------------------------------------
# Script runner / REPL
# ------------------------------------------------------------------

def _run_script(session: BPSession, script_path: Path) -> bool:
    stream = CommandTokenStream()
    stream.feed_file(script_path)
    try:
        return session.run_stream(stream)
    except Exception as exc:
        print(f"error: {exc}")
        return False


def _interactive_loop(session: BPSession) -> int:
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

    print("pythonPDP bp. Type 'quit y' to exit.")
    stream = CommandTokenStream()
    while True:
        try:
            line = input("bp: ")
        except (EOFError, KeyboardInterrupt):
            print()
            break

        stream.feed_string(line)

        try:
            keep_running = session.run_stream(stream)
        except Exception as exc:
            print(f"error: {exc}")
            stream = CommandTokenStream()
            continue

        if not keep_running:
            break

    return 0


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Python port of the PDP bp (backpropagation) model.\n\n"
        "Invocation matches the C binary:\n"
        "  cd bp/  &&  pdp-bp 424.TEM 424.STR"
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

    session = BPSession(data_dir=data_dir)

    if args.command_file:
        ok = _run_script(session, Path(args.command_file))
        if not ok:
            return 1

    if args.interactive or not args.command_file:
        return _interactive_loop(session)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
