from __future__ import annotations

from dataclasses import dataclass, field

from pdp.io.network_parser import NetworkSpec
from pdp.io.pattern_parser import PatternSet
from pdp.io.template_parser import TemplateSpec


@dataclass
class IACModel:
    maxactiv: float = 1.0
    minactiv: float = -0.2
    estr: float = 0.1
    alpha: float = 0.1
    gamma: float = 0.1
    decay: float = 0.1
    rest: float = -0.1
    ncycles: int = 10
    gb: int = 0

    nunits: int = 0
    cycleno: int = 0
    weights: list[list[float]] = field(default_factory=list)
    unit_names: list[str] = field(default_factory=list)
    pattern_names: list[str] = field(default_factory=list)
    input_patterns: list[list[float]] = field(default_factory=list)
    patno: int = 0
    template: TemplateSpec | None = None

    activation: list[float] = field(default_factory=list)
    netinput: list[float] = field(default_factory=list)
    extinput: list[float] = field(default_factory=list)
    inhibition: list[float] = field(default_factory=list)
    excitation: list[float] = field(default_factory=list)

    def __post_init__(self) -> None:
        self._refresh_decay_cache()

    def _refresh_decay_cache(self) -> None:
        self.dtr = self.decay * self.rest
        self.omd = 1.0 - self.decay

    def set_param(self, name: str, value: float) -> None:
        key = name.lower()
        if key == "max":
            self.maxactiv = value
        elif key == "min":
            self.minactiv = value
        elif key == "estr":
            self.estr = value
        elif key == "alpha":
            self.alpha = value
        elif key == "gamma":
            self.gamma = value
        elif key == "decay":
            self.decay = value
            self._refresh_decay_cache()
        elif key == "rest":
            self.rest = value
            self._refresh_decay_cache()
            if self.activation:
                self.activation = [self.rest for _ in range(self.nunits)]
        elif key == "ncycles":
            self.ncycles = int(value)
        elif key == "gb":
            self.gb = int(value)
        else:
            raise ValueError(f"Unsupported parameter: {name}")

    def load_network(self, network: NetworkSpec) -> None:
        self.nunits = network.nunits
        self.weights = network.weights
        self.reset_state()

    def reset_state(self) -> None:
        if self.nunits <= 0:
            return
        self.cycleno = 0
        self.activation = [self.rest for _ in range(self.nunits)]
        self.netinput = [0.0 for _ in range(self.nunits)]
        self.extinput = [0.0 for _ in range(self.nunits)]
        self.inhibition = [0.0 for _ in range(self.nunits)]
        self.excitation = [0.0 for _ in range(self.nunits)]

    def set_unit_names(self, names: list[str]) -> None:
        if self.nunits <= 0:
            raise ValueError("Load a network before setting unit names")
        if len(names) > self.nunits:
            raise ValueError(f"Too many unit names ({len(names)} > {self.nunits})")
        self.unit_names = names

    def load_patterns(self, pattern_set: PatternSet) -> None:
        if self.nunits <= 0:
            raise ValueError("Load a network before patterns")

        for pattern in pattern_set.inputs:
            if len(pattern) != self.nunits:
                raise ValueError(
                    f"Pattern width {len(pattern)} does not match nunits {self.nunits}"
                )

        self.pattern_names = pattern_set.names
        self.input_patterns = pattern_set.inputs
        self.patno = 0

    def load_template(self, template: TemplateSpec) -> None:
        self.template = template

    def get_pattern_index(self, pattern_ref: str) -> int:
        pattern_ref = pattern_ref.strip()
        if not pattern_ref:
            raise ValueError("Pattern reference cannot be empty")

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

        raise ValueError(f"Invalid pattern reference: {pattern_ref}")

    def set_input_from_pattern(self, pattern_ref: str) -> int:
        if not self.input_patterns:
            raise ValueError("No patterns loaded")

        index = self.get_pattern_index(pattern_ref)
        self.patno = index
        self.extinput = list(self.input_patterns[index])
        return index

    def test_pattern(self, pattern_ref: str, iterations: int | None = None) -> int:
        index = self.set_input_from_pattern(pattern_ref)

        self.cycleno = 0
        for i in range(self.nunits):
            self.excitation[i] = 0.0
            self.inhibition[i] = 0.0
            self.netinput[i] = 0.0
            self.activation[i] = self.rest

        self.cycle(iterations)
        return index

    def set_input(self, unit_name: str, strength: float) -> None:
        if not self.unit_names:
            raise ValueError("No unit names loaded")
        try:
            index = self.unit_names.index(unit_name)
        except ValueError as exc:
            raise ValueError(f"Unknown unit name: {unit_name}") from exc
        self.extinput[index] = strength

    def cycle(self, iterations: int | None = None) -> None:
        if self.nunits <= 0:
            raise ValueError("No network loaded")

        total = self.ncycles if iterations is None else iterations
        for _ in range(total):
            self.cycleno += 1
            self._getnet()
            self._update()

    def _getnet(self) -> None:
        for i in range(self.nunits):
            self.excitation[i] = 0.0
            self.inhibition[i] = 0.0

        for source in range(self.nunits):
            a = self.activation[source]
            if a <= 0.0:
                continue

            for target in range(self.nunits):
                w = self.weights[target][source]
                if w > 0.0:
                    self.excitation[target] += a * w
                elif w < 0.0:
                    self.inhibition[target] += a * w

        for i in range(self.nunits):
            self.excitation[i] *= self.alpha
            self.inhibition[i] *= self.gamma

            if self.gb:
                if self.extinput[i] > 0.0:
                    self.excitation[i] += self.estr * self.extinput[i]
                elif self.extinput[i] < 0.0:
                    self.inhibition[i] += self.estr * self.extinput[i]
            else:
                self.netinput[i] = (
                    self.excitation[i] + self.inhibition[i] + self.estr * self.extinput[i]
                )

    def _update(self) -> None:
        if self.gb:
            for i in range(self.nunits):
                act = self.activation[i]
                act = (
                    self.excitation[i] * (self.maxactiv - act)
                    + self.inhibition[i] * (act - self.minactiv)
                    + self.omd * act
                    + self.dtr
                )
                if act > self.maxactiv:
                    act = self.maxactiv
                elif act < self.minactiv:
                    act = self.minactiv
                self.activation[i] = act
            return

        for i in range(self.nunits):
            act = self.activation[i]
            net = self.netinput[i]
            if net > 0.0:
                act = net * (self.maxactiv - act) + self.omd * act + self.dtr
                if act > self.maxactiv:
                    act = self.maxactiv
            else:
                act = net * (act - self.minactiv) + self.omd * act + self.dtr
                if act < self.minactiv:
                    act = self.minactiv
            self.activation[i] = act
