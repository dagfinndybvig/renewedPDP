#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output_name="${1:-renewedPDP-linux-artifacts.tar.gz}"
output_path="$repo_root/$output_name"

artifacts=(
  "aa/aa"
  "bp/bp"
  "cl/cl"
  "cs/cs"
  "ia/ia"
  "iac/iac"
  "pa/pa"
  "utils/plot"
  "utils/colex"
)

missing=()
for rel in "${artifacts[@]}"; do
  if [[ ! -f "$repo_root/$rel" ]]; then
    missing+=("$rel")
  fi
done

if [[ ${#missing[@]} -gt 0 ]]; then
  echo "Missing build artifacts:" >&2
  for rel in "${missing[@]}"; do
    echo "  - $rel" >&2
  done
  echo "Run: (cd src && make progs && make utils)" >&2
  exit 1
fi

(
  cd "$repo_root"
  tar -czf "$output_path" "${artifacts[@]}"
)

echo "Created: $output_path"
echo "Contents:"
tar -tzf "$output_path"
