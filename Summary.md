# Test Sender Tools

## `tools/SimulatedTriggers.h` (shared)
Defines the 18-trigger array (matching `scalars.yaml`) with nominal rates, plus `poissonFire()` — a Poisson-distributed event generator. Both senders include this header.

## `udp-sender`
Qt-based; uses `QUdpSocket` + `QTimer`. Sends to a specific host or broadcast address:

```bash
# Local testing (default: 127.0.0.1:5555)
./build/udp-sender

# Subnet broadcast at 10× normal rates
./build/udp-sender --host 255.255.255.255 --rate-scale 10

# Faster tick for stress testing
./build/udp-sender --interval-ms 10 --rate-scale 0.5
```

## `zmq-sender`
Plain C++ with no Qt dependency — just `zmq.hpp` and `<thread>`. Binds a PUB (or PUSH) socket that the display app connects to:

```bash
# PUB socket (display should use socket_type: sub)
./build/zmq-sender

# PUSH socket (display should use socket_type: pull)
./build/zmq-sender --socket-type push

# Custom endpoint, 5× rates
./build/zmq-sender --endpoint tcp://0.0.0.0:5556 --rate-scale 5
```

Both print a compact status line each second showing elapsed time, total packets sent, and per-category cumulative counts. Both built by default with `cmake -B build`.
