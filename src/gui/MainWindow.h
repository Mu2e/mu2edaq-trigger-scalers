#pragma once

#include "config/Config.h"
#include "ColorScheme.h"
#include <QMainWindow>
#include <vector>
#include <cstdint>

class ScalarBank;
class QActionGroup;
class QTimer;
class QLabel;
class QScrollArea;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const AppConfig& config, QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void handleMessage(quint32 category, quint64 value);
    void handleError(const QString& error);
    void onRefreshTimer();
    void copyToClipboard();
    void saveConfig();

private:
    void setupUi();
    void setupMenus();
    void setupReceiver();
    void applyColorScheme(const ColorScheme& scheme);

    AppConfig  m_config;
    std::vector<ScalarBank*> m_banks;
    QTimer*      m_refreshTimer{nullptr};
    QLabel*      m_statusLabel{nullptr};
    QScrollArea* m_scrollArea{nullptr};
    quint64      m_messageCount{0};
    ColorScheme  m_colorScheme{builtinColorSchemes().first()};
};
