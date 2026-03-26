# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Real-time trigger scalar monitoring GUI for the Mu2e DAQ (Data Acquisition) system at Fermilab. The application receives broadcast messages (UDP or ZeroMQ) containing a trigger category index and a cumulative count, increments per-category counters, and displays them in a Nixie-tube-inspired dark panel GUI with configurable rate alarms.

`TriggerScalars-Old/` contains the legacy Qt4/SoftRelTools implementation (reference only).
The active codebase is in `src/` with a CMake build.

## Build

```bash
cmake -B build -DWITH_ZEROMQ=ON   # or OFF to skip ZeroMQ
cmake --build build
```

Requires: Qt5 or Qt6, yaml-cpp, libzmq + cppzmq headers (when `WITH_ZEROMQ=ON`).
On macOS with Homebrew, CMake auto-detects the Qt5 keg via `brew --prefix qt@5`. Use `-DUSE_QT6=ON` to build against Qt6 instead.

Run:
```bash
./build/trigger-scalers --config config/scalars.yaml
./build/trigger-scalers -c /path/to/custom.yaml
```

## Wire Protocol

Every message is a 12-byte little-endian struct (see `src/network/TriggerMessage.h`):

| Offset | Type     | Field      | Meaning                              |
|--------|----------|------------|--------------------------------------|
| 0      | uint32_t | `category` | Trigger category index (0-based)     |
| 4      | uint64_t | `count`    | Cumulative trigger count             |

The DAQ broadcasts these packets; the application only listens (no replies).

## Configuration (`config/scalars.yaml`)

```yaml
display:
  size: medium          # small | medium | large | huge  (scales count font)
  update_interval_ms: 1000
  columns: 4            # ScalarWidgets per row

transport:
  protocol: udp         # udp | zeromq
  udp:
    bind_address: "0.0.0.0"
    port: 5555
  zeromq:
    endpoint: "tcp://daq-host:5556"
    socket_type: sub    # sub (PUB/SUB) | pull (PUSH/PULL)

banks:
  - name: "Beam & Timing Triggers"
    triggers:
      - id: 0           # Must match TriggerMessage::category from DAQ
        name: "Mu2e Spill"
        target_rate: 0.6
        visible: true
        alarm:
          enabled: true
          min_rate: 0.1   # Hz; -1 = disabled
          max_rate: 2.0
```

Triggers with `visible: false` are created but hidden; they still process incoming messages.

## Architecture

### Component hierarchy

```
MainWindow  (QMainWindow)
├── QScrollArea
│   └── ScalarBank  (QGroupBox)  — one per "banks" entry in YAML
│       └── ScalarWidget  (QFrame)  — one per trigger
│           ├── name label
│           ├── count label  (amber monospace on black, Nixie-style)
│           └── rate label + alarm dot (●  gray/green/yellow/red)
└── QStatusBar  — message count / connection status / errors
```

### Data flow

1. `UdpReceiver` (event-driven, lives on main thread) or `ZmqReceiver` (QThread subclass running a blocking recv loop) emits `messageReceived(uint32_t category, uint64_t count)`.
2. `MainWindow::handleMessage` forwards to every `ScalarBank::handleMessage`, which dispatches to the matching `ScalarWidget::updateCount` via an `unordered_map<id, ScalarWidget*>`.
3. A single `QTimer` in `MainWindow` fires at `update_interval_ms` and calls `ScalarBank::refreshRates()` → `ScalarWidget::refreshRate()` on every widget.
4. `refreshRate()` computes an instantaneous rate from (Δcount / Δtime) and maintains a rolling average over the last 10 samples.

### Transport implementations

- **`UdpReceiver`** — wraps `QUdpSocket`; no separate thread; reads all pending datagrams on each `readyRead` signal.
- **`ZmqReceiver`** — `QThread` subclass; `run()` sets `ZMQ_RCVTIMEO = 100 ms` so the loop can check `m_stopping` between receives; `stop()` sets `m_stopping` and calls `wait(2000)`.

### Alarm states (ScalarWidget)

Controlled by `TriggerConfig::alarm`. The colored dot `●` shows:
- Gray — alarm disabled
- Green — rate within [min_rate, max_rate]
- Red — rate < min_rate
- Yellow/amber — rate > max_rate

## Test sender tools

Two sender programs live in `tools/` for exercising the display without real DAQ hardware. Both simulate Poisson-distributed trigger firing at the rates defined in `tools/SimulatedTriggers.h` (which mirrors `config/scalars.yaml`).

**`udp-sender`** (Qt-based, uses `QUdpSocket`)
```bash
# Local loopback (default)
./build/udp-sender

# Subnet broadcast
./build/udp-sender --host 255.255.255.255 --port 5555

# Options: --host, --port, --interval-ms, --rate-scale
./build/udp-sender --interval-ms 50 --rate-scale 10   # 10× faster rates
```

**`zmq-sender`** (plain C++, no Qt; requires `WITH_ZEROMQ=ON`)
```bash
# Bind PUB socket — trigger-scalers receiver should use socket_type: sub
./build/zmq-sender

# PUSH variant — trigger-scalers receiver should use socket_type: pull
./build/zmq-sender --socket-type push --endpoint tcp://0.0.0.0:5556

# Options: --endpoint, --socket-type (pub|push), --interval-ms, --rate-scale
./build/zmq-sender --rate-scale 5 --interval-ms 200
```

Both tools print a status line to stdout every second:
```
[  1.0s] pkts=42 | 0:1 1:14 2:10 3:11 4:0 5:1 ...
```
(category_id:cumulative_count for each trigger)

`SimulatedTriggers.h` is shared between both tools and defines the trigger array and `poissonFire()` helper.

## Legacy code reference

`TriggerScalars-Old/` used bit-mask based trigger matching (96-bit masks across three uint32 fields), Qt4, DDS messaging, and the Fermilab SoftRelTools GNUmakefile framework. The new design replaces all of these with category-index messages, Qt5/6, UDP/ZMQ, and CMake.
