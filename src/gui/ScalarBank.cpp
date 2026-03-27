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
        m_orderedWidgets.append(widget);
    }
}

void ScalarBank::applyColorScheme(const ColorScheme& scheme) {
    setStyleSheet(QString(
        "QGroupBox {"
        "  color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 5px;"
        "  margin-top: 8px;"
        "  font-weight: bold;"
        "  font-size: 10pt;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  padding: 0 6px;"
        "}").arg(scheme.bankTitle.name(), scheme.bankBorder.name()));

    for (auto* w : m_orderedWidgets)
        w->applyColorScheme(scheme);
}

void ScalarBank::resetAll() {
    for (auto* w : m_orderedWidgets)
        w->resetCount();
}

void ScalarBank::setAllEnabled(bool enabled) {
    for (auto* w : m_orderedWidgets)
        w->setCountingEnabled(enabled);
}

QVector<std::tuple<QString, quint64, double>> ScalarBank::csvRows() const {
    QVector<std::tuple<QString, quint64, double>> rows;
    rows.reserve(m_orderedWidgets.size());
    for (auto* w : m_orderedWidgets)
        rows.append({w->triggerName(), w->count(), w->rate()});
    return rows;
}

void ScalarBank::handleMessage(quint32 category, quint64 value) {
    auto it = m_scalars.find(static_cast<int>(category));
    if (it != m_scalars.end()) {
        it->second->updateCount(value);
    }
}

void ScalarBank::refreshRates() {
    for (auto& [id, widget] : m_scalars) {
        widget->refreshRate();
    }
}
