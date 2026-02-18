# TODO

## Python port progress

### Completed

- **`iac` model** — fully ported (`pdp-iac`)
  - interactive activation and competition model
  - loads `.NET`, `.TEM`, `.LOO`, supports named patterns via `get unames`
  - commands: `get network`, `get unames`, `set param`, `set dlevel/slevel`, `cycle [n]`, `test <pattern>`, `reset`, `input <unit> <strength>`, `state`/`display`, `quit`
  - auto-displays template state after `cycle`/`test` when `dlevel > 0`
  - combined table renderer: all LOO-sharing entries rendered in one unified grid, column-aligned per x-offset in the template
  - negative values highlighted with ANSI reverse-video (matches C curses standout)

- **`pa` model** — fully ported (`pdp-pa`)
  - pattern associator model
  - loads `.NET`, `.PAT` (input+target pairs), `.TEM`
  - commands: `get network`, `get patterns`, `set nepochs`, `set param`, `set mode`, `strain`, `ptrain`, `tall`, `test <pattern>`, `reset`, `state`/`display`, `quit`
  - delta rule and Hebb learning, linear/threshold-linear/continuous-sigmoid output modes

- **Invocation** — both CLIs match C binary exactly: `cd <model>/ && pdp-<model> TEMPLATE.TEM SCRIPT.STR`
  - script files work with space-separated or one-token-per-line input (mirrors C `fscanf` token-at-a-time)

- **26/26 tests passing**

### Remaining models to port (suggested order)

1. `bp` — backpropagation
2. `cl` — competitive learning
3. `cs` — constraint satisfaction
4. `aa` — auto-associator
5. `ia` — interactive activation (word perception variant)

---

## C version: `iac` terminal corruption on quit

### Problem summary

`iac` runs correctly during interactive use, but quitting (`quit` -> `y`) can leave the parent shell terminal in a broken state (no echo, garbled prompt, or input appearing corrupted).

The Python port (`pdp-iac`) does not have this issue. **The C version is now considered legacy/reference only.**

### What has been tried

1. **Command input / newline handling** — adjusted `readline()` in `COMMAND.C`. Improved responsiveness, did not fix terminal corruption.
2. **Curses shutdown ordering** — explicit mode restoration in `IO.C` (`nocbreak`, `noraw`, `echo`, `nl`, `reset_shell_mode`, `endwin`). Still intermittent.
3. **Display shutdown** — stopped clearing/redrawing screen during `end_display()`. Reduced artifacts, did not eliminate jammed terminal cases.
4. **Explicit quit-path resets** — added `stty sane`, `tput rmcup`, `reset` in `GENERAL.C`. Often recovers terminal, not perfectly reliable.
5. **TTY snapshot/restore** — saved/restored `termios` around curses session via `/dev/tty`. Partial improvement.
6. **External safe launcher scripts** — `scripts/run_iac_safe.sh` and `scripts/run_iac_pty.sh`. Best workaround, still not perfect in all environments.

### Current status

No in-process fix has been 100% reliable. The Python port is the recommended way to run `iac`. For users who must use the C binary, the recovery command is:

```bash
stty sane; tput rmcup 2>/dev/null || true; tput cnorm 2>/dev/null || true; reset
```
