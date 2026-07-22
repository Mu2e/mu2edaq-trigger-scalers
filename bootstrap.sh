#!/usr/bin/env bash
# bootstrap.sh -- install system dependencies (CMake, Qt, yaml-cpp, ZeroMQ)
# for mu2edaq-trigger-scalers and configure the CMake build directory.
# This does not compile the project -- ./start-mu2edaq-trigger-scalers.sh
# builds it on first run, or build manually with:
#   cmake --build build
#
# Usage: ./bootstrap.sh [--no-install] [--with-zeromq=ON|OFF] [--use-qt6]
#
# See build.md for manual per-platform dependency install instructions.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WITH_ZEROMQ=ON
USE_QT6=OFF
DO_INSTALL=1

for arg in "$@"; do
    case "$arg" in
        --no-install)        DO_INSTALL=0 ;;
        --with-zeromq=*)     WITH_ZEROMQ="${arg#*=}" ;;
        --use-qt6)           USE_QT6=ON ;;
        -h|--help)
            grep '^#' "$0" | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *) echo "Unknown option: $arg" >&2; exit 2 ;;
    esac
done

if ! command -v cmake >/dev/null 2>&1; then
    if [ "$DO_INSTALL" = 1 ]; then
        echo "cmake not found; attempting to install system dependencies..."
        if [ "$(uname)" = "Darwin" ] && command -v brew >/dev/null 2>&1; then
            brew install cmake qt yaml-cpp zeromq cppzmq
        elif command -v apt-get >/dev/null 2>&1; then
            sudo apt-get update
            sudo apt-get install -y cmake g++ qtbase5-dev libqt5network5 \
                libyaml-cpp-dev libzmq3-dev
        elif command -v dnf >/dev/null 2>&1; then
            sudo dnf install -y cmake gcc-c++ qt5-qtbase-devel yaml-cpp-devel \
                zeromq-devel cppzmq-devel
        else
            echo "error: cmake not found and no supported package manager" \
                 "(brew/apt-get/dnf) detected. See build.md for manual" \
                 "install instructions." >&2
            exit 1
        fi
    else
        echo "error: cmake not found on PATH. See build.md for install" \
             "instructions, or re-run without --no-install." >&2
        exit 1
    fi
fi
echo "Using $(cmake --version | head -1)"

CMAKE_ARGS=("-B" "$HERE/build" "-DWITH_ZEROMQ=$WITH_ZEROMQ")
if [ "$USE_QT6" = ON ]; then
    CMAKE_ARGS+=("-DUSE_QT6=ON")
fi

if [ "$(uname)" = "Darwin" ] && command -v brew >/dev/null 2>&1 && [ "$USE_QT6" = OFF ]; then
    BREW_QT5="$(brew --prefix qt@5 2>/dev/null || true)"
    if [ -n "$BREW_QT5" ] && [ -d "$BREW_QT5" ]; then
        CMAKE_ARGS+=("-DCMAKE_PREFIX_PATH=$BREW_QT5")
    fi
fi

echo "Configuring CMake build in $HERE/build"
cmake -S "$HERE" "${CMAKE_ARGS[@]}"

echo ""
echo "Bootstrap complete."
echo "Build with:  cmake --build build"
echo "Run with:    ./build/trigger-scalers --config config/scalars.yaml"
echo "Or start via ./start-mu2edaq-trigger-scalers.sh, which builds on first run."
