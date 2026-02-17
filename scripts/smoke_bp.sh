#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out_file="/tmp/renewedpdp_bp_smoke.out"
cmd_file="$(mktemp)"
trap 'rm -f "$cmd_file"' EXIT

cat > "$cmd_file" << 'EOF'
get
network
424.NET
get
patterns
424.PAT
quit
y
EOF

cd "$repo_root/bp"
./bp 424.TEM "$cmd_file" > "$out_file" 2>&1

if grep -q "Unrecognized command" "$out_file"; then
  echo "BP smoke test failed: command parsing error"
  tail -n 40 "$out_file"
  exit 1
fi

echo "BP smoke test passed"
tail -n 5 "$out_file"
