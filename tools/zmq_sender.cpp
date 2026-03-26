// zmq_sender — simulates DAQ trigger broadcasts over ZeroMQ.
//
// Usage:
//   zmq-sender [--endpoint <url>] [--socket-type <pub|push>]
//              [--interval-ms <n>] [--rate-scale <f>]
//
// Binds a PUB (or PUSH) socket and publishes TriggerMessage packets using the
// same wire format as the trigger-scalers display application.
//
// The trigger-scalers ZmqReceiver should be configured with:
//   socket_type: sub   (connects to this sender's PUB socket)
//   endpoint: tcp://127.0.0.1:5556
//
// Trigger counts grow according to Poisson-distributed firing at each trigger's
// nominal rate.  Use --rate-scale to speed up or slow down all rates together.

#include "SimulatedTriggers.h"
#include "../src/network/TriggerMessage.h"

#include <zmq.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <thread>

// ---------------------------------------------------------------------------
// Graceful shutdown on SIGINT / SIGTERM
// ---------------------------------------------------------------------------
static std::atomic<bool> g_running{true};

static void signalHandler(int) {
    g_running = false;
}

// ---------------------------------------------------------------------------
// Simple command-line parser (avoids pulling in Qt for this tool)
// ---------------------------------------------------------------------------
static std::string getArg(int argc, char* argv[], const std::string& flag,
                           const std::string& defaultVal = "") {
    for (int i = 1; i < argc - 1; ++i) {
        if (argv[i] == flag) return argv[i + 1];
    }
    return defaultVal;
}

static bool hasFlag(int argc, char* argv[], const std::string& flag) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == flag) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Print usage
// ---------------------------------------------------------------------------
static void printUsage(const char* prog) {
    std::cout
        << "Usage: " << prog << " [options]\n"
        << "\nSimulated DAQ trigger broadcast sender (ZeroMQ).\n"
        << "Sends TriggerMessage packets matching the trigger-scalers wire format.\n"
        << "\nOptions:\n"
        << "  --endpoint <url>      ZMQ bind endpoint (default: tcp://0.0.0.0:5556)\n"
        << "  --socket-type <type>  'pub' or 'push' (default: pub)\n"
        << "  --interval-ms <n>     Send interval in ms (default: 100)\n"
        << "  --rate-scale <f>      Multiply all rates by factor (default: 1.0)\n"
        << "  --help                Show this help\n"
        << "\nThe receiver should use:\n"
        << "  socket_type: sub  (for pub)\n"
        << "  socket_type: pull (for push)\n"
        << "  endpoint: tcp://127.0.0.1:5556\n";
}

// ---------------------------------------------------------------------------
// Print a status line to stdout
// ---------------------------------------------------------------------------
static void printStatus(double elapsedS, uint64_t totalPackets,
                        const std::array<uint64_t, kNumTriggers>& counts) {
    std::cout << std::fixed << std::setprecision(1)
              << "[" << std::setw(7) << elapsedS << "s]"
              << " pkts=" << totalPackets << " |";
    for (int i = 0; i < kNumTriggers; ++i) {
        std::cout << " " << kTriggers[i].id << ":" << counts[i];
    }
    std::cout << "\n" << std::flush;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (hasFlag(argc, argv, "--help") || hasFlag(argc, argv, "-h")) {
        printUsage(argv[0]);
        return 0;
    }

    const std::string endpoint   = getArg(argc, argv, "--endpoint",    "tcp://0.0.0.0:5556");
    const std::string socketType = getArg(argc, argv, "--socket-type", "pub");
    const int intervalMs         = std::stoi(getArg(argc, argv, "--interval-ms", "100"));
    const double rateScale       = std::stod(getArg(argc, argv, "--rate-scale",  "1.0"));

    if (intervalMs <= 0) {
        std::cerr << "Error: --interval-ms must be > 0\n";
        return 1;
    }
    if (socketType != "pub" && socketType != "push") {
        std::cerr << "Error: --socket-type must be 'pub' or 'push'\n";
        return 1;
    }

    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    zmq::context_t ctx(1);

    const zmq::socket_type zmqType = (socketType == "push")
        ? zmq::socket_type::push
        : zmq::socket_type::pub;
    zmq::socket_t socket(ctx, zmqType);

    // High-water mark: drop old messages rather than block if receiver is slow
    socket.set(zmq::sockopt::sndhwm, 1000);

    try {
        socket.bind(endpoint);
    } catch (const zmq::error_t& e) {
        std::cerr << "Failed to bind to " << endpoint << ": " << e.what() << "\n";
        return 1;
    }

    std::cout
        << "ZMQ Trigger Sender\n"
        << "  endpoint    : " << endpoint << "\n"
        << "  socket type : " << socketType << "\n"
        << "  interval    : " << intervalMs << " ms\n"
        << "  rate scale  : " << rateScale << "x\n"
        << "  triggers    : " << kNumTriggers << "\n"
        << "Press Ctrl+C to stop.\n\n";

    // Give subscribers a moment to connect before the first message
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::mt19937_64 rng{std::random_device{}()};
    std::array<uint64_t, kNumTriggers> counts{};
    uint64_t totalPackets = 0;

    const double dt = intervalMs / 1000.0;
    const auto intervalDur = std::chrono::milliseconds(intervalMs);

    const auto startTime = std::chrono::steady_clock::now();
    auto lastPrint       = startTime;

    while (g_running) {
        const auto tickStart = std::chrono::steady_clock::now();

        // Simulate and send
        for (int i = 0; i < kNumTriggers; ++i) {
            const uint64_t delta = poissonFire(i, dt * rateScale, rng);
            if (delta == 0) continue;

            counts[i] += delta;

            TriggerMessage msg;
            msg.category = kTriggers[i].id;
            msg.count    = counts[i];

            zmq::message_t zmqMsg(sizeof(TriggerMessage));
            std::memcpy(zmqMsg.data(), &msg, sizeof(TriggerMessage));
            socket.send(zmqMsg, zmq::send_flags::dontwait);
            ++totalPackets;
        }

        // Print status once per second
        const auto now = std::chrono::steady_clock::now();
        if (now - lastPrint >= std::chrono::seconds(1)) {
            lastPrint = now;
            const double elapsed =
                std::chrono::duration<double>(now - startTime).count();
            printStatus(elapsed, totalPackets, counts);
        }

        // Sleep for remainder of interval
        const auto elapsed = std::chrono::steady_clock::now() - tickStart;
        if (elapsed < intervalDur) {
            std::this_thread::sleep_for(intervalDur - elapsed);
        }
    }

    std::cout << "\nStopped. Total packets sent: " << totalPackets << "\n";
    return 0;
}
