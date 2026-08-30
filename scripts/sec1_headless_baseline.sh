#!/usr/bin/env bash
set +e

cd ~/projects/OpenCL2 || exit 1

ROOT="results/sec1/sec1_gui_less_control"
mkdir -p "$ROOT"

TAG="$(date +%Y%m%d_%H%M%S)"
LOG="$ROOT/headless_baseline_${TAG}.txt"

echo "========================================"
echo " SEC1 GUI-LESS V2 BASELINE"
echo "========================================"
echo "Nebenlast: JA"
echo "Keine anderen GPU/OpenCL/CUDA-Programme starten."
echo
echo "Hypothese:"
echo "Der harmlose V2-Victim läuft ohne KDE/Wayland stabil."
echo
echo "Erwartung:"
echo "Alle Heartbeats korrekt, keine Fehler."
echo
echo "Weiter-Kriterium:"
echo "Baseline sauber => Headless-B3-Trials erlaubt."
echo

RUSTICL_ENABLE=radeonsi \
./probes/sec1_victim_heartbeat_v2 \
    "SEC1_HEADLESS_BASELINE_${TAG}" 200 0 \
    >"$LOG" 2>&1 &
PID=$!

sleep 15

kill -INT "$PID" 2>/dev/null
wait "$PID"
RC=$?

HB=$(grep -a -c '^HB ' "$LOG" || true)
OK=$(grep -a '^HB ' "$LOG" | grep -c 'status=OK' || true)
WRONG=$(grep -a -c 'status=WRONG_RESULT' "$LOG" || true)
WD=$(grep -a -c '^WATCHDOG' "$LOG" || true)

MAX=$(
    grep -a '^HB ' "$LOG" \
    | sed -n 's/.*elapsed_ms=\([0-9.]*\).*/\1/p' \
    | sort -nr | head -1
)

echo
echo "========== HEADLESS BASELINE =========="
echo "exit=$RC"
echo "heartbeats=$HB"
echo "ok=$OK"
echo "wrong_result=$WRONG"
echo "watchdog=$WD"
echo "max_elapsed_ms=$MAX"
echo "log=$LOG"

echo
echo "=== BAD EVENTS ==="
grep -a -E \
'WRONG_RESULT|MISMATCH|WATCHDOG|FATAL|context is' \
"$LOG" || echo "none"
