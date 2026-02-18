#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
out_file="/tmp/pythonpdp_iac_smoke.out"

cd "$repo_root/pythonPDP"
PYTHONPATH=src python -m pdp.cli.iac_cli \
  ../iac/JETS.TEM \
  ../iac/JETS.STR \
  --data-dir ../iac \
  > "$out_file" 2>&1

if ! grep -q "network loaded: nunits=68" "$out_file"; then
  echo "pythonPDP iac smoke failed: network load marker missing"
  cat "$out_file"
  exit 1
fi

if ! grep -q "unit names loaded: 68" "$out_file"; then
  echo "pythonPDP iac smoke failed: unit name marker missing"
  cat "$out_file"
  exit 1
fi

echo "pythonPDP iac smoke test passed"
tail -n 5 "$out_file"
