#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out_file="/tmp/renewedpdp_pa_smoke.out"
cmd_file="$(mktemp)"
trap 'rm -f "$cmd_file"' EXIT

cat > "$cmd_file" << 'EOF'
get
network
JETS.NET
get
patterns
JETS.PAT
quit
y
EOF

cd "$repo_root/pa"
./pa JETS.TEM "$cmd_file" > "$out_file" 2>&1

if grep -q "Unrecognized command" "$out_file"; then
  echo "PA smoke test failed: command parsing error"
  tail -n 40 "$out_file"
  exit 1
fi

echo "PA smoke test passed"
tail -n 5 "$out_file"
