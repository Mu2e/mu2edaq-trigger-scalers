#!/usr/bin/env bash
# Stop the Trigger Scalers display and its discovery sidecar.
set -uo pipefail

PID_FILE="/tmp/trigger-scalers.pid"
DISC_PID_FILE="/tmp/trigger-scalers-responder.pid"

# Sidecar first, so the app disappears from discovery before it exits.
if [[ -f "$DISC_PID_FILE" ]]; then
  kill "$(cat "$DISC_PID_FILE")" 2>/dev/null || true
  rm -f "$DISC_PID_FILE"
fi

if [[ -f "$PID_FILE" ]] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
  kill "$(cat "$PID_FILE")"
  rm -f "$PID_FILE"
  echo "Trigger Scalers stopped."
else
  PIDS=$(pgrep -x "trigger-scalers" || true)
  if [[ -n "$PIDS" ]]; then
    kill $PIDS 2>/dev/null || true
    echo "Trigger Scalers stopped (found by name)."
  else
    echo "Trigger Scalers is not running."
  fi
  rm -f "$PID_FILE" 2>/dev/null || true
fi
exit 0
