# TODO: `iac` terminal corruption on quit

## Problem summary

`iac` runs correctly during interactive use, but quitting (`quit` -> `y`) can leave the parent shell terminal in a broken state (no echo, garbled prompt, or input appearing corrupted).

Behavior is intermittent:
- sometimes terminal recovers fully
- sometimes partial recovery
- sometimes remains effectively jammed until manual reset

## What has been tried

### 1) Command input / newline handling in `COMMAND.C`
- Adjusted `readline()` handling of `\n`, `\r`, and CRLF behavior.
- Removed blocking CRLF lookahead that likely caused “double Enter” behavior.
- Removed extra echoed space on Enter that caused visual glitches.

Result:
- improved command responsiveness
- did **not** fully solve terminal corruption after quit

### 2) Curses shutdown ordering in `IO.C`
- Switched shutdown flow to explicit mode restoration (`nocbreak`, `noraw`, `echo`, `nl`, `reset_shell_mode`, `endwin`).
- Attempted idempotent shutdown guards (`io_screen_active`).

Result:
- improved behavior in some runs
- still intermittent terminal breakage after quit

### 3) Display shutdown behavior in `DISPLAY.C`
- Stopped clearing/redrawing the screen during `end_display()` before quitting.

Result:
- reduced screen artifacts
- did **not** eliminate jammed terminal cases

### 4) Explicit quit-path resets in `GENERAL.C`
- Added shell-level recovery commands in quit path:
  - `stty sane`
  - `tput rmcup`
  - `reset`
- Used `/dev/tty` redirections to target controlling terminal.

Result:
- often recovers terminal
- still not perfectly reliable

### 5) TTY state snapshot/restore experiments in `IO.C`
- Saved/restored termios around curses session (`tcgetattr`/`tcsetattr`).
- Used `/dev/tty` fd rather than stdin for restore.

Result:
- partial improvement
- still intermittent failures

### 6) External safe launcher scripts
- Added `scripts/run_iac_safe.sh` to enforce post-exit terminal cleanup.
- Added `scripts/run_iac_pty.sh` to isolate `iac` in separate PTY (`script`/python pty fallback).

Result:
- best workaround so far
- still reported as not perfect in all environments

## Current status

No in-process fix has been 100% reliable for all quit paths and terminal environments.

## Next things to try

1. **Signal-safe cleanup path**
   - add explicit handlers for `SIGTERM`, `SIGHUP`, `SIGABRT`, `SIGSEGV` that attempt terminal restore before exit.

2. **Single canonical runner**
   - force use of a PTY wrapper for all interactive binaries (`aa`, `bp`, `cl`, `cs`, `ia`, `iac`, `pa`) to isolate shell TTY.

3. **ncurses-specific teardown hardening**
   - test `def_prog_mode`/`def_shell_mode` + `reset_shell_mode` workflow in a minimal reproducer.

4. **Environment matrix testing**
   - verify behavior across terminal emulators and shells (bash/zsh, VS Code integrated terminal, tmux, plain tty).

5. **Fallback recommendation**
   - document definitive recovery command for users:
   - `stty sane; tput rmcup 2>/dev/null || true; tput cnorm 2>/dev/null || true; reset`

## User-facing note

At this time, the issue is considered **partially mitigated** but **not fully resolved**.
