#!/usr/bin/env bash
set -u

OUT="${1:-results/b1/amdgpu_devcoredump.bin}"
PATH_DATA="/sys/class/drm/card1/device/devcoredump/data"
WAIT_SECONDS="${WAIT_SECONDS:-120}"

echo "watching=$PATH_DATA"
echo "output=$OUT"
echo "wait_seconds=$WAIT_SECONDS"

deadline=$((SECONDS + WAIT_SECONDS))

while [[ ! -e "$PATH_DATA" ]]; do
    if (( SECONDS >= deadline )); then
        echo "capture=NO_DUMP_WITHIN_TIMEOUT"
        exit 124
    fi

    sleep 0.05
done

echo "devcoredump_detected=$(date --iso-8601=seconds)"
echo "capture_start=$(date --iso-8601=seconds)"

tmp="${OUT}.partial"

rm -f "$tmp"

if cat "$PATH_DATA" > "$tmp"; then
    echo "read=SUCCESS"
else
    rc=$?
    echo "read=FAILED exit=$rc"
    exit "$rc"
fi

if [[ ! -s "$tmp" ]]; then
    echo "capture=EMPTY"
    exit 1
fi

mv "$tmp" "$OUT"

echo "capture_end=$(date --iso-8601=seconds)"
echo "capture=SUCCESS"

wc -c "$OUT"
sha256sum "$OUT"
