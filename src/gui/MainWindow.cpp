#include "MainWindow.h"
#include "ScalarBank.h"
#include "config/Config.h"
#include "network/UdpReceiver.h"
#include "network/ZmqReceiver.h"
#include <QActionGroup>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMenuBar>
#include <QStatusBar>
#include <QTimer>
#include <QLabel>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>

MainWindow::MainWindow(const AppConfig& config, QWidget* parent)
    : QMainWindow(parent)
    , m_config(config) {
    setWindowTitle(QString::fromStdString(config.display.window_title));

    // Dark application palette
    QPalette p = palette();
    p.setColor(QPalette::Window,     QColor(0x12, 0x12, 0x12));
    p.setColor(QPalette::WindowText, QColor(0xcc, 0xcc, 0xcc));
    p.setColor(QPalette::Base,       QColor(0x1a, 0x1a, 0x1a));
    p.setColor(QPalette::Text,       QColor(0xcc, 0xcc, 0xcc));
    setPalette(p);

    // Pick the scheme named in the config, fall back to Amber
    for (const ColorScheme& s : builtinColorSchemes()) {
        if (s.name.toStdString() == config.display.color_scheme) {
            m_colorScheme = s;
            break;
        }
    }

    setupUi();
    setupMenus();
    applyColorScheme(m_colorScheme);
    adjustSize();
    setupReceiver();

    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::onRefreshTimer);
    m_refreshTimer->start(config.display.update_interval_ms);
}

MainWindow::~MainWindow() = default;

// ---------------------------------------------------------------------------
// UI setup
// ---------------------------------------------------------------------------
void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    central->setStyleSheet("background-color: #121212;");
    setCentralWidget(central);

    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(10);

    m_scrollArea = new QScrollArea(this);
    auto* scroll = m_scrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameStyle(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background-color: #121212; border: none; }"
                          "QScrollBar:vertical { background: #1a1a1a; width: 10px; }"
                          "QScrollBar::handle:vertical { background: #3a3a3a; border-radius: 4px; }");

    auto* bankContainer = new QWidget;
    bankContainer->setStyleSheet("background-color: #121212;");
    auto* bankLayout = new QVBoxLayout(bankContainer);
    bankLayout->setSpacing(14);
    bankLayout->setContentsMargins(0, 0, 4, 0);

    for (const auto& bankCfg : m_config.banks) {
        auto* bank = new ScalarBank(bankCfg, m_config.display, bankContainer);
        bankLayout->addWidget(bank);
        m_banks.push_back(bank);
    }
    bankLayout->addStretch();
    scroll->setWidget(bankContainer);
    scroll->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    mainLayout->addWidget(scroll);

    // Status bar
    m_statusLabel = new QLabel("Waiting for messages...");
    m_statusLabel->setStyleSheet("color: #666666;");
    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->setStyleSheet(
        "QStatusBar { background-color: #0e0e0e; color: #666666; border-top: 1px solid #2a2a2a; }");
}

void MainWindow::setupMenus() {
    const QString menuStyle =
        "QMenuBar { background-color: #1a1a1a; color: #cccccc; }"
        "QMenuBar::item:selected { background-color: #2a2a2a; }"
        "QMenu { background-color: #1e1e1e; color: #cccccc; border: 1px solid #3a3a3a; }"
        "QMenu::item:selected { background-color: #2a4a7a; }";
    menuBar()->setStyleSheet(menuStyle);

    auto* fileMenu    = menuBar()->addMenu("&File");
    auto* saveAct     = fileMenu->addAction("&Save Config...");
    saveAct->setShortcut(QKeySequence::Save);
    connect(saveAct, &QAction::triggered, this, &MainWindow::saveConfig);
    fileMenu->addSeparator();
    auto* quitAct  = fileMenu->addAction("&Quit");
    quitAct->setShortcut(QKeySequence::Quit);
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);

    auto* editMenu  = menuBar()->addMenu("&Edit");
    auto* copyAct   = editMenu->addAction("&Copy Scalers");
    copyAct->setShortcut(QKeySequence::Copy);   // Ctrl+C
    connect(copyAct, &QAction::triggered, this, &MainWindow::copyToClipboard);

    editMenu->addSeparator();

    auto* resetAllAct = editMenu->addAction("Reset All Counts");
    connect(resetAllAct, &QAction::triggered, this, [this]() {
        for (auto* bank : m_banks) bank->resetAll();
    });

    editMenu->addSeparator();

    auto* enableAllAct  = editMenu->addAction("Enable All Scalers");
    auto* disableAllAct = editMenu->addAction("Disable All Scalers");
    connect(enableAllAct,  &QAction::triggered, this, [this]() {
        for (auto* bank : m_banks) bank->setAllEnabled(true);
    });
    connect(disableAllAct, &QAction::triggered, this, [this]() {
        for (auto* bank : m_banks) bank->setAllEnabled(false);
    });

    auto* viewMenu   = menuBar()->addMenu("&View");
    auto* schemeMenu = viewMenu->addMenu("Color &Scheme");
    auto* schemeGroup = new QActionGroup(this);
    schemeGroup->setExclusive(true);
    for (const ColorScheme& scheme : builtinColorSchemes()) {
        auto* act = schemeMenu->addAction(scheme.name);
        act->setCheckable(true);
        act->setChecked(scheme.name == m_colorScheme.name);
        schemeGroup->addAction(act);
        connect(act, &QAction::triggered, this, [this, scheme]() {
            applyColorScheme(scheme);
        });
    }

    auto* helpMenu  = menuBar()->addMenu("&Help");
    auto* aboutAct  = helpMenu->addAction("&About");
    connect(aboutAct, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About Trigger Scalers",
            "<h3>Mu2e Trigger Scalers</h3>"
            "<p>Real-time trigger rate monitoring for the Mu2e DAQ system.</p>"
            "<p>Supports UDP broadcast and ZeroMQ PUB/PUSH transports.</p>");
    });
}

void MainWindow::applyColorScheme(const ColorScheme& scheme) {
    m_colorScheme = scheme;
    m_config.display.color_scheme = scheme.name.toStdString();

    const QString bg = scheme.windowBg.name();

    centralWidget()->setStyleSheet(QString("background-color: %1;").arg(bg));

    m_scrollArea->setStyleSheet(QString(
        "QScrollArea { background-color: %1; border: none; }"
        "QScrollBar:vertical { background: %1; width: 10px; }"
        "QScrollBar::handle:vertical { background: %2; border-radius: 4px; }")
        .arg(bg, scheme.widgetBorder.name()));
    m_scrollArea->widget()->setStyleSheet(
        QString("background-color: %1;").arg(bg));

    statusBar()->setStyleSheet(QString(
        "QStatusBar { background-color: %1; color: %2; border-top: 1px solid %3; }")
        .arg(scheme.statusBg.name(), scheme.nameColor.name(),
             scheme.statusBorder.name()));

    const QString menuStyle = QString(
        "QMenuBar { background-color: %1; color: %2; }"
        "QMenuBar::item:selected { background-color: %3; }"
        "QMenu { background-color: %1; color: %2; border: 1px solid %4; }"
        "QMenu::item:selected { background-color: %3; }")
        .arg(scheme.menuBg.name(), scheme.bankTitle.name(),
             scheme.menuItemSelectedBg.name(), scheme.bankBorder.name());
    menuBar()->setStyleSheet(menuStyle);

    QPalette p = palette();
    p.setColor(QPalette::Window,     scheme.windowBg);
    p.setColor(QPalette::WindowText, scheme.bankTitle);
    p.setColor(QPalette::Base,       scheme.widgetBg);
    p.setColor(QPalette::Text,       scheme.nameColor);
    setPalette(p);

    for (auto* bank : m_banks)
        bank->applyColorScheme(scheme);
}

void MainWindow::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #1e1e1e; color: #cccccc; border: 1px solid #3a3a3a; }"
        "QMenu::item:selected { background-color: #2a4a7a; }");
    auto* copyAct = menu.addAction("&Copy Scalers\tCtrl+C");
    connect(copyAct, &QAction::triggered, this, &MainWindow::copyToClipboard);
    menu.exec(event->globalPos());
}

void MainWindow::saveConfig() {
    const QString path = QFileDialog::getSaveFileName(
        this, "Save Config",
        QString::fromStdString(m_config.display.window_title).replace(' ', '_') + ".yaml",
        "YAML Files (*.yaml *.yml);;All Files (*)");
    if (path.isEmpty()) return;

    try {
        ::saveConfig(m_config, path.toStdString());
        m_statusLabel->setText(QString("Config saved: %1").arg(path));
        m_statusLabel->setStyleSheet(QString("color: %1;")
            .arg(m_colorScheme.nameColor.name()));
    } catch (const std::exception& e) {
        handleError(QString("Save failed: %1").arg(e.what()));
    }
}

void MainWindow::copyToClipboard() {
    QString csv = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + '\n';
    for (auto* bank : m_banks) {
        for (const auto& [name, count, rateHz] : bank->csvRows()) {
            // Quote names that contain commas or quotes
            QString quotedName = name;
            if (quotedName.contains(',') || quotedName.contains('"')) {
                quotedName = '"' + quotedName.replace('"', "\"\"") + '"';
            }
            csv += quotedName + ','
                 + QString::number(count) + ','
                 + QString::number(rateHz, 'f', 3) + '\n';
        }
    }
    QGuiApplication::clipboard()->setText(csv);
    m_statusLabel->setText("Copied to clipboard");
    m_statusLabel->setStyleSheet("color: #888888;");
}

// ---------------------------------------------------------------------------
// Receiver setup
// ---------------------------------------------------------------------------
void MainWindow::setupReceiver() {
    const std::string& proto = m_config.transport.protocol;

    if (proto == "udp") {
        auto* recv = new UdpReceiver(
            QString::fromStdString(m_config.transport.udp.bind_address),
            static_cast<quint16>(m_config.transport.udp.port), this);
        connect(recv, &UdpReceiver::messageReceived, this, &MainWindow::handleMessage);
        connect(recv, &UdpReceiver::errorOccurred,   this, &MainWindow::handleError);
        if (!recv->start()) {
            handleError("Failed to start UDP receiver — check port and permissions");
        } else {
            m_statusLabel->setText(
                QString("Listening on UDP port %1").arg(m_config.transport.udp.port));
        }
    } else if (proto == "zeromq") {
        auto* recv = new ZmqReceiver(
            QString::fromStdString(m_config.transport.zeromq.endpoint),
            QString::fromStdString(m_config.transport.zeromq.socket_type), this);
        connect(recv, &ZmqReceiver::messageReceived, this, &MainWindow::handleMessage);
        connect(recv, &ZmqReceiver::errorOccurred,   this, &MainWindow::handleError);
        recv->start();
        m_statusLabel->setText(
            QString("Connected to ZMQ %1").arg(
                QString::fromStdString(m_config.transport.zeromq.endpoint)));
    } else {
        handleError(QString("Unknown transport protocol '%1' — no receiver started")
                        .arg(QString::fromStdString(proto)));
    }
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------
void MainWindow::handleMessage(quint32 category, quint64 value) {
    ++m_messageCount;
    for (auto* bank : m_banks) {
        bank->handleMessage(category, value);
    }
}

void MainWindow::handleError(const QString& error) {
    m_statusLabel->setText("Error: " + error);
    m_statusLabel->setStyleSheet("color: #ff4040;");
}

void MainWindow::onRefreshTimer() {
    for (auto* bank : m_banks) {
        bank->refreshRates();
    }
    if (m_messageCount > 0) {
        m_statusLabel->setText(
            QString("Messages received: %1").arg(m_messageCount));
        m_statusLabel->setStyleSheet("color: #888888;");
    }
}
