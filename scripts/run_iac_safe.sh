#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

restore_terminal() {
  stty sane </dev/tty >/dev/tty 2>/dev/null || true
  tput rmcup </dev/tty >/dev/tty 2>/dev/null || true
  tput cnorm </dev/tty >/dev/tty 2>/dev/null || true
  reset </dev/tty >/dev/tty 2>/dev/null || true
}

trap restore_terminal EXIT INT TERM HUP

if [ "$#" -eq 0 ]; then
  set -- iac/JETS.TEM iac/JETS.STR
fi

"$ROOT_DIR/iac/iac" "$@"
exit $?
