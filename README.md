# renewedPDP

`renewedPDP` is a preservation/portability copy of the classic PDP software used with:

- *Explorations in Parallel Distributed Processing: A Handbook of Models, Programs, and Exercises*
- J. L. McClelland and D. E. Rumelhart

The original sources are late-1980s C code with DOS and early Unix build assumptions.
This repository keeps that codebase intact while making it practical to build on modern Linux.

## Repository at a glance

- `src/`: C sources, headers, and Makefiles.
- `aa/`, `bp/`, `cl/`, `cs/`, `ia/`, `iac/`, `pa/`: program data files (`.PAT`, `.STR`, `.TEM`, `.NET`, etc.) and output binary location for each executable.
- `utils/`: utility tool outputs (`plot`, `colex`).
- `*.ARC`: archived/original distributions retained in repo.

## What has been done in this branch

The following Linux portability/build updates were applied and verified:

1. **Modernized build flags and libraries in `src/Makefile` and `src/MAKEFILE`**
	- `CFLAGS` set to `-std=gnu89 -fcommon`.
	- `-ltermlib` removed from link flags (not generally available on modern Linux).
	- `-lcurses` retained.

2. **Automatic case-compatibility setup for legacy uppercase filenames**
	- Added `linux_compat_links` make target to create lowercase symlinks for `*.C`/`*.H` so historical lowercase make rules work on case-sensitive filesystems.
	- Wired into `make progs` and `make utils`.

3. **Made command-style targets explicitly phony**
	- Added `.PHONY` list to avoid unintended implicit-rule behavior with names like `plot` and `colex`.

4. **Fixed hard compile blockers in `src/GENERAL.H` and `src/GENERAL.C`**
	- Removed obsolete declarations conflicting with modern libc prototypes (`sprintf`, `malloc`, `realloc`).
	- Added required standard headers (`stdlib.h`, `string.h`).
	- Changed global `in_stream` initialization from `stdin` at file scope to runtime initialization in `init_general()` (portable with modern compilers).

5. **Fixed utility compile blocker in `src/COLEX.C`**
	- Renamed variable `inline` to `input_line` (`inline` is a modern C keyword).
	- Added missing standard headers (`stdlib.h`, `string.h`).

6. **Added build artifact ignores**
	- Added `.gitignore` entries for generated objects, archives, symlinks, and built executables.

## Build (Ubuntu/Linux)

From repository root:

```bash
cd src
make progs
make utils
```

This builds:

- Core executables: `aa/aa`, `bp/bp`, `cl/cl`, `cs/cs`, `ia/ia`, `iac/iac`, `pa/pa`
- Utilities: `utils/plot`, `utils/colex`

## Clean

```bash
cd src
make clean
```

## Dependencies

If curses dev files are missing:

```bash
sudo apt-get update
sudo apt-get install -y libncurses-dev
```

## Notes

- The codebase is intentionally still K&R-era style C in many places.
- You should expect compiler warnings on modern toolchains; build success is the primary target of the portability pass.
- Historical notes from the original software update are preserved in `src/CHANGES.TXT`.
