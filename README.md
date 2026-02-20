# renewedPDP
------ WORK IN PROGRESS -------

Note: The modernized C version kinda works, but there has been issues with the terminal, so I'm attempting a port to Python in order to get fully modern I/O.


`renewedPDP` is a preservation and modernisation project for the classic PDP software associated with:

- *Explorations in Parallel Distributed Processing: A Handbook of Models, Programs, and Exercises*
- J. L. McClelland and D. E. Rumelhart

![handbook](https://github.com/user-attachments/assets/756591a1-e7b8-48c4-b69c-25d4f1219791)

Read it here: https://web.stanford.edu/group/pdplab/originalpdphandbook/

The original sources are late-1980s C code with DOS and early Unix assumptions. This repository keeps the historical codebase intact while working towards a clean, modern Python port as the recommended way to run the models.

## C version (legacy / modification)

The original C sources are preserved in `src/` and remain buildable on modern Linux with the right tweaks. Use this if you need to reproduce the exact original terminal behaviour or compare against the Python port. A build for Ubuntu/Linux is included in compressed form.

### Known issue

Terminal recovery after quitting `iac` is improved but not yet fully reliable across all environments. The Python will not have this issue.

Compiling it in your own environment might help.

See [TODO.md](TODO.md) for current status and debugging notes.

## Repository layout

- `src/`: core C sources, headers, and Makefiles
- `aa/`, `bp/`, `cl/`, `cs/`, `ia/`, `iac/`, `pa/`: model data (`.PAT`, `.STR`, `.TEM`, `.NET`, etc.) and C executable output locations
- `utils/`: utility tool outputs (`plot`, `colex`)
- `pythonPDP/`: Python port, tests, and migration planning documents
- `*.ARC`: archived original distribution artifacts

## Build (Ubuntu/Linux)

From repository root:

```bash
cd src
make utils
make progs
```

Outputs:

- core executables: `aa/aa`, `bp/bp`, `cl/cl`, `cs/cs`, `ia/ia`, `iac/iac`, `pa/pa`
- utility executables: `utils/plot`, `utils/colex`

## Portability changes applied to the C sources

1. **Build flags updated** — `CFLAGS` set to `-std=gnu89 -fcommon`; removed `-ltermlib`; retained `-lcurses`
2. **Case-compatibility links** — `linux_compat_links` make target creates lowercase symlinks for `*.C`/`*.H` files
3. **Phony target cleanup** — `.PHONY` entries prevent unintended implicit-rule behaviour
4. **Compile blockers fixed in `GENERAL.H` / `GENERAL.C`** — removed obsolete declarations conflicting with modern libc; added `stdlib.h`, `string.h`
5. **Compile blocker fixed in `COLEX.C`** — renamed variable `inline` → `input_line`; added missing headers
6. **`.gitignore`** — ignores generated objects, archives, symlinks, and built executables

## Note

Clean object/library artifacts:

```bash
cd src
make clean
```

Install curses development headers:

```bash
sudo apt-get update
sudo apt-get install -y libncurses-dev
```

## Terminal recovery (if a TUI session wedges the shell)

```bash
stty sane; tput rmcup 2>/dev/null || true; reset
```

Then press `Ctrl+J` once if needed.

## Smoke tests

From repository root:

```bash
./scripts/smoke_all.sh    # bp + pa C binaries
./scripts/smoke_bp.sh
./scripts/smoke_pa.sh
```

## Packaging build artifacts

```bash
./scripts/package_artifacts.sh
# produces renewedPDP-linux-artifacts.tar.gz
```

To unpack on a target host:

```bash
tar -xzf renewedPDP-linux-artifacts.tar.gz
```

## Additional notes

- The C codebase intentionally remains mostly K&R-style C. Compiler warnings are expected.
- Historical upstream notes are preserved in `src/CHANGES.TXT
- Binaries are dynamically linked; verify dependencies with `ldd aa/aa`.
- For user-supplied filenames the C version attempts exact, uppercase, and lowercase variants in order, which helps with historical uppercase data files on case-sensitive Linux.

# In progress: Python version

A Python port is underway in the `pythonPDP/` folder. It is the **intended** way to run the models — inherently cross platform, no terminal quirks, no curses dependencies. Invocation is similar to the C originals.

### Setup (one time)

From the repository root:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -e pythonPDP
```

This installs the `pdp-iac` and `pdp-pa` commands into your virtual environment.

To activate the venv in future sessions:

```bash
source .venv/bin/activate
```

### Running the models

Change into the model's data directory and invoke the CLI with a template file and an optional command script — exactly as you would with the C binary:

To stay in the interactive REPL after the script finishes, add `--interactive`:

```bash
cd iac/
pdp-iac JETS.TEM JETS.STR --interactive
```

### Command syntax

Commands can be typed one per line or space-separated — the parser reads one token at a time, so these are identical:

```
get network JETS.NET
```
```
get
network
JETS.NET
```

### `pdp-iac` quick command reference

```
get network <file>          load a .NET file
get unames <n1> … end       assign unit names
set dlevel <n>              display level
set slevel <n>              settling level
set param <name> <value>    set a model parameter (max, min, rest, alpha, gamma, decay, estr)
cycle [n]                   run settling cycles
test <pattern>              clamp a named pattern and cycle to settling
reset                       reset activations to resting level
input <unit> <strength>     apply external input to a unit
state / display             print template-formatted network state
quit [y]                    exit
```

### `pdp-pa` quick command reference

```
get network <file>          load a .NET file
get patterns <file>         load input+target pattern pairs from a .PAT file
set nepochs <n>
set param lrate <v>
set param noise <v>
set param temp <v>
set param ecrit <v>
set mode linear <0|1>       linear output mode
set mode lt <0|1>           threshold-linear output mode
set mode cs <0|1>           continuous sigmoid output mode
set mode hebb <0|1>         Hebb learning rule (default: delta rule)
strain                      sequential training for nepochs
ptrain                      permuted (random-order) training for nepochs
tall                        one pass through patterns without weight update
test <pattern>              single forward pass on a named pattern
reset                       zero weights
state / display             print current state
quit [y]                    exit
```

For fuller documentation, setup instructions, tests, and parity scripts see [pythonPDP/README.md](pythonPDP/README.md).

---
