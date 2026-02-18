#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
out_file="/tmp/pythonpdp_iac_test_smoke.out"
cmd_file="$(mktemp)"
pat_file="$(mktemp)"
trap 'rm -f "$cmd_file" "$pat_file"' EXIT

{
  printf "jets"
  for _ in $(seq 1 68); do
    printf " +"
  done
  printf "\n"
} > "$pat_file"

cat > "$cmd_file" << EOF
get network JETS.NET
get unames Jets Sharks in20s in30s in40s JH HS College Single Married Divorced Pusher Burglar Bookie Art Al Sam Clyde Mike Jim Greg John Doug Lance George Pete Fred Gene Ralph Phil Ike Nick Don Ned Karl Ken Earl Rick Ol Neal Dave _Art _Al _Sam _Clyde _Mike _Jim _Greg _John _Doug _Lance _George _Pete _Fred _Gene _Ralph _Phil _Ike _Nick _Don _Ned _Karl _Ken _Earl _Rick _Ol _Neal _Dave end
get patterns $pat_file
test jets
quit y
EOF

cd "$repo_root/pythonPDP"
PYTHONPATH=src python -m pdp.cli.iac_cli \
  ../iac/JETS.TEM \
  "$cmd_file" \
  --data-dir ../iac \
  > "$out_file" 2>&1

if ! grep -q "patterns loaded: 1" "$out_file"; then
  echo "pythonPDP iac test smoke failed: pattern load marker missing"
  cat "$out_file"
  exit 1
fi

if ! grep -q "test complete: pattern=jets" "$out_file"; then
  echo "pythonPDP iac test smoke failed: test command marker missing"
  cat "$out_file"
  exit 1
fi

echo "pythonPDP iac pattern/test smoke passed"
tail -n 8 "$out_file"
