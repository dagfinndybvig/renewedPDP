#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
pythonpdp_dir="$repo_root/pythonPDP"

echo "[1/5] Running unit tests"
(
  cd "$pythonpdp_dir"
  PYTHONPATH=src python -m unittest discover -s tests -v
)

echo "[2/5] Running iac smoke"
"$pythonpdp_dir/scripts/smoke_iac_python.sh"

echo "[3/5] Running iac pattern/test smoke"
"$pythonpdp_dir/scripts/smoke_iac_test_python.sh"

echo "[4/5] Running iac template/state smoke"
"$pythonpdp_dir/scripts/smoke_iac_template_state_python.sh"

echo "[5/5] Running C-vs-Python parity"
"$pythonpdp_dir/scripts/parity_iac_c_vs_python.sh"

echo "All pythonPDP checks passed"
