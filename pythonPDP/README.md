# pythonPDP

This folder contains all planning and implementation materials for migrating `renewedPDP` from legacy C/curses code to a modern Python codebase.

## Contents

- `MIGRATION_PLAN.md` — full phased migration plan
- `MODULE_MAPPING.md` — legacy-to-new module mapping and ownership boundaries
- `PARITY_CHECKLIST.md` — output/behavior parity criteria and validation matrix

## Initial implementation status

- Python package scaffold is present in `pythonPDP/src/pdp/`
- first active model port is `iac`
- initial CLI entrypoint: `pdp-iac` (`pdp.cli.iac_cli`)
- current parser coverage: legacy `.NET` (network) format
- added parser coverage: legacy `.PAT` pattern format (named input patterns)
- added parser coverage: legacy `.TEM` templates with `look`/`label_look` support via `.LOO`
- `iac` command support now includes `get patterns` and `test <pattern>`
- `iac` command support includes `state` / `display` for template-driven mapped output
- tests live in `pythonPDP/tests/`

## Migration goals

1. Preserve original model behavior and file format compatibility (`.TEM`, `.PAT`, `.NET`, `.STR`, `.LOO`, `.WTS`).
2. Replace brittle terminal handling with modern, reliable I/O.
3. Keep historical datasets and workflows runnable.
4. Enable maintainable future development.

## Near-term recommendation

Begin with a compatibility-first Python implementation of `iac` as the reference model and terminal test case, then generalize shared runtime services across the other programs (`aa`, `bp`, `cl`, `cs`, `ia`, `pa`).

## Quick run

From repository root:

```bash
cd pythonPDP
PYTHONPATH=src python -m pdp.cli.iac_cli iac/JETS.TEM iac/JETS.STR --data-dir ../iac --interactive
```

Run tests:

```bash
cd pythonPDP
PYTHONPATH=src python -m unittest discover -s tests -v
```

Run `iac` smoke check:

```bash
./pythonPDP/scripts/smoke_iac_python.sh
```

Run `iac` pattern/test smoke check:

```bash
./pythonPDP/scripts/smoke_iac_test_python.sh
```

Run `iac` template/state smoke check:

```bash
./pythonPDP/scripts/smoke_iac_template_state_python.sh
```

Run C-vs-Python `iac` parity check:

```bash
./pythonPDP/scripts/parity_iac_c_vs_python.sh
```

This runs multiple `JETS` regimes (default, extinput+activation, and `gb` mode), compares cycle logs generated from legacy `iac` and Python `iac`, and reports max absolute difference per regime.

Run all pythonPDP checks in one command:

```bash
./pythonPDP/scripts/check_all_pythonpdp.sh
```
