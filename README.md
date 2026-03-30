# trigger-scalers

Real-time trigger scalar monitoring GUI for the [Mu2e](https://mu2e.fnal.gov) DAQ system at Fermilab.

The application receives broadcast messages (UDP or ZeroMQ) containing a trigger category index and an increment value, accumulates per-category counts, and displays them in a Nixie-tube-inspired dark panel GUI with configurable rate alarms.

![Color schemes: Amber, Green Phosphor, Blue, Red, Light](docs/screenshot-placeholder.png)

---

## Features

- Live trigger count and rolling-average rate display across multiple configurable banks
- Rate alarm indicators (green / red / yellow) with per-trigger thresholds
- Five switchable color schemes: **Amber**, **Green Phosphor**, **Blue**, **Red**, **Light**
- Per-scaler enable/disable toggle and count reset
- Global enable all / disable all / reset all from the Edit menu
- Copy all counts and rates to clipboard as CSV (with timestamp)
- Save current configuration (including active color scheme) back to YAML
- UDP broadcast and ZeroMQ PUB/SUB and PUSH/PULL transports
- Command-line overrides for all major config settings
- Two test sender tools for exercising the display without real DAQ hardware

---

## Requirements

| Dependency | Version | Notes |
|---|---|---|
| Qt | 5.x or 6.x | Core, Widgets, Network modules |
| yaml-cpp | any | Configuration parsing and saving |
| libzmq + cppzmq | any | Only required with `-DWITH_ZEROMQ=ON` |
| CMake | ≥ 3.16 | |
| C++ compiler | C++17 | |

On macOS with Homebrew:

```bash
brew install qt@5 yaml-cpp zeromq
```

On Linux (Debian/Ubuntu):

```bash
sudo apt install qt5-default libyaml-cpp-dev libzmq3-dev
```

---

## Build

```bash
cmake -B build -DWITH_ZEROMQ=ON
cmake --build build
```

### CMake Options

| Option | Default | Description |
|---|---|---|
| `WITH_ZEROMQ` | `ON` | Build ZeroMQ receiver and `zmq-sender` tool |
| `USE_QT6` | `OFF` | Build against Qt6 instead of Qt5 |
| `BUILD_TOOLS` | `ON` | Build `udp-sender` and `zmq-sender` test tools |

### Targets

| Binary | Description |
|---|---|
| `trigger-scalers` | Main GUI application |
| `udp-sender` | Qt-based UDP test sender |
| `zmq-sender` | Plain C++ ZeroMQ test sender (requires `WITH_ZEROMQ=ON`) |

---

## Running

```bash
./build/trigger-scalers --config config/scalars.yaml
```

### Command-Line Options

| Option | Argument | Description |
|---|---|---|
| `-c`, `--config` | `file` | YAML configuration file (default: `scalars.yaml`) |
| `--protocol` | `udp\|zeromq` | Override transport protocol |
| `--size` | `small\|medium\|large\|huge` | Override display size |
| `--interval-ms` | `ms` | Override rate update interval |
| `--columns` | `n` | Override widgets per row |
| `--udp-address` | `addr` | Override UDP bind address |
| `--udp-port` | `port` | Override UDP port |
| `--zmq-endpoint` | `url` | Override ZeroMQ endpoint (full URL) |
| `--zmq-port` | `port` | Replace port in configured ZeroMQ endpoint |

---

## Configuration

The configuration file is YAML. The default is `config/scalars.yaml`.

```yaml
display:
  size: medium              # small | medium | large | huge
  update_interval_ms: 1000
  columns: 4                # ScalarWidgets per row
  color_scheme: "Amber"     # Amber | Green Phosphor | Blue | Red | Light

transport:
  protocol: udp             # udp | zeromq
  udp:
    bind_address: "0.0.0.0"
    port: 5555
  zeromq:
    endpoint: "tcp://daq-host:5556"
    socket_type: sub        # sub (PUB/SUB) | pull (PUSH/PULL)

banks:
  - name: "Beam & Timing Triggers"
    triggers:
      - id: 0               # Matches TriggerMessage::category from DAQ
        name: "Mu2e Spill"
        target_rate: 0.6    # Hz (informational)
        visible: true
        alarm:
          enabled: true
          min_rate: 0.1     # Hz; rate below this → red indicator
          max_rate: 2.0     # Hz; rate above this → yellow indicator
```

Triggers with `visible: false` are hidden in the UI but still accumulate counts.
Set `min_rate` or `max_rate` to `-1.0` to disable that threshold.

The active color scheme is written back when you use **File → Save Config**.

---

## Wire Protocol

Every message is a 12-byte little-endian struct:

| Offset | Type | Field | Meaning |
|---|---|---|---|
| 0 | `uint32_t` | `category` | Trigger category index (0-based) |
| 4 | `uint64_t` | `value` | Increment to add to the receiver's running count |

The receiver maintains its own per-category total and adds each incoming `value` to it.
The DAQ only sends; the application never replies.

---

## Test Sender Tools

Both tools simulate Poisson-distributed trigger firing at the rates defined in
`tools/SimulatedTriggers.h`.

### udp-sender

Qt-based UDP broadcast sender.

```bash
# Local loopback (default)
./build/udp-sender

# Subnet broadcast
./build/udp-sender --host 255.255.255.255 --port 5555

# 10× faster rates, 50 ms send interval
./build/udp-sender --rate-scale 10 --interval-ms 50

# Single shot: send one message for category 0x00E and exit
./build/udp-sender --single 0x00E
```

### zmq-sender

Plain C++ ZeroMQ sender (no Qt dependency).

```bash
# PUB socket — receiver must use socket_type: sub
./build/zmq-sender

# PUSH socket — receiver must use socket_type: pull
./build/zmq-sender --socket-type push --endpoint tcp://0.0.0.0:5556

# Single shot: send one message for category 14 and exit
./build/zmq-sender --single 14

# 5× rates, 200 ms send interval
./build/zmq-sender --rate-scale 5 --interval-ms 200
```

Both tools print a status line to stdout every second:

```
[  1.0s] pkts=42 | 0:1 1:14 2:10 3:11 4:0 5:1 ...
```

The `--single` flag accepts decimal or hex (`0x00E`) category IDs.

---

## GUI Reference

### Menus

| Menu | Item | Shortcut | Action |
|---|---|---|---|
| File | Save Config… | Ctrl+S | Save current settings to YAML |
| File | Quit | Ctrl+Q | Exit |
| Edit | Copy Scalers | Ctrl+C | Copy CSV snapshot to clipboard |
| Edit | Reset All Counts | | Zero all counters |
| Edit | Enable All Scalers | | Resume counting on all widgets |
| Edit | Disable All Scalers | | Pause counting on all widgets |
| View | Color Scheme | | Switch between five built-in themes |
| Help | About | | Version and transport information |

### Per-Scaler Controls

Right-click any scalar widget for a context menu with:
- **Enable / Disable Scaler** — pauses or resumes count accumulation for that trigger
- **Reset Count** — zeroes the count for that trigger only

Click the small LED dot in the bottom-left of each widget to toggle enable/disable without opening a menu. The LED is green when enabled and dark when disabled.

### Clipboard CSV Format

```
2026-03-30 09:15:42
Mu2e Spill,12345,0.587
Booster Spill,67890,14.923
Beam Flash,4,0.003
```

Timestamp, then one row per scaler: `name,count,rate_hz`.

### Alarm Indicator Colors

| Color | Meaning |
|---|---|
| Gray | Alarm disabled |
| Green | Rate within [min_rate, max_rate] |
| Red | Rate below min_rate |
| Yellow | Rate above max_rate |

---

## Architecture

```
MainWindow  (QMainWindow)
├── QScrollArea
│   └── ScalarBank  (QGroupBox)  — one per "banks" entry in YAML
│       └── ScalarWidget  (QFrame)  — one per trigger
│           ├── Name label
│           ├── Count label  (Nixie-style, scheme-colored)
│           └── Status row: LED · rate label · alarm dot
└── QStatusBar
```

**Data flow:**
1. `UdpReceiver` (main thread, event-driven) or `ZmqReceiver` (background `QThread`) emits `messageReceived(quint32 category, quint64 value)`.
2. `MainWindow::handleMessage` dispatches to every `ScalarBank`, which routes to the matching `ScalarWidget` via an `unordered_map<id, ScalarWidget*>`.
3. `ScalarWidget::updateCount` adds `value` to `m_count` if the widget is enabled.
4. A `QTimer` fires every `update_interval_ms` and triggers `refreshRate()` on every widget, which computes a rolling average over the last 10 samples.

---

## Legacy Code

`TriggerScalars-Old/` contains the original Qt4 / SoftRelTools / DDS implementation and is kept for reference only. It is not built by CMake.

| Aspect | Legacy | Current |
|---|---|---|
| Qt | 4 | 5 or 6 |
| Build | GNUmakefile + SoftRelTools | CMake |
| Transport | DDS | UDP / ZeroMQ |
| Trigger ID | 96-bit bitmask | uint32_t category index |

---

## License

This software is part of the Mu2e DAQ project at Fermilab.
