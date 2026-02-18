"""Parser for BP-style .NET files used by the backpropagation model.

The file format is identical in structure to the general PDP network files
but uses ``%`` block syntax in the ``network:`` and ``biases:`` sections
(in addition to explicit character-row strings that the original parser
understands for other models).

Block syntax (C source: ``weights.c``, ``read_network``/``read_biases``):

    %<char> rstart rnum sstart snum

    - ``%`` alone (no second char): the next ``rnum`` lines each give a
      string of ``snum`` characters that spec the weights for one receiver row.
    - ``%r`` (or any single letter after ``%``): use that character as the
      fill character for all (rstart..rstart+rnum-1) × (sstart..sstart+snum-1)
      entries; no extra line needed.

Bias block syntax:

    %<char> rstart rnum

    Same rules: ``%`` alone reads a string of width ``rnum`` from the next
    line; ``%<ch>`` fills all ``rnum`` bias entries with ``<ch>``.

Constraint syntax (per character):

    r  random             → weight = wrange * U(-0.5, 0.5)
    p  random positive    → weight = wrange * U(0, 1)
    n  random negative    → weight = wrange * U(-1, 0)
    a  0.3                → fixed value 0.3
    linked                → linked (symmetry) constraint (ignored here)

By default (no explicit constraints section):
    r → random
    p → random positive
    n → random negative

Returned structure
------------------
``BPNetworkSpec`` carries the sparse connectivity in ``first_weight_to`` /
``num_weights_to`` arrays, the weight initialisation specification
(``wchar`` character codes + ``wrange``), and ``definitions``
(nunits, ninputs, noutputs, etc.).

The weight *values* are not set here — ``BPModel.reset_weights`` will draw
them using the stored ``wchar`` codes and ``wrange``.
"""
from __future__ import annotations

import random
from dataclasses import dataclass, field
from pathlib import Path

from .compat import resolve_compat_path


@dataclass
class ConstraintSpec:
    """Mirrors the C ``struct constants`` for one letter."""
    value: float = 0.0
    random: bool = False
    positive: bool = False  # random positive
    negative: bool = False  # random negative
    linked: bool = False    # linked (tied weights)


@dataclass
class BPNetworkSpec:
    """Full specification parsed from a BP .NET file.

    Attributes
    ----------
    nunits, ninputs, noutputs
        Architecture dimensions.
    wrange
        Weight random initialisation range (from ``definitions:`` section).
    first_weight_to
        first_weight_to[i] = index of first sender unit for unit i.
    num_weights_to
        num_weights_to[i] = number of incoming weights for unit i.
    wchar
        wchar[i][j] = character code for weight[i][first_weight_to[i]+j].
    bchar
        bchar[i] = character code for bias[i].
    constraints
        Dict mapping lowercase letter → ``ConstraintSpec``.
    definitions
        All key→int pairs from the ``definitions:`` section.
    """
    nunits: int
    ninputs: int
    noutputs: int
    wrange: float
    first_weight_to: list[int]
    num_weights_to: list[int]
    wchar: list[list[str]]       # wchar[unit][weight_idx]
    bchar: list[str]             # bchar[unit]
    constraints: dict[str, ConstraintSpec]
    definitions: dict[str, int] = field(default_factory=dict)


# ---------------------------------------------------------------------------
# Public entry point
# ---------------------------------------------------------------------------

def parse_bp_network_file(
    path: str | Path,
    base_dir: str | Path | None = None,
    wrange: float = 1.0,
) -> BPNetworkSpec:
    """Parse a BP-format network description file.

    Parameters
    ----------
    path:
        Filename of the ``.NET`` file.
    base_dir:
        Directory to resolve relative filenames in (default: cwd).
    wrange:
        Default weight initialisation range; may be overridden by a
        ``wrange`` entry in the ``definitions:`` section.
    """
    resolved = resolve_compat_path(path, base_dir=base_dir)
    text = resolved.read_text(encoding="utf-8", errors="ignore")
    tokens = _tokenize(text)
    return _parse(tokens, wrange=wrange)


# ---------------------------------------------------------------------------
# Tokeniser (mirrors C fscanf "%s" — splits on whitespace, skips comments)
# ---------------------------------------------------------------------------

def _tokenize(text: str) -> list[str]:
    """Split the file text into whitespace-delimited tokens."""
    return text.split()


# ---------------------------------------------------------------------------
# Parser
# ---------------------------------------------------------------------------

_DEFAULT_CONSTRAINTS: dict[str, ConstraintSpec] = {
    "r": ConstraintSpec(random=True),
    "p": ConstraintSpec(random=True, positive=True),
    "n": ConstraintSpec(random=True, negative=True),
}


def _parse(tokens: list[str], wrange: float) -> BPNetworkSpec:
    pos = 0  # current token index
    definitions: dict[str, int] = {}
    constraints: dict[str, ConstraintSpec] = dict(_DEFAULT_CONSTRAINTS)
    network_section: list[str] = []   # raw tokens for the network block
    biases_section: list[str] = []    # raw tokens for the biases block

    def peek() -> str | None:
        return tokens[pos] if pos < len(tokens) else None

    def consume() -> str:
        nonlocal pos
        t = tokens[pos]
        pos += 1
        return t

    def consume_int() -> int:
        return int(consume())

    def consume_float() -> float:
        return float(consume())

    while pos < len(tokens):
        tok = consume()
        tl = tok.lower()

        if tl == "definitions:":
            while pos < len(tokens):
                k = consume()
                if k.lower() == "end":
                    break
                # Each definition is "key value"; value may be int or float
                try:
                    val_str = consume()
                except IndexError:
                    break
                key_lc = k.lower()
                try:
                    # Store as int when possible (integers), else as float
                    if "." in val_str:
                        definitions[key_lc] = float(val_str)  # type: ignore[assignment]
                    else:
                        definitions[key_lc] = int(val_str)
                except ValueError:
                    pass  # skip unrecognised value tokens

        elif tl == "constraints:":
            while pos < len(tokens):
                ch = consume()
                if ch.lower() == "end":
                    break
                # Rest of line: keyword flags or a float value
                # The C parser reads a whole line via fgets then sscanf;
                # we collect the remaining tokens on the same logical line
                # by scanning forward until we see the next constraint char,
                # "end", or a section keyword.
                specs = []
                while pos < len(tokens):
                    nxt = tokens[pos]
                    nl = nxt.lower()
                    # Stop when we hit a new single-char token that starts a
                    # new constraint row, or a section keyword, or "end"
                    if nl in ("end",) or nl.endswith(":"):
                        break
                    if len(nxt) == 1 and nxt.isalpha() and pos + 1 < len(tokens):
                        # Peek further to see if the next token is a keyword/float
                        nxt2 = tokens[pos + 1].lower()
                        if nxt2 in ("random", "positive", "negative", "linked") or _is_number(nxt2):
                            break  # nxt is the start of next constraint row
                    specs.append(consume())
                ch_key = ch.lower()
                cs = ConstraintSpec()
                for s in specs:
                    sl = s.lower()
                    if sl == "random":
                        cs.random = True
                    elif sl == "positive":
                        cs.positive = True
                    elif sl == "negative":
                        cs.negative = True
                    elif sl == "linked":
                        cs.linked = True
                    else:
                        try:
                            cs.value = float(s)
                        except ValueError:
                            pass
                constraints[ch_key] = cs

        elif tl == "network:":
            # Collect raw tokens until "end"
            while pos < len(tokens):
                t = consume()
                if t.lower() == "end":
                    break
                network_section.append(t)

        elif tl == "biases:":
            while pos < len(tokens):
                t = consume()
                if t.lower() == "end":
                    break
                biases_section.append(t)

        # "end" at top level — skip
        elif tl == "end":
            pass
        # else skip unknown top-level tokens

    nunits = definitions.get("nunits", 0)
    ninputs = definitions.get("ninputs", 0)
    noutputs = definitions.get("noutputs", 0)
    if not nunits:
        raise ValueError("Missing 'nunits' in network file")

    # Override wrange from definitions if present (stored as float but
    # definitions are read as int; check for a float-ish key too)
    if "wrange" in definitions:
        wrange = float(definitions["wrange"])

    first_weight_to, num_weights_to, wchar = _parse_network_section(
        network_section, nunits
    )
    bchar = _parse_biases_section(biases_section, nunits)

    return BPNetworkSpec(
        nunits=nunits,
        ninputs=ninputs,
        noutputs=noutputs,
        wrange=wrange,
        first_weight_to=first_weight_to,
        num_weights_to=num_weights_to,
        wchar=wchar,
        bchar=bchar,
        constraints=constraints,
        definitions=definitions,
    )


def _is_number(s: str) -> bool:
    try:
        float(s)
        return True
    except ValueError:
        return False


def _parse_network_section(
    tokens: list[str], nunits: int
) -> tuple[list[int], list[int], list[list[str]]]:
    """Parse the ``network:`` block tokens into connectivity arrays.

    Returns (first_weight_to, num_weights_to, wchar).
    """
    # Initialise: all units have no incoming weights by default
    first_weight_to = [nunits] * nunits
    num_weights_to = [0] * nunits
    wchar: list[list[str]] = [[] for _ in range(nunits)]

    pos = 0

    def at() -> str | None:
        return tokens[pos] if pos < len(tokens) else None

    def consume() -> str:
        nonlocal pos
        t = tokens[pos]
        pos += 1
        return t

    rstart = 0
    rend = nunits - 1
    sstart = 0
    send = nunits - 1

    while pos < len(tokens):
        tok = consume()
        if tok[0] == "%":
            # % block header: %<char> rstart rnum sstart snum
            all_ch = tok[1] if len(tok) > 1 else ""
            rstart = int(consume())
            rnum = int(consume())
            sstart = int(consume())
            snum = int(consume())
            rend = rstart + rnum - 1
            send = sstart + snum - 1

            for r in range(rstart, rend + 1):
                if all_ch:
                    row_str = all_ch * snum
                else:
                    row_str = consume()  # one explicit string per receiver
                _set_receiver(r, sstart, snum, row_str, first_weight_to,
                              num_weights_to, wchar)
        else:
            # Bare string (first block, no % header): full-row explicit string
            # Only valid for the very first block and only when block == 0
            # In practice the BP files always use % blocks, but handle it.
            row_str = tok
            # Deduce receiver index from implicit row ordering
            # (rare path — use the current rstart as row counter)
            _set_receiver(rstart, sstart, len(row_str), row_str,
                          first_weight_to, num_weights_to, wchar)
            rstart += 1

    return first_weight_to, num_weights_to, wchar


def _set_receiver(
    r: int, sstart: int, snum: int, row_str: str,
    first_weight_to: list[int],
    num_weights_to: list[int],
    wchar: list[list[str]],
) -> None:
    """Assign connectivity for receiver unit r from row_str."""
    first_weight_to[r] = sstart
    num_weights_to[r] = snum
    wchar[r] = list(row_str[:snum])
    # Pad with '.' if the string is shorter than snum (shouldn't happen)
    while len(wchar[r]) < snum:
        wchar[r].append(".")


def _parse_biases_section(
    tokens: list[str], nunits: int
) -> list[str]:
    """Parse the ``biases:`` block tokens into bchar list (length=nunits)."""
    bchar = ["."] * nunits
    pos = 0

    def consume() -> str:
        nonlocal pos
        t = tokens[pos]
        pos += 1
        return t

    rstart = 0

    while pos < len(tokens):
        tok = consume()
        if tok[0] == "%":
            all_ch = tok[1] if len(tok) > 1 else ""
            rstart = int(consume())
            rnum = int(consume())
            rend = rstart + rnum - 1
            if all_ch:
                bias_str = all_ch * rnum
            else:
                bias_str = consume()
            for j, ch in enumerate(bias_str):
                if rstart + j <= rend:
                    bchar[rstart + j] = ch
        else:
            # Bare string: apply from rstart
            for j, ch in enumerate(tok):
                if rstart + j < nunits:
                    bchar[rstart + j] = ch
            rstart += len(tok)

    return bchar


# ---------------------------------------------------------------------------
# Weight file reader / writer (C format: one float per line)
# ---------------------------------------------------------------------------

def read_weight_file(
    path: str | Path,
    spec: BPNetworkSpec,
    base_dir: str | Path | None = None,
) -> tuple[list[list[float]], list[float]]:
    """Load weights and biases from a C-format weight file.

    Returns (weights, biases) where:
    - weights[i] has length num_weights_to[i]
    - biases has length nunits
    """
    resolved = resolve_compat_path(path, base_dir=base_dir)
    values = [float(v) for v in resolved.read_text().split() if v.strip()]
    idx = 0
    weights: list[list[float]] = []
    for i in range(spec.nunits):
        n = spec.num_weights_to[i]
        weights.append(values[idx: idx + n])
        idx += n
    biases = values[idx: idx + spec.nunits]
    if len(biases) < spec.nunits:
        biases.extend([0.0] * (spec.nunits - len(biases)))
    return weights, biases


def write_weight_file(
    path: str | Path,
    weights: list[list[float]],
    biases: list[float],
    nunits: int,
) -> None:
    """Write weights and biases to a C-format weight file (one float per line)."""
    lines: list[str] = []
    for i in range(nunits):
        for w in weights[i]:
            lines.append(f"{w:f}")
    for b in biases:
        lines.append(f"{b:f}")
    Path(path).write_text("\n".join(lines) + "\n")


# ---------------------------------------------------------------------------
# Random weight initialisation (mirrors reset_weights in bp.c)
# ---------------------------------------------------------------------------

def init_weights_from_spec(
    spec: BPNetworkSpec,
    seed: int | None = None,
) -> tuple[list[list[float]], list[float]]:
    """Draw initial random weights according to the wchar / constraints spec.

    Returns (weights, biases).
    """
    rng = random.Random(seed)
    wrange = spec.wrange

    weights: list[list[float]] = []
    for i in range(spec.nunits):
        row: list[float] = []
        for ch in spec.wchar[i]:
            row.append(_draw(ch, wrange, rng, spec.constraints))
        weights.append(row)

    biases: list[float] = []
    for ch in spec.bchar:
        biases.append(_draw(ch, wrange, rng, spec.constraints))

    return weights, biases


def _draw(ch: str, wrange: float, rng: random.Random,
          constraints: dict[str, ConstraintSpec]) -> float:
    """Draw a single weight value from character code and constraints."""
    if ch == ".":
        return 0.0
    key = ch.lower()
    cs = constraints.get(key)
    if cs is None:
        return 0.0
    if not cs.random:
        return cs.value
    if cs.positive:
        return wrange * rng.random()
    if cs.negative:
        return wrange * (rng.random() - 1.0)
    return wrange * (rng.random() - 0.5)
