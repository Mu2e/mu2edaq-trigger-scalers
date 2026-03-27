#pragma once

#include "config/Config.h"
#include <QFrame>
#include <cstdint>
#include <deque>

class QLabel;

// Displays the cumulative count and rolling rate for one trigger category.
// Visual style: dark "instrument panel" background, amber numeric display.
//
// Alarm states are indicated by the color of the indicator dot (●):
//   gray   — alarm monitoring disabled
//   green  — rate within configured thresholds
//   yellow — rate above max_rate
//   red    — rate below min_rate
class ScalarWidget : public QFrame {
    Q_OBJECT
public:
    explicit ScalarWidget(const TriggerConfig& config, const DisplayConfig& display,
                          QWidget* parent = nullptr);

    int     triggerId()   const { return m_config.id; }
    QString triggerName() const { return QString::fromStdString(m_config.name); }
    quint64 count()       const { return static_cast<quint64>(m_count); }

public slots:
    // Called by the message receiver when a new count arrives for this category.
    void updateCount(quint64 value);

    // Called by the MainWindow timer on each display refresh cycle.
    // Computes instantaneous rate, updates the rolling average, refreshes labels.
    void refreshRate();

private:
    void setupUi(const DisplayConfig& display);
    void updateAlarmIndicator();

    static int countFontSize(const std::string& size);

    TriggerConfig m_config;
    uint64_t m_count{0};
    uint64_t m_snapshotCount{0};
    qint64   m_snapshotTime{0};    // ms since epoch
    double   m_rate{0.0};          // rolling average rate (Hz)

    static constexpr int kHistoryDepth = 10;
    std::deque<double> m_rateHistory;

    QLabel* m_nameLabel{nullptr};
    QLabel* m_countLabel{nullptr};
    QLabel* m_rateLabel{nullptr};
    QLabel* m_alarmDot{nullptr};
};
