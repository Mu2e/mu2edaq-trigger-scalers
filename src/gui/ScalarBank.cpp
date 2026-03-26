#include "ScalarBank.h"
#include "ScalarWidget.h"
#include <QGridLayout>

ScalarBank::ScalarBank(const BankConfig& bankConfig, const DisplayConfig& display,
                       QWidget* parent)
    : QGroupBox(QString::fromStdString(bankConfig.name), parent) {
    setStyleSheet(
        "QGroupBox {"
        "  color: #bbbbbb;"
        "  border: 1px solid #3a3a3a;"
        "  border-radius: 5px;"
        "  margin-top: 8px;"
        "  font-weight: bold;"
        "  font-size: 10pt;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  padding: 0 6px;"
        "}");

    auto* grid = new QGridLayout(this);
    grid->setSpacing(6);
    grid->setContentsMargins(8, 16, 8, 8);

    int col = 0, row = 0;
    const int maxCols = display.columns;

    for (const auto& trigCfg : bankConfig.triggers) {
        auto* widget = new ScalarWidget(trigCfg, display, this);
        if (trigCfg.visible) {
            grid->addWidget(widget, row, col);
            if (++col >= maxCols) { col = 0; ++row; }
        }
        m_scalars[trigCfg.id] = widget;
    }
}

void ScalarBank::handleMessage(uint32_t category, uint64_t count) {
    auto it = m_scalars.find(static_cast<int>(category));
    if (it != m_scalars.end()) {
        it->second->updateCount(count);
    }
}

void ScalarBank::refreshRates() {
    for (auto& [id, widget] : m_scalars) {
        widget->refreshRate();
    }
}
