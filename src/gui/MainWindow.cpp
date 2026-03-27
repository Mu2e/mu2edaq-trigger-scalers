#include "MainWindow.h"
#include "ScalarBank.h"
#include "network/UdpReceiver.h"
#include "network/ZmqReceiver.h"
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

    setupUi();
    adjustSize();
    setupMenus();
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

    auto* scroll = new QScrollArea(this);
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

    auto* fileMenu = menuBar()->addMenu("&File");
    auto* quitAct  = fileMenu->addAction("&Quit");
    quitAct->setShortcut(QKeySequence::Quit);
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);

    auto* editMenu  = menuBar()->addMenu("&Edit");
    auto* copyAct   = editMenu->addAction("&Copy Scalers");
    copyAct->setShortcut(QKeySequence::Copy);   // Ctrl+C
    connect(copyAct, &QAction::triggered, this, &MainWindow::copyToClipboard);

    auto* helpMenu  = menuBar()->addMenu("&Help");
    auto* aboutAct  = helpMenu->addAction("&About");
    connect(aboutAct, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About Trigger Scalers",
            "<h3>Mu2e Trigger Scalers</h3>"
            "<p>Real-time trigger rate monitoring for the Mu2e DAQ system.</p>"
            "<p>Supports UDP broadcast and ZeroMQ PUB/PUSH transports.</p>");
    });
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

void MainWindow::copyToClipboard() {
    QString csv = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + '\n';
    for (auto* bank : m_banks) {
        for (const auto& [name, count] : bank->csvRows()) {
            // Quote names that contain commas or quotes
            QString quotedName = name;
            if (quotedName.contains(',') || quotedName.contains('"')) {
                quotedName = '"' + quotedName.replace('"', "\"\"") + '"';
            }
            csv += quotedName + ',' + QString::number(count) + '\n';
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
