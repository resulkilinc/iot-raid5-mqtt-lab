#!/usr/bin/env bash
# Poll /proc/mdstat; write status; alert when degraded.
set -euo pipefail
OUT_DIR="${MONITOR_DIR:-./monitor}"
mkdir -p "$OUT_DIR"
TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
STAT="$OUT_DIR/status_latest.txt"
HIST="$OUT_DIR/history.csv"
ALERT="$OUT_DIR/alerts.log"

mdstat="$(cat /proc/mdstat 2>/dev/null || true)"
state="unknown"
if echo "$mdstat" | grep -q '\[UUUU\]'; then state="ok"
elif echo "$mdstat" | grep -Eq '\[U+_\]|recovery|degraded'; then state="degraded"
elif echo "$mdstat" | grep -q 'md[0-9]'; then state="ok-or-other"
fi

{
  echo "ts=$TS"
  echo "state=$state"
  echo "--- mdstat ---"
  echo "$mdstat"
} > "$STAT"

if [[ ! -f "$HIST" ]]; then echo "ts,state" > "$HIST"; fi
echo "$TS,$state" >> "$HIST"

if [[ "$state" == "degraded" ]]; then
  echo "$TS ALERT RAID degraded" | tee -a "$ALERT"
  exit 2
fi
echo "$TS OK"
