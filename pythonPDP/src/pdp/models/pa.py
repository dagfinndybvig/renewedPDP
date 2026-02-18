"""Pattern Associator (pa) model — Python port of the legacy C pa program.

Faithfully reproduces the core computation from ``pa.c`` / ``pa.h``:
  - Delta-rule (default) and Hebb learning
  - Output modes: stochastic (default), continuous sigmoid (cs), linear, threshold-linear (lt)
  - Sequential (strain) and permuted (ptrain) training
  - Noise distortion on inputs and targets during training
  - Early stopping via ecrit
"""
from __future__ import annotations

import math
import random
from dataclasses import dataclass, field

from pdp.io.network_parser import NetworkSpec
from pdp.io.pattern_parser import PatternPairSet


@dataclass
class PAModel:
    # ---- hyper-parameters -----------------------------------------------
    lrate: float = 2.0
    noise: float = 0.0
    temp: float = 15.0       # sigmoid temperature
    ecrit: float = 0.0       # early-stop criterion on tss
    nepochs: int = 500
    lflag: bool = True        # learning flag (False → test-only pass)
    linear: bool = False      # linear output mode
    lt: bool = False          # threshold-linear: 1 if net > 0 else 0
    cs_mode: bool = False     # continuous sigmoid (deterministic)
    hebb: bool = False        # Hebb rule instead of delta rule

    # ---- architecture sizes ---------------------------------------------
    ninputs: int = 0
    noutputs: int = 0
    nunits: int = 0           # = ninputs + noutputs

    # ---- state ----------------------------------------------------------
    epochno: int = 0
    patno: int = 0
    tss: float = 0.0          # total sum-of-squares (accumulated across epoch)
    pss: float = 0.0          # per-pattern sum-of-squares
    ndp: float = 0.0          # normalised dot product
    vcor: float = 0.0         # vector correlation (cosine)
    nvl: float = 0.0          # normalised vector length

    # ---- learned parameters  -------------------------------------------
    # weights[out_idx][in_idx] — zero-indexed among output units
    weights: list[list[float]] = field(default_factory=list)
    biases: list[float] = field(default_factory=list)

    # ---- runtime arrays (allocated on load) ----------------------------
    input: list[float] = field(default_factory=list)    # [ninputs]
    output: list[float] = field(default_factory=list)   # [nunits]  (input passthrough + output)
    target: list[float] = field(default_factory=list)   # [noutputs]
    netinput: list[float] = field(default_factory=list) # [nunits]
    error: list[float] = field(default_factory=list)    # [nunits]

    # ---- pattern store --------------------------------------------------
    pattern_names: list[str] = field(default_factory=list)
    input_patterns: list[list[float]] = field(default_factory=list)
    target_patterns: list[list[float]] = field(default_factory=list)

    # ------------------------------------------------------------------
    # Loading
    # ------------------------------------------------------------------

    def load_network(self, network: NetworkSpec) -> None:
        """Initialise architecture and weight arrays from a parsed .NET file."""
        if network.ninputs is None or network.noutputs is None:
            raise ValueError("PA network must define ninputs and noutputs")
        self.nunits = network.nunits
        self.ninputs = network.ninputs
        self.noutputs = network.noutputs
        if self.ninputs + self.noutputs != self.nunits:
            raise ValueError(
                f"nunits ({self.nunits}) must equal ninputs+noutputs "
                f"({self.ninputs}+{self.noutputs})"
            )

        # Extract output-unit weight submatrix [noutputs][ninputs]
        self.weights = [
            list(network.weights[self.ninputs + oi][: self.ninputs])
            for oi in range(self.noutputs)
        ]

        # Biases for output units (default 0)
        if network.biases is not None:
            self.biases = list(network.biases[self.ninputs :])
        else:
            self.biases = [0.0] * self.noutputs

        self.reset_weights()

    def load_patterns(self, pairs: PatternPairSet) -> None:
        if self.nunits <= 0:
            raise ValueError("Load a network before patterns")
        if any(len(inp) != self.ninputs for inp in pairs.inputs):
            raise ValueError("Pattern input width does not match ninputs")
        if any(len(tgt) != self.noutputs for tgt in pairs.targets):
            raise ValueError("Pattern target width does not match noutputs")
        self.pattern_names = list(pairs.names)
        self.input_patterns = [list(p) for p in pairs.inputs]
        self.target_patterns = [list(t) for t in pairs.targets]

    # ------------------------------------------------------------------
    # Reset / state init
    # ------------------------------------------------------------------

    def reset_weights(self) -> None:
        """Zero all learned weights, biases, and state arrays."""
        self.epochno = 0
        self.tss = self.pss = self.ndp = self.vcor = self.nvl = 0.0
        self.patno = 0

        self.weights = [[0.0] * self.ninputs for _ in range(self.noutputs)]
        self.biases = [0.0] * self.noutputs

        self.input = [0.0] * self.ninputs
        self.output = [0.0] * self.nunits
        self.target = [0.0] * self.noutputs
        self.netinput = [0.0] * self.nunits
        self.error = [0.0] * self.nunits

    # ------------------------------------------------------------------
    # Forward pass helpers
    # ------------------------------------------------------------------

    def _logistic(self, x: float) -> float:
        x /= self.temp
        if x > 11.5129:
            return 0.99999
        if x < -11.5129:
            return 0.00001
        return 1.0 / (1.0 + math.exp(-x))

    def _distort(self, pattern: list[float], amount: float) -> list[float]:
        """Add uniform noise ±amount to each element of *pattern*."""
        if amount == 0.0:
            return list(pattern)
        return [v + (1.0 - 2.0 * random.random()) * amount for v in pattern]

    def _set_input(self) -> None:
        """Copy self.input into the input portion of self.output (passthrough)."""
        for i in range(self.ninputs):
            self.output[i] = self.input[i]

    def compute_output(self) -> None:
        for oi in range(self.noutputs):
            ui = self.ninputs + oi
            net = self.biases[oi]
            for j in range(self.ninputs):
                net += self.output[j] * self.weights[oi][j]
            self.netinput[ui] = net

            if self.linear:
                self.output[ui] = net
            elif self.lt:
                self.output[ui] = 1.0 if net > 0.0 else 0.0
            elif self.cs_mode:
                self.output[ui] = self._logistic(net)
            else:  # stochastic (default)
                self.output[ui] = 1.0 if random.random() < self._logistic(net) else 0.0

    def compute_error(self) -> None:
        for oi in range(self.noutputs):
            ui = self.ninputs + oi
            self.error[ui] = self.target[oi] - self.output[ui]

    def _sum_stats(self) -> None:
        out_slice = self.output[self.ninputs :]
        self.pss = _sumsquares(self.target, out_slice, self.noutputs)
        self.vcor = _veccor(self.target, out_slice, self.noutputs)
        self.nvl = _veclen(out_slice, self.noutputs)
        self.ndp = _dotprod(self.target, out_slice, self.noutputs)
        self.tss += self.pss

    def trial(self) -> None:
        self._set_input()
        self.compute_output()
        self.compute_error()
        self._sum_stats()

    def change_weights(self) -> None:
        if self.hebb:
            for oi in range(self.noutputs):
                ui = self.ninputs + oi
                # Hebb: output is clamped to target
                self.output[ui] = self.target[oi]
                for j in range(self.ninputs):
                    self.weights[oi][j] += self.lrate * self.output[ui] * self.output[j]
                self.biases[oi] += self.lrate * self.output[ui]
        else:
            # Delta rule (default)
            for oi in range(self.noutputs):
                ui = self.ninputs + oi
                for j in range(self.ninputs):
                    self.weights[oi][j] += self.lrate * self.error[ui] * self.output[j]
                self.biases[oi] += self.lrate * self.error[ui]

    # ------------------------------------------------------------------
    # High-level training / testing
    # ------------------------------------------------------------------

    def get_pattern_index(self, pattern_ref: str) -> int:
        pattern_ref = pattern_ref.strip()
        try:
            index = int(pattern_ref)
            if 0 <= index < len(self.input_patterns):
                return index
            raise ValueError
        except ValueError:
            lowered = pattern_ref.lower()
            for idx, name in enumerate(self.pattern_names):
                if name.lower().startswith(lowered):
                    return idx
        raise ValueError(f"Invalid pattern reference: {pattern_ref!r}")

    def strain(self) -> None:
        """Train for *nepochs* epochs in sequential pattern order."""
        self._train_loop(permuted=False)

    def ptrain(self) -> None:
        """Train for *nepochs* epochs in permuted (random) pattern order."""
        self._train_loop(permuted=True)

    def tall(self) -> None:
        """Run one pass over all patterns without weight updates (test-all)."""
        save_lflag = self.lflag
        save_nepochs = self.nepochs
        self.lflag = False
        self.nepochs = 1
        self._train_loop(permuted=False)
        self.lflag = save_lflag
        self.nepochs = save_nepochs

    def _train_loop(self, *, permuted: bool) -> None:
        if not self.input_patterns:
            raise ValueError("No patterns loaded")
        npatterns = len(self.input_patterns)

        for _ in range(self.nepochs):
            self.epochno += 1
            order = list(range(npatterns))
            if permuted:
                random.shuffle(order)

            self.tss = 0.0
            for idx in order:
                self.patno = idx
                self.input = self._distort(self.input_patterns[idx], self.noise)
                self.target = self._distort(self.target_patterns[idx], self.noise)
                self.trial()
                if self.lflag:
                    self.change_weights()

            if self.ecrit > 0.0 and self.tss < self.ecrit:
                break

    def test_pattern(self, pattern_ref: str) -> int:
        """Run a single forward pass for the named pattern; return its index."""
        if not self.input_patterns:
            raise ValueError("No patterns loaded")
        index = self.get_pattern_index(pattern_ref)
        self.patno = index
        self.input = list(self.input_patterns[index])
        self.target = list(self.target_patterns[index])
        self.tss = 0.0
        self.trial()
        return index


# ------------------------------------------------------------------
# Pure maths helpers (mirror the C functions in pa.c)
# ------------------------------------------------------------------

def _dotprod(v1: list[float], v2: list[float], n: int) -> float:
    """Normalised dot product: sum(v1*v2) / n."""
    if n == 0:
        return 0.0
    return sum(v1[i] * v2[i] for i in range(n)) / n


def _sumsquares(v1: list[float], v2: list[float], n: int) -> float:
    return sum((v1[i] - v2[i]) ** 2 for i in range(n))


def _veccor(v1: list[float], v2: list[float], n: int) -> float:
    """Vector correlation (cosine of angle between v1 and v2)."""
    dp = sum(v1[i] * v2[i] for i in range(n))
    l1 = sum(v1[i] ** 2 for i in range(n))
    l2 = sum(v2[i] ** 2 for i in range(n))
    if l1 == 0.0 or l2 == 0.0:
        return 0.0
    return dp / math.sqrt(l1 * l2)


def _veclen(v: list[float], n: int) -> float:
    """RMS vector length: sqrt(sum(v^2) / n)."""
    if n == 0:
        return 0.0
    return math.sqrt(sum(x * x for x in v[:n]) / n)
