#pragma once

#include "config/Config.h"
#include "ColorScheme.h"
#include <QGroupBox>
#include <QVector>
#include <tuple>
#include <unordered_map>
#include <cstdint>

class ScalarWidget;

// A labeled group of ScalarWidgets arranged in a grid.
// Triggers that are not visible in config are omitted.
class ScalarBank : public QGroupBox {
    Q_OBJECT
public:
    explicit ScalarBank(const BankConfig& config, const DisplayConfig& display,
                        QWidget* parent = nullptr);

    // Route an incoming message to the matching ScalarWidget (if any).
    void handleMessage(quint32 category, quint64 value);

    // Trigger rate recalculation on all contained widgets.
    void refreshRates();

    // Returns (name, count, rate_hz) tuples for all scalars in config order.
    QVector<std::tuple<QString, quint64, double>> csvRows() const;

    // Enable or disable counting on every scalar in this bank.
    void setAllEnabled(bool enabled);

    void resetAll();

    void applyColorScheme(const ColorScheme& scheme);

private:
    std::unordered_map<int, ScalarWidget*> m_scalars;  // keyed by TriggerConfig::id
    QVector<ScalarWidget*> m_orderedWidgets;            // config insertion order
};
