#!/usr/bin/env bash
#
# start-mu2edaq-trigger-scalers.sh - standardized Mu2e control-room start
# script. Launched as `crs-app start trigger-scalers`, which exports
# CRS_PORT_UDP and CRS_PORT_ZMQ from apps.yaml. These are forwarded to the
# trigger-scalers binary as --udp-port / --zmq-port (which override the config).
# Builds the binary with CMake if it is not present, then runs it in the
# background within the VNC session.
#
# The control room assigns UDP 5557 (5555 is taken by the dashboard's ZMQ).
# Port precedence: CRS_PORT_* env > config file > built-in default.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

export CRS_PORT_UDP="${CRS_PORT_UDP:-5557}"   # UDP bind port
export CRS_PORT_ZMQ="${CRS_PORT_ZMQ:-5556}"   # ZeroMQ endpoint port
export DISPLAY="${DISPLAY:-:0}"
CONFIG_FILE="${1:-config/scalars.yaml}"
BIN="$SCRIPT_DIR/build/trigger-scalers"
PID_FILE="$SCRIPT_DIR/trigger-scalers.pid"
LOG_FILE="$SCRIPT_DIR/trigger-scalers.log"

if [[ ! -x "$BIN" ]]; then
  echo "Binary not found at $BIN; building with CMake..."
  cmake -S "$SCRIPT_DIR" -B "$SCRIPT_DIR/build"
  cmake --build "$SCRIPT_DIR/build" --target trigger-scalers
fi

if [[ -f "$PID_FILE" ]] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
  echo "Trigger Scalers already running (PID $(cat "$PID_FILE"))."
  exit 0
fi
rm -f "$PID_FILE"

echo "Starting Trigger Scalers (udp=$CRS_PORT_UDP zmq=$CRS_PORT_ZMQ, config: $CONFIG_FILE)"
nohup "$BIN" --config "$CONFIG_FILE" \
  --udp-port "$CRS_PORT_UDP" --zmq-port "$CRS_PORT_ZMQ" >> "$LOG_FILE" 2>&1 &
bgpid=$!
sleep 1
if ! kill -0 "$bgpid" 2>/dev/null; then
  echo "error: Trigger Scalers failed to start; see $LOG_FILE" >&2
  exit 1
fi
echo "$bgpid" > "$PID_FILE"
echo "Trigger Scalers started (PID $bgpid); log: $LOG_FILE"
