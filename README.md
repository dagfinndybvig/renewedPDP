# renewedPDP

`renewedPDP` is a preservation and modernisation project for the classic PDP software associated with:

- *Explorations in Parallel Distributed Processing: A Handbook of Models, Programs, and Exercises*
- J. L. McClelland and D. E. Rumelhart

![handbook](https://github.com/user-attachments/assets/756591a1-e7b8-48c4-b69c-25d4f1219791)

Read it here: https://web.stanford.edu/group/pdplab/originalpdphandbook/

The original sources are late-1980s C code with DOS and early Unix assumptions. This repository essentailly keeps the historical codebase intact. The tweaked C version builds and runs on both Ubuntu/Linux and Windows.

C sources are preserved in `src/`. Pre-built binaries are available for both platforms (see **Packaging build artifacts** below).

## Repository layout

- `src/`: core C sources, headers, and Makefiles
- `aa/`, `bp/`, `cl/`, `cs/`, `ia/`, `iac/`, `pa/`: model data files (`.PAT`, `.STR`, `.TEM`, `.NET`, etc.) — executables are **not** stored here; populate by building from source or unpacking a prebuilt ZIP (see below)
- `utils/`: contains `COLEX.EXE` and `PLOT.EXE`, which are **original MS-DOS binaries** (runnable in [DOSBox](https://www.dosbox.com/))
- `COLEX.EXE` (repo root): original MS-DOS binary, same as `utils/COLEX.EXE`
- `pythonPDP/`: Python port, tests, and migration planning documents
- `*.ARC`: archived original DOS distribution (extractable with `ARCE.COM` in DOSBox)

> **Original DOS version:** The original 1980s MS-DOS software is available from the PDP Lab at Stanford: <https://web.stanford.edu/group/pdplab/resources.html>. The `.ARC` files in this repository are those original archives. `utils/COLEX.EXE`, `utils/PLOT.EXE`, and root `COLEX.EXE` are DOS executables from that distribution, preserved here for reference.

## Build (Ubuntu/Linux)

Install curses development headers:

```bash
sudo apt-get update
sudo apt-get install -y libncurses-dev
```

From repository root:

```bash
cd src
make utils
make progs
```

Outputs:

- core executables: `aa/aa`, `bp/bp`, `cl/cl`, `cs/cs`, `ia/ia`, `iac/iac`, `pa/pa`
- utility executables: `utils/plot`, `utils/colex`

Cleanup afterwards:

```bash
cd src
make clean
```

## Build (Windows / MSVC)

Requires **Visual Studio 2022** with the **Desktop development with C++** workload installed.

### Step 1 — Build PDCurses (one time)

PDCurses is not committed to the repository; build it locally from the
[PDCurses source](https://github.com/wmcbrine/PDCurses):

```bat
git clone --depth=1 https://github.com/wmcbrine/PDCurses.git pdcurses
```

Then, from a **Developer Command Prompt for VS 2022** (or after running `vcvars64.bat`):

```bat
cd pdcurses\wincon
nmake /f Makefile.vc
cd ..\..
```

### Step 2 — Build the PDP programs

```bat
cd src
nmake /f Makefile.win
```

Outputs: `aa\aa.exe`, `bp\bp.exe`, `cl\cl.exe`, `cs\cs.exe`, `ia\ia.exe`, `iac\iac.exe`, `pa\pa.exe`

### Running

Change into the model directory and run the executable directly (requires an interactive console — input redirection is not supported by PDCurses):

```bat
cd iac
iac.exe JETS.TEM JETS.STR
```

### Cleaning Windows build artifacts

```bat
cd src
nmake /f Makefile.win clean
```

## Portability changes applied to the C sources

### Linux

1. **Build flags updated** — `CFLAGS` set to `-std=gnu89 -fcommon`; removed `-ltermlib`; retained `-lcurses`
2. **Case-compatibility links** — `linux_compat_links` make target creates lowercase symlinks for `*.C`/`*.H` files
3. **Phony target cleanup** — `.PHONY` entries prevent unintended implicit-rule behaviour
4. **Compile blockers fixed in `GENERAL.H` / `GENERAL.C`** — removed obsolete declarations conflicting with modern libc; added `stdlib.h`, `string.h`
5. **Compile blocker fixed in `COLEX.C`** — renamed variable `inline` → `input_line`; added missing headers
6. **`.gitignore`** — ignores generated objects, archives, symlinks, and built executables

### Windows (MSVC)

All Windows changes are guarded by `#ifdef _WIN32` / `#if !defined(_WIN32)` so the Linux build is completely unaffected.

1. **`src/Makefile.win`** — new `nmake` build file; uses `cl.exe`, `lib.exe`, links PDCurses (wincon) + `user32.lib gdi32.lib advapi32.lib`
2. **`src/GENERAL.H`** — skips `<strings.h>` (POSIX); maps `index → strchr` and `sleep(n) → Sleep(n*1000)` via forward-declared Win32 `Sleep()`
3. **`src/IO.C`** — guards the four `#ifndef MSDOS` blocks around `termios.h`, `unistd.h`, `/dev/tty`, and `tcgetattr/tcsetattr` with an additional `&& !defined(_WIN32)` condition

## Packaging Linux build artifacts

```bash
./scripts/package_artifacts.sh
# produces renewedPDP-linux-artifacts.tar.gz
```

To unpack on a target host (executables are not stored in the repo — unpack after cloning):

```bash
tar -xzf renewedPDP-linux-artifacts.tar.gz
```

## Packaging Windows build artifacts

After building with `nmake /f Makefile.win` (see **Build (Windows / MSVC)** above):

```powershell
powershell -ExecutionPolicy Bypass -File scripts\package_artifacts_win.ps1
# produces renewedPDP-windows-artifacts.zip
```

The ZIP contains `aa\aa.exe`, `bp\bp.exe`, `cl\cl.exe`, `cs\cs.exe`, `ia\ia.exe`, `iac\iac.exe`, `pa\pa.exe`.

### Unpacking on a target host

> **Fresh clone:** executables are not stored in the repository. After cloning, unpack the prebuilt ZIP (or build from source) to populate the model directories.

ZIP is natively supported — no additional software required.

**Windows Explorer:** right-click `renewedPDP-windows-artifacts.zip` → *Extract All…* → set destination to the repository root.

**PowerShell (any version ≥ 5, standard on Windows 10/11):**

```powershell
Expand-Archive -Path renewedPDP-windows-artifacts.zip -DestinationPath . -Force
```

Run that command from the repository root. The executables land directly in their model directories (`aa\aa.exe`, `iac\iac.exe`, etc.).

## Additional notes

- The C codebase intentionally remains mostly K&R-style C. Compiler warnings are expected.
- Historical upstream notes are preserved in `src/CHANGES.TXT
- Binaries are dynamically linked; verify dependencies with `ldd aa/aa`.
- For user-supplied filenames the C version attempts exact, uppercase, and lowercase variants in order, which helps with historical uppercase data files on case-sensitive Linux.

# In progress (back burner): Python version

A Python port exists in the `pythonPDP/` folder but is not actively being developed. The C version is the primary and recommended way to run the models.

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
