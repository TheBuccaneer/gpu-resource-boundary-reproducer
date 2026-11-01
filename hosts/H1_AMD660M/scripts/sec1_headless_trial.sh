#!/usr/bin/env bash
set +e

cd ~/projects/OpenCL2 || exit 1

TRIAL="${1:-}"

if ! [[ "$TRIAL" =~ ^[1-5]$ ]]; then
    echo "Usage: $0 <1-5>"
    exit 1
fi

NN=$(printf '%02d' "$TRIAL")

ROOT="results/sec1/sec1_gui_less_control"
mkdir -p "$ROOT"

BOOT_ID="$(cat /proc/sys/kernel/random/boot_id)"

# Enforce exactly one B3 trigger per boot.
if grep -Rqs "boot_id=$BOOT_ID" "$ROOT"/*/meta.txt 2>/dev/null; then
    echo "ERROR: This boot_id was already used for a headless trial."
    echo "boot_id=$BOOT_ID"
    echo "Reboot before another trigger."
    exit 10
fi

# Refuse to run if graphical stack is active.
if systemctl is-active --quiet display-manager; then
    echo "ERROR: display-manager is active."
    exit 11
fi

GRAPH=$(
    pgrep -a -f \
    '[k]win_wayland|[p]lasmashell|[s]tartplasma|[s]ddm' \
    2>/dev/null
)

if [ -n "$GRAPH" ]; then
    echo "ERROR: graphical processes detected:"
    echo "$GRAPH"
    exit 12
fi

TAG="$(date +%Y%m%d_%H%M%S)"
DIR="${ROOT}/${TAG}_headless_trial${NN}"

mkdir -p "$DIR"

VLOG="$DIR/victim.txt"
ALOG="$DIR/attacker.txt"
META="$DIR/meta.txt"
KLOG="$DIR/kernel.txt"
ENVLOG="$DIR/environment.txt"

START_WALL="$(date '+%Y-%m-%d %H:%M:%S')"

echo "========================================"
echo " SEC1 GUI-LESS TRIAL $NN"
echo "========================================"
echo "Nebenlast: JA"
echo "Keine anderen GPU/OpenCL/CUDA-Programme starten."
echo
echo "Hypothese:"
echo "B3 kann auch ohne KDE/Wayland den Compute-Recovery-Pfad auslösen."
echo
echo "Erwartung:"
echo "Offen: Ring reset oder Full GPU reset."
echo
echo "Weiter-Kriterium:"
echo "GENAU EIN B3-Trigger in diesem Boot."
echo
echo "boot_id=$BOOT_ID"
echo

{
    echo "experiment=SEC1_GUI_LESS_CONTROL"
    echo "trial=$TRIAL"
    echo "boot_id=$BOOT_ID"
    echo "tag=$TAG"
    echo "start_wall=$(date -Is)"
    echo "kernel=$(uname -r)"
    echo "victim=sec1_victim_heartbeat_v2"
    echo "trigger=b3_multi_local_probe_32767_32769"
    echo "condition=multi-user.target_no_display_manager_no_plasma"
} > "$META"

{
    echo "=== target ==="
    systemctl get-default

    echo
    echo "=== display-manager ==="
    systemctl is-active display-manager || true

    echo
    echo "=== graphical-processes ==="
    pgrep -a -f \
      '[k]win_wayland|[p]lasmashell|[s]tartplasma|[s]ddm' \
      || echo "none"

    echo
    echo "=== session ==="
    echo "XDG_SESSION_TYPE=${XDG_SESSION_TYPE:-unset}"

    echo
    echo "=== clinfo ==="
    RUSTICL_ENABLE=radeonsi clinfo 2>/dev/null \
      | grep -m1 -E 'Device Name|Device Type'
} > "$ENVLOG"

RUSTICL_ENABLE=radeonsi \
./probes/sec1_victim_heartbeat_v2 \
    "SEC1_HEADLESS_T${NN}_${TAG}" 200 0 \
    >"$VLOG" 2>&1 &
VPID=$!

echo "victim_pid=$VPID" >> "$META"

sleep 3

python3 -c \
'import time; print("trigger_call_mono_ns=" + str(time.monotonic_ns()))' \
>> "$META"

RUSTICL_ENABLE=radeonsi \
./probes/b3_multi_local_probe 32767 32769 \
    >"$ALOG" 2>&1

ARC=$?

python3 -c \
'import time; print("trigger_return_mono_ns=" + str(time.monotonic_ns()))' \
>> "$META"

echo "attacker_exit=$ARC" >> "$META"

sleep 8

if kill -0 "$VPID" 2>/dev/null; then
    echo "victim_alive=1" >> "$META"
    kill -INT "$VPID" 2>/dev/null
else
    echo "victim_alive=0" >> "$META"
fi

sleep 2

if kill -0 "$VPID" 2>/dev/null; then
    echo "victim_hung_after_sigint=1" >> "$META"
    kill -KILL "$VPID" 2>/dev/null
else
    echo "victim_hung_after_sigint=0" >> "$META"
fi

wait "$VPID"
VRC=$?

echo "victim_exit=$VRC" >> "$META"

journalctl -k --since "$START_WALL" --no-pager > "$KLOG"

if grep -q 'GPU reset begin' "$KLOG"; then
    CLASS="FULL_GPU_RESET"
elif grep -q 'Ring comp_.* reset succeeded' "$KLOG"; then
    CLASS="RING_RESET_ONLY"
else
    CLASS="NO_CLEAR_RESET_CLASS"
fi

echo "recovery_class=$CLASS" >> "$META"

HB=$(grep -a -c '^HB ' "$VLOG" || true)
OK=$(grep -a '^HB ' "$VLOG" | grep -c 'status=OK' || true)
WRONG=$(grep -a -c 'status=WRONG_RESULT' "$VLOG" || true)
WD=$(grep -a -c '^WATCHDOG' "$VLOG" || true)

MAX=$(
    grep -a '^HB ' "$VLOG" \
    | sed -n 's/.*elapsed_ms=\([0-9.]*\).*/\1/p' \
    | sort -nr | head -1
)

sha256sum \
    "$VLOG" "$ALOG" "$META" "$KLOG" "$ENVLOG" \
    > "$DIR/SHA256.txt"

echo
echo "========== SEC1 GUI-LESS TRIAL ${NN} =========="
echo "boot_id=$BOOT_ID"
echo "recovery_class=$CLASS"
echo "attacker_exit=$ARC"
echo "victim_exit=$VRC"
echo "heartbeats=$HB"
echo "ok=$OK"
echo "wrong_result=$WRONG"
echo "watchdog=$WD"
echo "max_elapsed_ms=$MAX"

echo
echo "=== WRONG RESULT ==="
grep -a -E \
'status=WRONG_RESULT|^MISMATCH ' \
"$VLOG" || echo "none"

echo
echo "=== VICTIM EVENTS ==="
grep -a -E \
'^WATCHDOG|REINIT|FATAL|SIGNAL|context is' \
"$VLOG" || echo "none"

echo
echo "=== RECOVERY ==="
grep -Ei \
'ring.*timeout|Process |ring.*reset|GPU reset|MODE2|wedged' \
"$KLOG" | tail -n 100

echo
echo "========================================"
echo "TRIAL $NN COMPLETE."
echo "DO NOT RUN ANOTHER TRIGGER IN THIS BOOT."
echo "For the next trial: sudo reboot"
echo "========================================"
