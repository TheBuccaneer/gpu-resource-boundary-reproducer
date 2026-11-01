#!/usr/bin/bash -p
set -eu
exec /usr/bin/python3 -I /home/rock/projects/gpu-resource-boundary-reproducer/hosts/H2_RTX3090/f02/run_case.py --execute-approved-plan
