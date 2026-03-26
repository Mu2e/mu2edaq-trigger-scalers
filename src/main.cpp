#include "config/Config.h"
#include "gui/MainWindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
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
    parser.addOption(configOpt);
    parser.process(app);

    const std::string configFile = parser.value(configOpt).toStdString();

    AppConfig config;
    try {
        config = loadConfig(configFile);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    MainWindow window(config);
    window.resize(1100, 650);
    window.show();

    return app.exec();
}
