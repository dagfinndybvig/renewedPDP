# Python Migration Plan for renewedPDP

## 0. Objective

Migrate the legacy C/curses codebase to a contemporary Python implementation that:

1. preserves model behavior and dataset compatibility,
2. removes fragile terminal handling,
3. improves maintainability, testability, and portability.

## 1. Scope and principles

## In scope

- Re-implement all model programs currently built from `src/`:
  - `aa`, `bp`, `cl`, `cs`, `ia`, `iac`, `pa`
- Re-implement command parsing and command-file execution behavior
- Preserve existing file formats and dataset layout
- Replace curses teardown-sensitive I/O with robust modern terminal handling
- Re-implement `plot` and `colex` tooling where needed

## Out of scope (phase 1)

- redesigning file formats
- changing model semantics
- adding new learning algorithms
- GUI/web front-end

## Success criteria

- All existing smoke scenarios pass in Python
- Numerical outputs match legacy runs within documented tolerances
- Interactive sessions no longer leave terminal in broken state

## 2. Target technical stack

- Python `3.12+`
- Numerical kernel: `numpy`
- CLI/TUI: `prompt_toolkit` (preferred) or `textual` for richer UI
- Parsing: standard library + small focused parser utilities
- Testing: `pytest`
- Packaging: `pyproject.toml` + console entry points

Reasoning: Python enables fast iteration and strong test ergonomics; `prompt_toolkit` gives safe terminal lifecycle management and signal handling.

## 3. Proposed project layout

```text
pythonPDP/
  src/pdp/
    core/                # math and data structures
    io/                  # legacy format parsers
    runtime/             # session state and command dispatch
    models/              # aa, bp, cl, cs, ia, iac, pa implementations
    ui/                  # terminal interface layer
    tools/               # plot/colex equivalents
  tests/
    parity/              # C-vs-Python comparison tests
    smoke/               # scripted startup/load/quit tests
  scripts/
    generate_golden/     # baseline capture from legacy binaries
```

## 4. Migration phases

## Phase 1 — Baseline and instrumentation (1-2 weeks)

Deliverables:

- Define deterministic run harness for legacy binaries (seeded runs)
- Capture baseline outputs (“golden traces”) for representative datasets
- Document tolerated numerical drift per model

Acceptance:

- Repeatable baseline artifacts committed under `pythonPDP/baselines/`

## Phase 2 — Shared runtime foundation (1-2 weeks)

Deliverables:

- Implement command tokenization and menu dispatch semantics
- Implement compatibility file open behavior (case-insensitive fallback)
- Implement session state container replacing global C state

Acceptance:

- Command parser parity tests pass against known command scripts

## Phase 3 — Parser layer (1-2 weeks)

Deliverables:

- `.TEM`, `.PAT`, `.NET` parsers with fixtures from current repo
- Round-trip or structural verification tests

Acceptance:

- All files used by current smoke workflows parse correctly

## Phase 4 — First model port (`iac`) (2-3 weeks)

Deliverables:

- Full `iac` kernel and command integration
- Scripted smoke equivalent for `iac/JETS` startup and command flow
- Numerical parity report vs legacy `iac`

Acceptance:

- `iac` smoke test and parity thresholds pass in CI

## Phase 5 — Remaining model ports (`bp`, `pa`, `cl`, `cs`, `aa`, `ia`) (6-10 weeks)

Deliverables:

- Port each model one by one, sharing runtime/services
- Add per-model smoke and parity tests before moving to next

Suggested order:

1. `bp`
2. `pa`
3. `cl`
4. `cs`
5. `aa`
6. `ia`

Acceptance:

- Each model passes smoke + parity before next model begins

## Phase 6 — Terminal UX hardening (ongoing during phases 4-5; finalize in 1 week)

Deliverables:

- Clean startup/teardown with explicit signal-safe cleanup
- Verified behavior on VS Code terminal + plain shell + tmux
- Unified safe runner for interactive mode

Acceptance:

- No observed terminal corruption in matrix test runs

## Phase 7 — Tooling migration (`plot`, `colex`) (1-2 weeks)

Deliverables:

- Python replacements or wrappers with equivalent CLI behavior
- Tests for representative inputs

Acceptance:

- Utility workflows documented and verified

## Phase 8 — Cutover and deprecation strategy (1 week)

Deliverables:

- Final docs for users and contributors
- Side-by-side command mapping table
- Optional dual-run mode (legacy C and Python both available)

Acceptance:

- Users can run all examples from README through Python CLI

## 5. Validation strategy

## Golden-baseline approach

For each model and representative dataset:

- run legacy binary with scripted command files,
- capture selected traces (loss, activations, key weights),
- compare Python output at checkpoint boundaries.

## Comparison levels

1. strict string parity for command parsing behavior,
2. structural parity for parsed data objects,
3. numerical parity with defined tolerances.

## CI gates

Minimum required before merge:

- parser tests pass,
- smoke tests pass,
- parity tests pass for affected model(s).

## 6. Risks and mitigations

1. **Hidden parser edge cases**
   - Mitigation: fixture corpus from all existing data directories.

2. **Floating-point drift**
   - Mitigation: deterministic seeds, consistent operation ordering, explicit dtype policy.

3. **Behavior encoded in global mutable state**
   - Mitigation: explicit `Session` object and transition-by-transition tests.

4. **Terminal quirks across environments**
   - Mitigation: robust TUI library + matrix testing + fail-safe reset command.

## 7. Milestone schedule (practical estimate)

- Foundation + first model (`iac`): ~6-9 weeks
- Full model coverage + tools: additional ~8-14 weeks
- Total: ~14-23 weeks for high-confidence parity migration

(Estimate assumes one primary engineer with periodic review.)

## 8. Immediate next 10 tasks

1. Create `pythonPDP` Python package skeleton and `pyproject.toml`
2. Add deterministic legacy baseline capture script
3. Capture baseline for `iac/JETS`
4. Implement command tokenizer and parser tests
5. Implement compatibility open helper (case-variant support)
6. Implement `.TEM` parser with tests
7. Implement `.NET` parser with tests
8. Implement `.PAT` parser with tests
9. Port `iac` compute kernel
10. Add `iac` smoke + parity CI job

## 9. Decision log template

Use this section to record migration decisions:

- Date:
- Decision:
- Alternatives considered:
- Rationale:
- Impacted modules:
