#pragma once

#include "config/Config.h"
#include <QMainWindow>
#include <vector>
#include <cstdint>

class ScalarBank;
class QTimer;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const AppConfig& config, QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void handleMessage(uint32_t category, uint64_t count);
    void handleError(const QString& error);
    void onRefreshTimer();

private:
    void setupUi();
    void setupMenus();
    void setupReceiver();

    AppConfig m_config;
    std::vector<ScalarBank*> m_banks;
    QTimer*  m_refreshTimer{nullptr};
    QLabel*  m_statusLabel{nullptr};
    quint64  m_messageCount{0};
};
