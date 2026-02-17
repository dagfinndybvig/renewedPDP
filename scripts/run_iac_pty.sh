#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

restore_terminal() {
  stty sane </dev/tty >/dev/tty 2>/dev/null || true
  tput rmcup </dev/tty >/dev/tty 2>/dev/null || true
  tput cnorm </dev/tty >/dev/tty 2>/dev/null || true
  reset </dev/tty >/dev/tty 2>/dev/null || true
}

if [ "$#" -eq 0 ]; then
  set -- iac/JETS.TEM iac/JETS.STR
fi

run_under_script() {
  local cmd=""
  local part
  local part_q
  for part in "$@"; do
    printf -v part_q '%q' "$part"
    cmd+="$part_q "
  done
  script -qfec "$cmd" /dev/null
}

run_under_python_pty() {
  python3 -c 'import os, pty, sys
status = pty.spawn(sys.argv[1:])
try:
    code = os.waitstatus_to_exitcode(status)
except Exception:
    code = status if isinstance(status, int) else 0
raise SystemExit(code)
' "$@"
}

if command -v script >/dev/null 2>&1; then
  run_under_script "$ROOT_DIR/iac/iac" "$@"
  code=$?
elif command -v python3 >/dev/null 2>&1; then
  run_under_python_pty "$ROOT_DIR/iac/iac" "$@"
  code=$?
else
  "$ROOT_DIR/iac/iac" "$@"
  code=$?
fi

restore_terminal
exit $code
