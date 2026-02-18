# pythonPDP

Modern Python port of the [renewedPDP](https://github.com/dagfinndybvig/renewedPDP) models, originally from McClelland & Rumelhart's 1987 *Parallel Distributed Processing* explorations software.

## Ported models

| CLI command | Model | Source file |
|-------------|-------|-------------|
| `pdp-iac`   | Interactive Activation and Competition | `src/pdp/cli/iac_cli.py` |
| `pdp-pa`    | Pattern Associator | `src/pdp/cli/pa_cli.py` |

Remaining models (`aa`, `bp`, `cl`, `cs`, `ia`) are planned — see [MIGRATION_PLAN.md](MIGRATION_PLAN.md).

## Setting up the virtual environment

From the repository root (one-time setup):

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -e pythonPDP
```

The `-e` flag installs the package in editable mode so source edits take effect immediately. The `pdp-iac` and `pdp-pa` commands are registered as entry points and become available on your `PATH` once the venv is activated.

To activate the venv in subsequent sessions:

```bash
source .venv/bin/activate
```

## Running the programs

Invocation mirrors the original C binaries exactly. Change into the model's data directory first, then pass the template and command files as positional arguments:

```bash
cd iac/
pdp-iac JETS.TEM JETS.STR          # load template + run script, then exit
pdp-iac JETS.TEM JETS.STR --interactive  # run script, then enter REPL
pdp-iac JETS.TEM                   # load template, enter REPL immediately
```

```bash
cd pa/
pdp-pa JETS.TEM JETS.STR           # load template + run script, then exit
pdp-pa JETS.TEM JETS.STR --interactive   # run script, then enter REPL
pdp-pa JETS.TEM                    # load template, enter REPL immediately
```

All filenames inside `.STR` command files are resolved relative to the directory you run the command from (i.e. the model data directory), matching the C original.

### Command syntax

Commands can be written one per line, space-separated on a single line, or any mix — the parser reads one token at a time, exactly as the C `get_command()` function did with `fscanf`. These are all equivalent:

```
get network JETS.NET
```
```
get
network
JETS.NET
```

### `pdp-iac` commands

```
get network <file>          load a .NET file
get unames <n1> <n2> … end  assign unit names
set dlevel <n>              display level
set slevel <n>              settling level
set param <name> <value>    set a model parameter (max, min, rest, alpha, gamma, decay, estr)
cycle [n]                   run n settling cycles (default from template)
test <pattern>              clamp a named pattern and cycle
reset                       reset activations to resting level
input <unit> <strength>     set external input on a unit
state / display             print current template-formatted state
quit [y]                    exit
```

### `pdp-pa` commands

```
get network <file>          load a .NET file
get patterns <file>         load input+target pattern pairs from a .PAT file
set nepochs <n>
set lflag <0|1>
set param lrate <v>
set param noise <v>
set param temp <v>
set param ecrit <v>
set mode linear <0|1>       linear output
set mode lt <0|1>           threshold-linear output
set mode cs <0|1>           continuous sigmoid
set mode hebb <0|1>         Hebb learning rule (default: delta rule)
strain                      sequential training for nepochs
ptrain                      permuted (random-order) training for nepochs
tall                        one pass through patterns without weight update
test <pattern>              single forward pass on a named pattern
reset                       zero weights
state / display             print current state
quit [y]                    exit
```

## Running the tests

```bash
cd pythonPDP
python -m pytest tests/ -v
```

## Smoke tests and parity checks

These scripts live in `pythonPDP/scripts/` and are meant to be run from the repository root.

| Script | Purpose |
|--------|---------|
| `scripts/smoke_iac_python.sh` | Basic `iac` load + cycle smoke check |
| `scripts/smoke_iac_test_python.sh` | `iac` pattern clamping smoke check |
| `scripts/smoke_iac_template_state_python.sh` | `iac` template/state display smoke check |
| `scripts/parity_iac_c_vs_python.sh` | Compare cycle logs between C and Python `iac` across multiple JETS regimes |
| `scripts/check_all_pythonpdp.sh` | Run all of the above in sequence |

## Planning and mapping documents

- [MIGRATION_PLAN.md](MIGRATION_PLAN.md) — phased migration plan
- [MODULE_MAPPING.md](MODULE_MAPPING.md) — legacy C → Python module mapping
- [PARITY_CHECKLIST.md](PARITY_CHECKLIST.md) — output/behavior parity criteria
