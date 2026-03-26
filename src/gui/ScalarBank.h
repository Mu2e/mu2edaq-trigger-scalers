#pragma once

#include "config/Config.h"
#include <QGroupBox>
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

private:
    std::unordered_map<int, ScalarWidget*> m_scalars;  // keyed by TriggerConfig::id
};
