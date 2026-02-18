# Module Mapping (Legacy C -> Python)

## Core runtime

- `src/main.c` -> `pythonPDP/runtime/app.py` (program entry flow)
- `src/command.c` -> `pythonPDP/runtime/command_parser.py` (menu/command dispatch)
- `src/general.c` -> `pythonPDP/runtime/session.py` (global session state and control)
- `src/io.c`, `src/display.c` -> `pythonPDP/ui/tui.py` (terminal UX abstraction)
- `src/template.c` -> `pythonPDP/io/template_parser.py`
- `src/patterns.c` -> `pythonPDP/io/pattern_parser.py`
- `src/weights.c` -> `pythonPDP/core/weights.py`
- `src/variable.c` -> `pythonPDP/runtime/variables.py`

## Model programs

- `src/bp.c` -> `pythonPDP/models/bp.py`
- `src/aa.c` -> `pythonPDP/models/aa.py`
- `src/cl.c` -> `pythonPDP/models/cl.py`
- `src/cs.c` -> `pythonPDP/models/cs.py`
- `src/ia.c` + `src/iaaux.c` + `src/iatop.c` -> `pythonPDP/models/ia.py`
- `src/iac.c` -> `pythonPDP/models/iac.py`
- `src/pa.c` -> `pythonPDP/models/pa.py`

## Utilities

- `src/plot.c` -> `pythonPDP/tools/plot_cli.py`
- `src/colex.c` -> `pythonPDP/tools/colex_cli.py`

## Data and compatibility layer

- current model directories (`aa/`, `bp/`, `cl/`, `cs/`, `ia/`, `iac/`, `pa/`) remain source-of-truth data roots
- compatibility adapters in `pythonPDP/io/compat_paths.py` should support mixed case file references

## Notes

- Keep parser behavior command-compatible before introducing UX changes.
- Separate computational kernels from command/UI code from the first milestone.
