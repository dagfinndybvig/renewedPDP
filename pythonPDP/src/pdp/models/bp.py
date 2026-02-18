"""Backpropagation (bp) model — Python port of the legacy C bp program.

Faithfully reproduces the core computation from ``bp.c``:

- Multi-layer feedforward network with arbitrary connectivity
  (specified by first_weight_to / num_weights_to arrays from the .NET file)
- Logistic (sigmoid) activation for all non-input units
- Delta-rule backprop with momentum
- Batch (epoch-grain) and online (pattern-grain) weight updates
- Sequential (strain) and permuted (ptrain) training
- Target clamping: 0 → (1-tmax), 1 → tmax  (default tmax=1.0)
- Early stopping via ecrit (tss threshold)
- Cascade mode: iterative settling before forward pass
- ``get weights`` / ``save weights`` (one float per line)
"""
from __future__ import annotations

import math
import random
from dataclasses import dataclass, field
from pathlib import Path

from pdp.io.bp_network_parser import (
    BPNetworkSpec,
    init_weights_from_spec,
    read_weight_file,
    write_weight_file,
)
from pdp.io.pattern_parser import PatternPairSet


@dataclass
class BPModel:
    # ---- hyper-parameters -----------------------------------------------
    lrate: float = 0.5
    momentum: float = 0.9
    tmax: float = 1.0           # 1.0 → identity target; < 1 constrains range
    ecrit: float = 0.0          # early-stop tss criterion
    nepochs: int = 500
    ncycles: int = 50           # cascade: settle cycles per pattern
    crate: float = 0.05         # cascade: new-input mix rate
    lflag: bool = True          # learning enabled
    cascade: bool = False       # cascade (iterative) mode

    # lgrain: 'epoch' → accumulate wed then update per epoch
    #         'pattern' → update weights after every pattern
    lgrain: str = "epoch"

    # ---- architecture ---------------------------------------------------
    nunits: int = 0
    ninputs: int = 0
    noutputs: int = 0
    # Sparse connectivity (set when network loaded)
    first_weight_to: list[int] = field(default_factory=list)
    num_weights_to: list[int] = field(default_factory=list)

    # ---- state ----------------------------------------------------------
    epochno: int = 0
    patno: int = 0
    cycleno: int = 0
    tss: float = 0.0
    pss: float = 0.0
    gcor: float = 0.0           # gradient correlation (follow mode)

    # ---- arrays (allocated on load_network) ----------------------------
    netinput: list[float] = field(default_factory=list)
    activation: list[float] = field(default_factory=list)
    target: list[float] = field(default_factory=list)
    error: list[float] = field(default_factory=list)
    delta: list[float] = field(default_factory=list)

    # ---- weights (weights[i][j] = weight from unit first_weight_to[i]+j
    #                               to unit i) -------------------------
    weight: list[list[float]] = field(default_factory=list)
    bias: list[float] = field(default_factory=list)

    # ---- weight-error derivatives for current epoch/pattern ----------
    wed: list[list[float]] = field(default_factory=list)
    bed: list[float] = field(default_factory=list)

    # ---- momentum buffers -----------------------------------------------
    dweight: list[list[float]] = field(default_factory=list)
    dbias: list[float] = field(default_factory=list)

    # ---- patterns -------------------------------------------------------
    pattern_names: list[str] = field(default_factory=list)
    input_patterns: list[list[float]] = field(default_factory=list)
    target_patterns: list[list[float]] = field(default_factory=list)
    cpname: str = ""            # current pattern name

    # ---- spec stored for reset -----------------------------------------
    _spec: BPNetworkSpec | None = field(default=None, repr=False)
    _seed: int = field(default_factory=lambda: random.randrange(2**31), repr=False)

    # ==================================================================
    # Loading
    # ==================================================================

    def load_network(self, spec: BPNetworkSpec, seed: int | None = None) -> None:
        """Initialise architecture from a parsed BPNetworkSpec.

        Sets up connectivity arrays and draws random initial weights.
        """
        self._spec = spec
        if seed is not None:
            self._seed = seed

        self.nunits = spec.nunits
        self.ninputs = spec.ninputs
        self.noutputs = spec.noutputs
        self.first_weight_to = list(spec.first_weight_to)
        self.num_weights_to = list(spec.num_weights_to)

        # Pull overrides from the network definitions (nepochs, ecrit, etc.)
        defs = spec.definitions
        if "nepochs" in defs:
            self.nepochs = int(defs["nepochs"])
        if "ecrit" in defs:
            self.ecrit = float(defs["ecrit"])

        self._allocate()
        self.reset_weights()

    def load_patterns(self, pairs: PatternPairSet) -> None:
        if self.nunits <= 0:
            raise ValueError("Load a network before patterns")
        if any(len(p) != self.ninputs for p in pairs.inputs):
            raise ValueError("Pattern input width does not match ninputs")
        if any(len(t) != self.noutputs for t in pairs.targets):
            raise ValueError("Pattern target width does not match noutputs")
        self.pattern_names = list(pairs.names)
        self.input_patterns = [list(p) for p in pairs.inputs]
        self.target_patterns = [list(t) for t in pairs.targets]

    def load_weights(self, path: str | Path,
                     base_dir: str | Path | None = None) -> None:
        """Load saved weights from a C-format weight file."""
        if self._spec is None:
            raise ValueError("Load a network before weights")
        weights, biases = read_weight_file(path, self._spec, base_dir=base_dir)
        self.weight = [list(weights[i]) for i in range(self.nunits)]
        self.bias = list(biases[: self.nunits])
        # Zero gradient buffers on weight load
        self._zero_wed()

    def save_weights(self, path: str | Path) -> None:
        """Save current weights to a C-format weight file."""
        write_weight_file(path, self.weight, self.bias, self.nunits)

    # ==================================================================
    # Allocation helpers
    # ==================================================================

    def _allocate(self) -> None:
        n = self.nunits
        self.netinput = [0.0] * n
        self.activation = [0.0] * n
        self.target = [0.0] * self.noutputs
        self.error = [0.0] * n
        self.delta = [0.0] * n
        self.weight = [[] for _ in range(n)]
        self.bias = [0.0] * n
        self.wed = [[] for _ in range(n)]
        self.bed = [0.0] * n
        self.dweight = [[] for _ in range(n)]
        self.dbias = [0.0] * n

        for i in range(n):
            nw = self.num_weights_to[i]
            self.weight[i] = [0.0] * nw
            self.wed[i] = [0.0] * nw
            self.dweight[i] = [0.0] * nw

    # ==================================================================
    # Reset / re-init
    # ==================================================================

    def reset_weights(self) -> None:
        """Re-draw random weights from spec and zero all state."""
        self.epochno = 0
        self.cycleno = 0
        self.tss = self.pss = self.gcor = 0.0
        self.cpname = ""

        if self._spec is None:
            return

        weights, biases = init_weights_from_spec(self._spec, seed=self._seed)
        for i in range(self.nunits):
            nw = self.num_weights_to[i]
            self.weight[i] = list(weights[i][:nw])
            self.bias[i] = biases[i]

        self._zero_wed()
        for i in range(self.nunits):
            self.netinput[i] = self.activation[i] = self.delta[i] = \
                self.error[i] = 0.0
        for i in range(self.noutputs):
            self.target[i] = 0.0

    def newstart(self) -> None:
        """Pick a new random seed then reset (mirrors ``newstart`` in bp.c)."""
        self._seed = random.randrange(2**31)
        self.reset_weights()

    def _zero_wed(self) -> None:
        for i in range(self.nunits):
            self.wed[i] = [0.0] * self.num_weights_to[i]
            self.bed[i] = 0.0
            self.dweight[i] = [0.0] * self.num_weights_to[i]
            self.dbias[i] = 0.0

    # ==================================================================
    # Forward pass
    # ==================================================================

    @staticmethod
    def _logistic(x: float) -> float:
        if x > 15.935773:
            return 0.99999988
        if x < -15.935773:
            return 0.00000012
        return 1.0 / (1.0 + math.exp(-x))

    def compute_output(self) -> None:
        """Standard feedforward activation (non-cascade mode)."""
        for i in range(self.ninputs, self.nunits):
            net = self.bias[i]
            fwt = self.first_weight_to[i]
            for j, w in enumerate(self.weight[i]):
                net += self.activation[fwt + j] * w
            self.netinput[i] = net
            self.activation[i] = self._logistic(net)

    def init_output(self) -> None:
        """Seed cascade: compute activations ignoring input units."""
        self.cycleno = 0
        for i in range(self.ninputs, self.nunits):
            net = self.bias[i]
            fwt = self.first_weight_to[i]
            nw = self.num_weights_to[i]
            sender = fwt
            for j in range(nw):
                if sender + j >= self.ninputs:  # skip input units
                    net += self.activation[sender + j] * self.weight[i][j]
            self.netinput[i] = net
            self.activation[i] = self._logistic(net)

    def _cascade_cycle(self) -> None:
        """One cascade settling cycle (exponential decay of netinput)."""
        drate = 1.0 - self.crate
        for i in range(self.ninputs, self.nunits):
            newinput = self.bias[i]
            fwt = self.first_weight_to[i]
            for j, w in enumerate(self.weight[i]):
                newinput += self.activation[fwt + j] * w
            self.netinput[i] = self.crate * newinput + drate * self.netinput[i]
            self.activation[i] = self._logistic(self.netinput[i])

    def cycle(self) -> None:
        """Run ncycles cascade cycles."""
        for _ in range(self.ncycles):
            self.cycleno += 1
            self._cascade_cycle()

    # ==================================================================
    # Set input / target
    # ==================================================================

    def setinput(self) -> None:
        for i, v in enumerate(self.input_patterns[self.patno]):
            self.activation[i] = v
        self.cpname = self.pattern_names[self.patno] \
            if self.pattern_names else str(self.patno)

    def settarget(self) -> None:
        """Load target, applying tmax clamping (mirrors settarget in bp.c)."""
        for j, v in enumerate(self.target_patterns[self.patno]):
            if v == 1.0:
                self.target[j] = self.tmax
            elif v == 0.0:
                self.target[j] = 1.0 - self.tmax
            else:
                self.target[j] = v

    def setup_pattern(self) -> None:
        self.setinput()
        self.settarget()

    # ==================================================================
    # Backward pass
    # ==================================================================

    def compute_error(self) -> None:
        """Compute error and backprop deltas.

        Mirrors ``compute_error`` in ``bp.c``:
        - Output units: error[i] = target[j] - activation[i]  (or 0 if
          target < 0, meaning 'don't care')
        - Hidden units: error accumulated from downstream deltas
        - delta[i] = error[i] * act[i] * (1 - act[i])
        """
        # Zero hidden errors
        for i in range(self.ninputs, self.nunits - self.noutputs):
            self.error[i] = 0.0

        # Set output errors
        first_out = self.nunits - self.noutputs
        for j in range(self.noutputs):
            i = first_out + j
            if self.target[j] >= 0:
                self.error[i] = self.target[j] - self.activation[i]
            else:
                self.error[i] = 0.0

        # Backpropagate: walk backwards through non-input units
        for i in range(self.nunits - 1, self.ninputs - 1, -1):
            act_i = self.activation[i]
            del_i = self.delta[i] = self.error[i] * act_i * (1.0 - act_i)
            fwt = self.first_weight_to[i]
            nw = self.num_weights_to[i]
            # No point propagating error back to input units
            if fwt + nw <= self.ninputs:
                continue
            for j, w in enumerate(self.weight[i]):
                sender = fwt + j
                if sender < self.ninputs:
                    continue  # skip input positions
                self.error[sender] += del_i * w

    def compute_wed(self) -> None:
        """Accumulate weight error derivatives (wed += delta[i] * act[sender])."""
        for i in range(self.ninputs, self.nunits):
            self.bed[i] += self.delta[i]
            fwt = self.first_weight_to[i]
            del_i = self.delta[i]
            for j in range(self.num_weights_to[i]):
                self.wed[i][j] += del_i * self.activation[fwt + j]

    def change_weights(self) -> None:
        """Apply accumulated wed to weights with momentum, then zero wed.

        Also handles linked weight constraints via ``_link_sum``
        (averages wed across all members of a link group).  Currently
        links are not tracked in the Python port so ``_link_sum`` is a no-op.
        """
        for i in range(self.ninputs, self.nunits):
            # Bias
            self.dbias[i] = self.lrate * self.bed[i] + self.momentum * self.dbias[i]
            self.bias[i] += self.dbias[i]
            self.bed[i] = 0.0

            # Weights
            for j in range(self.num_weights_to[i]):
                self.dweight[i][j] = (
                    self.lrate * self.wed[i][j]
                    + self.momentum * self.dweight[i][j]
                )
                self.weight[i][j] += self.dweight[i][j]
                self.wed[i][j] = 0.0

    # ==================================================================
    # Stats
    # ==================================================================

    def sumstats(self) -> None:
        """Accumulate pss (per-pattern SSE) into tss."""
        self.pss = 0.0
        first_out = self.nunits - self.noutputs
        for j in range(self.noutputs):
            i = first_out + j
            if self.target[j] >= 0:
                e = self.error[i]
                self.pss += e * e
        self.tss += self.pss

    # ==================================================================
    # Single trial
    # ==================================================================

    def trial(self) -> None:
        """Setup pattern, forward pass, compute error, accumulate stats."""
        self.setup_pattern()
        if self.cascade:
            self.init_output()
            self.cycle()
        else:
            self.compute_output()
        self.compute_error()
        self.sumstats()

    # ==================================================================
    # Training loops
    # ==================================================================

    def _train_loop(self, *, permuted: bool) -> None:
        if not self.input_patterns:
            raise ValueError("No patterns loaded")
        npatterns = len(self.input_patterns)

        # Clear wed at the start of each training run
        self._zero_wed()
        self.cycleno = 0

        for _ in range(self.nepochs):
            self.epochno += 1
            order = list(range(npatterns))
            if permuted:
                random.shuffle(order)

            self.tss = 0.0

            for idx in order:
                self.patno = idx
                self.trial()

                if self.lflag:
                    self.compute_wed()
                    if self.lgrain == "pattern":
                        self.change_weights()

            if self.lflag and self.lgrain == "epoch":
                self.change_weights()

            if self.ecrit > 0.0 and self.tss < self.ecrit:
                break

    def strain(self) -> None:
        """Train for nepochs epochs in sequential pattern order."""
        self._train_loop(permuted=False)

    def ptrain(self) -> None:
        """Train for nepochs epochs in permuted (random) pattern order."""
        self._train_loop(permuted=True)

    def tall(self) -> None:
        """Run one pass over all patterns without weight updates (test-all).

        Mirrors the C ``tall`` function: runs a single sequential epoch
        with learning disabled.
        """
        save_lflag = self.lflag
        save_nepochs = self.nepochs
        self.lflag = False
        self.nepochs = 1
        self._train_loop(permuted=False)
        self.lflag = save_lflag
        self.nepochs = save_nepochs

    # ==================================================================
    # Single pattern test
    # ==================================================================

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

    def test_pattern(self, pattern_ref: str) -> int:
        """Run a single forward pass for the named pattern."""
        if not self.input_patterns:
            raise ValueError("No patterns loaded")
        index = self.get_pattern_index(pattern_ref)
        self.patno = index
        self.tss = 0.0
        self.trial()
        return index

    # ==================================================================
    # Convenience accessors
    # ==================================================================

    @property
    def output_activations(self) -> list[float]:
        """Activation values of the output units."""
        first_out = self.nunits - self.noutputs
        return self.activation[first_out:]
