# Build Instructions

## Dependencies

| Dependency | Version | Purpose |
|------------|---------|---------|
| CMake      | ≥ 3.16  | Build system |
| C++ compiler | C++17 | GCC ≥ 7, Clang ≥ 5, or Apple Clang ≥ 10 |
| Qt         | 5 or 6  | GUI, UDP networking (`Core`, `Widgets`, `Network`) |
| yaml-cpp   | ≥ 0.7   | YAML configuration file parsing |
| libzmq     | ≥ 4.3   | ZeroMQ transport (required when `WITH_ZEROMQ=ON`) |
| cppzmq     | ≥ 4.3   | Header-only C++ bindings for libzmq |

---

## Installing Dependencies

### macOS (Homebrew)

```bash
brew install cmake qt yaml-cpp zeromq cppzmq
```

Qt is keg-only on Homebrew; help CMake find it by setting `CMAKE_PREFIX_PATH`:

```bash
# Qt6 (default brew formula)
export CMAKE_PREFIX_PATH="$(brew --prefix qt):$CMAKE_PREFIX_PATH"

# Qt5 if preferred
brew install qt@5
export CMAKE_PREFIX_PATH="$(brew --prefix qt@5):$CMAKE_PREFIX_PATH"
```

### Ubuntu / Debian

```bash
sudo apt-get update
sudo apt-get install -y \
    cmake \
    g++ \
    qtbase5-dev \
    libqt5network5 \
    libyaml-cpp-dev \
    libzmq3-dev

# cppzmq is header-only; install from package if available, otherwise manually:
sudo apt-get install -y libcppzmq-dev
# If not available in your distro:
sudo apt-get install -y libzmq3-dev   # provides the headers on older distros
```

> **Qt6 on Ubuntu 22.04+:**
> ```bash
> sudo apt-get install -y qt6-base-dev
> ```

### RHEL / Fedora / Rocky Linux / AlmaLinux

```bash
sudo dnf install -y \
    cmake \
    gcc-c++ \
    qt5-qtbase-devel \
    yaml-cpp-devel \
    zeromq-devel \
    cppzmq-devel
```

> **Qt6:**
> ```bash
> sudo dnf install -y qt6-qtbase-devel
> ```

### Scientific Linux / CentOS 7 (Fermilab systems)

These systems typically use the Fermilab UPS/CVMFS software stack.  The
recommended approach is to use the system compilers and install the remaining
dependencies via `spack` or from pre-built UPS products:

```bash
# Load a modern compiler
source /cvmfs/fermilab.opensciencegrid.org/products/common/etc/setups.sh
setup gcc v12_2_0

# Qt, yaml-cpp, and zeromq may be available as UPS products:
setup qt v5_15_2
setup yaml_cpp v0_7_0
setup zeromq v4_3_4
```

If UPS products are not available, build from source using `spack`:

```bash
spack install cmake qt@5 yaml-cpp zeromq cppzmq
spack load cmake qt yaml-cpp zeromq cppzmq
```

---

## Building

```bash
# Configure (from the repo root)
cmake -B build

# Configure with options
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_ZEROMQ=ON \
    -DBUILD_TOOLS=ON

# Compile
cmake --build build

# Compile using multiple cores
cmake --build build -- -j$(nproc)       # Linux
cmake --build build -- -j$(sysctl -n hw.logicalcpu)  # macOS
```

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `WITH_ZEROMQ` | `ON` | Build ZeroMQ receiver and `zmq-sender` tool |
| `BUILD_TOOLS` | `ON` | Build `udp-sender` and `zmq-sender` test tools |
| `CMAKE_BUILD_TYPE` | (none) | `Release`, `Debug`, or `RelWithDebInfo` |
| `CMAKE_PREFIX_PATH` | (none) | Extra search paths; needed for keg-only Homebrew Qt |

### Qt not found by CMake

If CMake cannot locate Qt automatically, pass the Qt installation prefix
explicitly:

```bash
# Homebrew Qt6
cmake -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"

# Homebrew Qt5
cmake -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt@5)"

# Custom Qt install
cmake -B build -DCMAKE_PREFIX_PATH="/opt/Qt/6.5.0/gcc_64"
```

### ZeroMQ not found by CMake

```bash
# If cppzmq CMake config is not installed, fall back is automatic via pkg-config.
# Confirm pkg-config can see libzmq:
pkg-config --modversion libzmq

# If not found, set PKG_CONFIG_PATH to point at the zmq installation:
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
cmake -B build
```

---

## Running

```bash
# Display application (reads config/scalars.yaml by default)
./build/trigger-scalers

# Specify a custom config file
./build/trigger-scalers --config /path/to/custom.yaml

# Test senders (in separate terminals)
./build/udp-sender                          # UDP loopback on port 5555
./build/zmq-sender                          # ZMQ PUB on tcp://0.0.0.0:5556
```

---

## Install

```bash
cmake --install build --prefix /usr/local
# Installs:
#   /usr/local/bin/trigger-scalers
#   /usr/local/bin/udp-sender
#   /usr/local/bin/zmq-sender
#   /usr/local/share/trigger-scalers/scalars.yaml
```
