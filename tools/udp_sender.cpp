// udp_sender — simulates DAQ trigger broadcasts over UDP.
//
// Usage:
//   udp-sender [--host <addr>] [--port <n>] [--interval-ms <n>] [--rate-scale <f>]
//
// Sends TriggerMessage packets (12 bytes each) to the given host/port using the
// same wire format as the trigger-scalers display application.  By default it
// sends to 127.0.0.1 for local testing; use "--host 255.255.255.255" to broadcast
// to the local subnet.
//
// Trigger counts grow according to Poisson-distributed firing at each trigger's
// nominal rate.  Use --rate-scale to speed up or slow down all rates together.

#include "SimulatedTriggers.h"
#include "../src/network/TriggerMessage.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QUdpSocket>
#include <QTimer>
#include <QDateTime>
#include <QTextStream>

#include <array>
#include <random>
#include <cstring>

// ---------------------------------------------------------------------------
// Sender state
// ---------------------------------------------------------------------------
struct SenderState {
    QUdpSocket*  socket{nullptr};
    QHostAddress host;
    quint16      port{5555};
    int          intervalMs{100};
    double       rateScale{1.0};

    std::mt19937_64                         rng{std::random_device{}()};
    std::array<uint64_t, kNumTriggers>      counts{};
    qint64                                  startMs{0};
    uint64_t                                totalPackets{0};
    qint64                                  lastPrintMs{0};
};

// ---------------------------------------------------------------------------
// One tick: simulate + send
// ---------------------------------------------------------------------------
static void onTick(SenderState& s) {
    const double dt = s.intervalMs / 1000.0;

    for (int i = 0; i < kNumTriggers; ++i) {
        const uint64_t delta = poissonFire(i, dt * s.rateScale, s.rng);
        if (delta == 0) continue;

        s.counts[i] += delta;

        TriggerMessage msg;
        msg.category = kTriggers[i].id;
        msg.count    = s.counts[i];

        QByteArray buf(sizeof(TriggerMessage), Qt::Uninitialized);
        std::memcpy(buf.data(), &msg, sizeof(TriggerMessage));
        s.socket->writeDatagram(buf, s.host, s.port);
        ++s.totalPackets;
    }

    // Print a status line once per second
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - s.lastPrintMs >= 1000) {
        s.lastPrintMs = now;
        const double elapsed = (now - s.startMs) / 1000.0;

        QTextStream out(stdout);
        out << QString("[%1s] pkts=%2 | ").arg(elapsed, 6, 'f', 1).arg(s.totalPackets);
        for (int i = 0; i < kNumTriggers; ++i) {
            out << QString("%1:%2 ").arg(kTriggers[i].id).arg(s.counts[i]);
        }
        out << "\n";
        out.flush();
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("udp-sender");
    QCoreApplication::setApplicationVersion("1.0.0");

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Simulated DAQ trigger broadcast sender (UDP).\n"
        "Sends TriggerMessage packets matching the trigger-scalers wire format.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption hostOpt({"H", "host"},
        "Destination address (default: 127.0.0.1; use 255.255.255.255 to broadcast)",
        "addr", "127.0.0.1");
    QCommandLineOption portOpt({"p", "port"},
        "Destination UDP port (default: 5555)", "port", "5555");
    QCommandLineOption intervalOpt({"i", "interval-ms"},
        "Send interval in milliseconds (default: 100)", "ms", "100");
    QCommandLineOption scaleOpt({"s", "rate-scale"},
        "Multiply all trigger rates by this factor (default: 1.0)", "factor", "1.0");

    parser.addOption(hostOpt);
    parser.addOption(portOpt);
    parser.addOption(intervalOpt);
    parser.addOption(scaleOpt);
    parser.process(app);

    SenderState state;
    state.host       = QHostAddress(parser.value(hostOpt));
    state.port       = static_cast<quint16>(parser.value(portOpt).toUInt());
    state.intervalMs = parser.value(intervalOpt).toInt();
    state.rateScale  = parser.value(scaleOpt).toDouble();

    if (state.host.isNull()) {
        QTextStream(stderr) << "Invalid host address: " << parser.value(hostOpt) << "\n";
        return 1;
    }
    if (state.intervalMs <= 0) {
        QTextStream(stderr) << "Interval must be > 0\n";
        return 1;
    }

    state.socket = new QUdpSocket(&app);

    // SO_BROADCAST is required for 255.255.255.255
    if (state.host == QHostAddress::Broadcast) {
        state.socket->bind(QHostAddress(QHostAddress::AnyIPv4), 0);
    }

    QTextStream(stdout)
        << "UDP Trigger Sender\n"
        << "  destination : " << state.host.toString() << ":" << state.port << "\n"
        << "  interval    : " << state.intervalMs << " ms\n"
        << "  rate scale  : " << state.rateScale << "x\n"
        << "  triggers    : " << kNumTriggers << "\n"
        << "Press Ctrl+C to stop.\n\n";

    state.startMs    = QDateTime::currentMSecsSinceEpoch();
    state.lastPrintMs = state.startMs;

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&state]() { onTick(state); });
    timer.start(state.intervalMs);

    return app.exec();
}
