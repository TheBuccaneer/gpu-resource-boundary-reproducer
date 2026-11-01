#!/usr/bin/env bash
# One manually selected case. No loop, no approval editing or automatic retry.
set -euo pipefail
case "${1-}" in
  S01-OWN-API|S01-INDEX-API|S01-SIZE-API|S01-FOREIGN-API|S01-CLONE-API) ;;
  *) echo 'STOP: select exactly one of the five API-only cases.' >&2; exit 64 ;;
esac
cd /home/rock/projects/gpu-resource-boundary-reproducer
python3 hosts/H2_RTX3090/s01/pilot/check_api_pilot.py --case "$1"
# The supervisor rejects the pending manifest before ledger reservation/childstart.
exec python3 hosts/H2_RTX3090/c1/reproducers/run_approved_case.py \
  --execute-approved-plan \
  --approval-note 'S01 API-only pilot; actual approval evidence must be in manifest' \
  --approval-manifest hosts/H2_RTX3090/c1/approvals/s01_api_pilot_20261031.json \
  --binary /home/rock/projects/gpu-resource-boundary-reproducer/hosts/H2_RTX3090/s01/pilot/h2_s01_api_pilot \
  --case "$1" --repeat-number 1 \
  --out "/home/rock/projects/gpu-resource-boundary-reproducer/hosts/H2_RTX3090/results/c1/s01_api_pilot_20261031/$1"
