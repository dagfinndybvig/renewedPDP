# Parity Checklist

Use this checklist to determine whether the Python implementation is behavior-compatible with the existing C programs.

## 1) File format compatibility

- [ ] Load all existing `.TEM` files in each model directory
- [ ] Load all existing `.PAT` files in each model directory
- [ ] Load all existing `.NET` files where applicable
- [ ] Load `.STR`, `.LOO`, `.WTS`, `.PAR`, `.DLI` files used by workflows
- [ ] Support legacy case-variant filenames

## 2) Command compatibility

- [ ] `get/template`
- [ ] `get/network`
- [ ] `get/patterns`
- [ ] training/run commands for each model
- [ ] `quit` path with clean terminal recovery
- [ ] command file mode (`program template command-file`)

## 3) Numerical parity

- [ ] Deterministic seed support and reproducible runs
- [ ] Per-epoch or per-cycle loss/activation traces agree within tolerance
- [ ] Final weights and key metrics agree within tolerance

Suggested tolerances:

- float trace tolerance: `1e-6` to `1e-5` relative, depending on model
- final weight tolerance: `1e-6` absolute (adjust only with justification)

## 4) UX/runtime parity

- [ ] Interactive command loop behaves consistently
- [ ] Error messages preserve intent and guidance
- [ ] No terminal corruption after normal exit
- [ ] No terminal corruption after Ctrl-C or abnormal exit paths

## 5) CI/automation parity

- [ ] Python smoke tests equivalent to `scripts/smoke_bp.sh`
- [ ] Python smoke tests equivalent to `scripts/smoke_pa.sh`
- [ ] Combined smoke command equivalent to `scripts/smoke_all.sh`

## 6) Go/No-go criteria per model

A model is considered migrated when all of the following are true:

- [ ] file loaders pass for included datasets
- [ ] command script execution passes for baseline scenario
- [ ] numerical parity meets tolerance on at least 3 representative datasets
- [ ] terminal behavior is stable in VS Code integrated terminal and a plain shell
