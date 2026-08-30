#!/usr/bin/env bash
set +e

cd ~/projects/OpenCL2 || exit 1

echo "========================================"
echo " SEC1 GUI-LESS PREFLIGHT"
echo "========================================"
echo "Nebenlast: NEIN"
echo "Noch kein Fehlertrigger."
echo

echo "=== DEFAULT TARGET ==="
systemctl get-default

echo
echo "=== DISPLAY MANAGER ==="
systemctl is-active display-manager || true

echo
echo "=== GRAPHICAL PROCESSES ==="
GRAPH=$(
    pgrep -a -f \
    '[k]win_wayland|[p]lasmashell|[s]tartplasma|[s]ddm' \
    2>/dev/null
)

if [ -n "$GRAPH" ]; then
    echo "$GRAPH"
    echo
    echo "ERROR: graphical processes still active."
    exit 2
else
    echo "none"
fi

echo
echo "=== SESSION ==="
echo "XDG_SESSION_TYPE=${XDG_SESSION_TYPE:-unset}"

echo
echo "=== OPENCL DEVICE ==="
RUSTICL_ENABLE=radeonsi clinfo 2>/dev/null \
    | grep -m1 -E 'Device Name|Device Type'

CLRC=${PIPESTATUS[0]}

if [ "$CLRC" -ne 0 ]; then
    echo
    echo "ERROR: clinfo failed."
    exit 3
fi

echo
echo "PREFLIGHT_OK"
