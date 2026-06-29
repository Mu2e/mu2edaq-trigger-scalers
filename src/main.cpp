#include "config/Config.h"
#include "gui/MainWindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QRegularExpression>
#include <mu2edaq_discovery/Responder.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("trigger-scalers");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("Mu2e");

    QCommandLineParser parser;
    parser.setApplicationDescription("Mu2e DAQ Trigger Scalers Display");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption configOpt({"c", "config"},
        "Path to YAML configuration file", "file", "scalars.yaml");
    QCommandLineOption protocolOpt("protocol",
        "Transport protocol: udp | zeromq (overrides config)", "proto");
    QCommandLineOption sizeOpt("size",
        "Display size: small | medium | large | huge", "size");
    QCommandLineOption intervalOpt("interval-ms",
        "Rate update interval in milliseconds", "ms");
    QCommandLineOption columnsOpt("columns",
        "Scalar widgets per row", "n");
    QCommandLineOption udpPortOpt("udp-port",
        "UDP bind port (overrides config)", "port");
    QCommandLineOption udpAddrOpt("udp-address",
        "UDP bind address (overrides config)", "addr");
    QCommandLineOption zmqEndpointOpt("zmq-endpoint",
        "ZeroMQ endpoint URL, e.g. tcp://host:5556 (overrides config)", "url");
    QCommandLineOption zmqPortOpt("zmq-port",
        "ZeroMQ port; replaces the port in the config endpoint", "port");

    parser.addOption(configOpt);
    parser.addOption(protocolOpt);
    parser.addOption(sizeOpt);
    parser.addOption(intervalOpt);
    parser.addOption(columnsOpt);
    parser.addOption(udpPortOpt);
    parser.addOption(udpAddrOpt);
    parser.addOption(zmqEndpointOpt);
    parser.addOption(zmqPortOpt);
    parser.process(app);

    const std::string configFile = parser.value(configOpt).toStdString();

    AppConfig config;
    try {
        config = loadConfig(configFile);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    // Apply command-line overrides
    if (parser.isSet(protocolOpt))
        config.transport.protocol = parser.value(protocolOpt).toStdString();
    if (parser.isSet(sizeOpt))
        config.display.size = parser.value(sizeOpt).toStdString();
    if (parser.isSet(intervalOpt))
        config.display.update_interval_ms = parser.value(intervalOpt).toInt();
    if (parser.isSet(columnsOpt))
        config.display.columns = parser.value(columnsOpt).toInt();
    if (parser.isSet(udpAddrOpt))
        config.transport.udp.bind_address = parser.value(udpAddrOpt).toStdString();
    if (parser.isSet(udpPortOpt))
        config.transport.udp.port = parser.value(udpPortOpt).toInt();
    if (parser.isSet(zmqEndpointOpt))
        config.transport.zeromq.endpoint = parser.value(zmqEndpointOpt).toStdString();
    if (parser.isSet(zmqPortOpt)) {
        // Replace just the port in whatever endpoint is currently set
        QString ep = QString::fromStdString(config.transport.zeromq.endpoint);
        ep.replace(QRegularExpression(":\\d+$"), ":" + parser.value(zmqPortOpt));
        config.transport.zeromq.endpoint = ep.toStdString();
    }

    MainWindow window(config);
    window.resize(1100, 650);
    window.show();

    // Mu2e DAQ service discovery: advertise the active transport port so the
    // app appears in mu2edaq-discover scans and the control room browser. The
    // receiver is already bound by MainWindow above. Best-effort: a failure to
    // start the responder must not stop the application.
    mu2edaq_discovery::Responder::Options dopt;
    dopt.name = "Trigger Scalers";
    dopt.app = "trigger-scalers";
    if (config.transport.protocol == "zeromq") {
        QString ep = QString::fromStdString(config.transport.zeromq.endpoint);
        QRegularExpressionMatch m = QRegularExpression(":(\\d+)$").match(ep);
        dopt.port = m.hasMatch() ? m.captured(1).toInt() : 5556;
        dopt.scheme = "tcp";
    } else {
        dopt.port = config.transport.udp.port;
        dopt.scheme = "udp";
    }
    mu2edaq_discovery::Responder responder(dopt);
    try {
        responder.start();
    } catch (const std::exception& e) {
        std::cerr << "[Discovery] responder not started: " << e.what() << "\n";
    }

    int rc = app.exec();
    responder.stop();
    return rc;
}
