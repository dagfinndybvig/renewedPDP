# renewedPDP

`renewedPDP` is a preservation/portability copy of the classic PDP software associated with:

- *Explorations in Parallel Distributed Processing: A Handbook of Models, Programs, and Exercises*
- J. L. McClelland and D. E. Rumelhart

The original sources are late-1980s C code with DOS and early Unix assumptions. This repository keeps the historical codebase intact while making it practical to build and run on modern Linux.

## Repository Layout

- `src/`: core C sources, headers, and Makefiles
- `aa/`, `bp/`, `cl/`, `cs/`, `ia/`, `iac/`, `pa/`: model data (`.PAT`, `.STR`, `.TEM`, `.NET`, etc.) and executable output locations
- `utils/`: utility tool outputs (`plot`, `colex`)
- `*.ARC`: archived/original distribution artifacts preserved in repo

## Portability Work Completed

The following Linux portability/build updates were applied and verified in this branch.

1. **Build flags and linker inputs updated in `src/Makefile` and `src/MAKEFILE`**
   - `CFLAGS` set to `-std=gnu89 -fcommon`
   - removed `-ltermlib` (typically unavailable on modern Linux)
   - retained `-lcurses`

2. **Automatic case-compatibility for legacy uppercase source names**
   - added `linux_compat_links` target to create lowercase symlinks for `*.C`/`*.H`
   - integrated into `make progs` and `make utils`

3. **Phony target cleanup**
   - added `.PHONY` entries to prevent unintended implicit-rule behavior for names such as `plot` and `colex`

4. **Hard compile blockers fixed in `src/GENERAL.H` and `src/GENERAL.C`**
   - removed obsolete declarations conflicting with modern libc (`sprintf`, `malloc`, `realloc`)
   - added required standard headers (`stdlib.h`, `string.h`)
   - moved global `in_stream` initialization from file scope (`stdin`) to runtime in `init_general()`

5. **Hard compile blocker fixed in `src/COLEX.C`**
   - renamed variable `inline` to `input_line` (`inline` is a C keyword in modern compilers)
   - added missing standard headers (`stdlib.h`, `string.h`)

6. **Build artifact ignores added**
   - added `.gitignore` entries for generated objects, archives, symlinks, and built executables

## Build (Ubuntu/Linux)

From repository root:

```bash
cd src
make progs
make utils
```

Outputs:

- core executables: `aa/aa`, `bp/bp`, `cl/cl`, `cs/cs`, `ia/ia`, `iac/iac`, `pa/pa`
- utility executables: `utils/plot`, `utils/colex`

## Quick Start

Run each model from its own directory so template and data files resolve naturally:

```bash
cd aa && ./aa
cd ../bp && ./bp
cd ../cl && ./cl
cd ../cs && ./cs
cd ../ia && ./ia
cd ../iac && ./iac
cd ../pa && ./pa
```

Common first commands in interactive sessions:

- `get/template` (load a `.TEM` file)
- `get/network` (load a `.NET` file, where applicable)
- `get/patterns` (load a `.PAT` file)
- training/run commands for the selected program

Run utility tools:

```bash
./utils/plot
./utils/colex
```

### Interactive Example (`bp`)

This example uses files already present in `bp/`: `424.TEM`, `424.NET`, `424.PAT`.

```bash
cd bp
./bp 424.TEM
```

At the prompt:

1. Enter `get` (opens the `get/` submenu)
2. Enter `network`, then provide `424.NET`
3. Enter `patterns`, then provide `424.PAT`
4. Press `Enter` on a blank line to leave the submenu
5. Enter `ptrain`
6. Enter `quit`, then `y`

Other built-in `bp` dataset families include `REC.*`, `SEQ.*`, and `XOR.*`.

### Batch Example (`bp`)

You can script a session by passing a command file as the second CLI argument:

```bash
cd bp
cat > example.com << 'EOF'
get
network 424.NET
patterns 424.PAT

ptrain
quit y
EOF

./bp 424.TEM example.com
```

Notes:

- first argument: template file (`.TEM`)
- second argument: command script consumed by the same parser used interactively
- `quit y` ends the run cleanly

### Batch Example (`pa`)

`pa/` includes a matched `JETS` set: `JETS.TEM`, `JETS.NET`, `JETS.PAT`.

```bash
cd pa
cat > example.com << 'EOF'
get
network JETS.NET
patterns JETS.PAT

ptrain
quit y
EOF

./pa JETS.TEM example.com
```

## Maintenance

Clean object/library artifacts:

```bash
cd src
make clean
```

Install dependencies if curses development files are missing:

```bash
sudo apt-get update
sudo apt-get install -y libncurses-dev
```

## Smoke Tests

Checked-in smoke scripts validate that `bp` and `pa` can start, load real dataset files, and exit cleanly in scripted mode.

From repository root:

```bash
./scripts/smoke_all.sh
```

You can also run them individually:

```bash
./scripts/smoke_bp.sh
./scripts/smoke_pa.sh
```

## Packaging Build Artifacts

Create a shareable archive of Linux build outputs:

```bash
./scripts/package_artifacts.sh
```

This produces `renewedPDP-linux-artifacts.tar.gz` in the repository root and prints the archive contents.

Optional custom output filename:

```bash
./scripts/package_artifacts.sh my-artifacts.tar.gz
```

## Additional Notes

- The codebase intentionally remains mostly K&R-style C.
- Compiler warnings are expected on modern toolchains; the portability target here is successful build and execution.
- Historical upstream notes are preserved in `src/CHANGES.TXT`.
