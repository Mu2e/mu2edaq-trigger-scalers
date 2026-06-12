#!/usr/bin/env bash
# Start the Trigger Scalers display (C++ Qt GUI).
# DISPLAY is honored from the environment so the window appears in the
# invoking VNC session. CRS_PORT_UDP / CRS_PORT_ZMQ (exported by the
# control room crs-app launcher) are mapped onto the binary's
# --udp-port / --zmq-port options. A Python discovery responder runs
# as a sidecar until the native C++ mu2edaq-discovery library exists.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PID_FILE="/tmp/trigger-scalers.pid"
DISC_PID_FILE="/tmp/trigger-scalers-responder.pid"

BINARY="$SCRIPT_DIR/build/trigger-scalers"
if [[ ! -x "$BINARY" ]]; then
  echo "ERROR: binary not found at $BINARY — build first:" >&2
  echo "  cmake -B build && cmake --build build" >&2
  exit 1
fi

if [[ -f "$PID_FILE" ]] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
  echo "Trigger Scalers is already running (PID $(cat "$PID_FILE"))."
  exit 0
fi

ARGS=(--config "$SCRIPT_DIR/config/scalars.yaml")
[[ -n "${CRS_PORT_UDP:-}" ]] && ARGS+=(--udp-port "$CRS_PORT_UDP")
[[ -n "${CRS_PORT_ZMQ:-}" ]] && ARGS+=(--zmq-port "$CRS_PORT_ZMQ")

nohup "$BINARY" "${ARGS[@]}" "$@" >> /tmp/trigger-scalers.log 2>&1 &
echo $! > "$PID_FILE"
echo "Trigger Scalers started (PID $(cat "$PID_FILE"))."

# Discovery sidecar (interim until the C++ library exists). No-op when
# mu2edaq-discovery is not installed.
python3 - "${CRS_PORT_UDP:-5555}" "${CRS_PORT_ZMQ:-5556}" >/dev/null 2>&1 <<'PYEOF' &
import signal, sys
try:
    from mu2edaq_discovery import Responder
except ImportError:
    sys.exit(0)
r = Responder(name="Trigger Scalers", app="trigger-scalers",
              port=int(sys.argv[1]), scheme="udp",
              meta={"zmq_port": sys.argv[2]})
r.start()
signal.signal(signal.SIGTERM, lambda *a: sys.exit(0))
signal.pause()
PYEOF
echo $! > "$DISC_PID_FILE"
