#include "ScalarWidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>

// ---------------------------------------------------------------------------
// Font size for the count display
// ---------------------------------------------------------------------------
int ScalarWidget::countFontSize(const std::string& size) {
    if (size == "small")  return 14;
    if (size == "large")  return 30;
    if (size == "huge")   return 42;
    return 22;  // medium (default)
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
ScalarWidget::ScalarWidget(const TriggerConfig& config, const DisplayConfig& display,
                           QWidget* parent)
    : QFrame(parent)
    , m_config(config) {
    setupUi(display);
    setVisible(config.visible);
}

void ScalarWidget::setupUi(const DisplayConfig& display) {
    setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
    setLineWidth(1);
    setStyleSheet(
        "ScalarWidget {"
        "  background-color: #1a1a1a;"
        "  border: 1px solid #2e2e2e;"
        "  border-radius: 4px;"
        "}");

    const int fontSize = countFontSize(display.size);

    // --- Name label (top) ---
    m_nameLabel = new QLabel(QString::fromStdString(m_config.name), this);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setStyleSheet("color: #999999; font-size: 9pt; background: transparent;");

    // --- Count display (Nixie-tube inspired: dark panel, amber digits) ---
    m_countLabel = new QLabel("0", this);
    m_countLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_countLabel->setStyleSheet(
        QString("color: #ff9000;"
                "background-color: #0c0c0c;"
                "border: 1px solid #2a2a2a;"
                "border-radius: 3px;"
                "padding: 2px 8px;"
                "font-family: 'Courier New', 'DejaVu Sans Mono', monospace;"
                "font-size: %1pt;"
                "font-weight: bold;").arg(fontSize));
    m_countLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_countLabel->setMinimumWidth(fontSize * 9);

    // --- Rate label ---
    m_rateLabel = new QLabel("0.000 Hz", this);
    m_rateLabel->setStyleSheet("color: #666666; font-size: 8pt; background: transparent;");

    // --- Alarm indicator dot ---
    m_alarmDot = new QLabel("●", this);
    m_alarmDot->setStyleSheet("color: #383838; font-size: 11pt; background: transparent;");
    m_alarmDot->setToolTip("Alarm disabled");
    m_alarmDot->setFixedWidth(14);

    // --- Status row ---
    auto* statusRow = new QHBoxLayout;
    statusRow->setContentsMargins(0, 0, 0, 0);
    statusRow->setSpacing(4);
    statusRow->addWidget(m_rateLabel, 1);
    statusRow->addWidget(m_alarmDot);

    // --- Outer layout ---
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 4);
    layout->setSpacing(3);
    layout->addWidget(m_nameLabel);
    layout->addWidget(m_countLabel);
    layout->addLayout(statusRow);

    // Tooltip shows expected rate when configured
    if (m_config.target_rate > 0.0) {
        setToolTip(QString("Expected rate: %1 Hz").arg(m_config.target_rate, 0, 'f', 3));
    }
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------
void ScalarWidget::updateCount(uint64_t count) {
    m_count = count;
    m_countLabel->setText(QString::number(m_count));
}

void ScalarWidget::refreshRate() {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (m_snapshotTime > 0) {
        const double dt = (now - m_snapshotTime) / 1000.0;
        if (dt > 0.0) {
            const double instant = static_cast<double>(m_count - m_snapshotCount) / dt;
            m_rateHistory.push_back(instant);
            if (static_cast<int>(m_rateHistory.size()) > kHistoryDepth) {
                m_rateHistory.pop_front();
            }
            double sum = 0.0;
            for (double r : m_rateHistory) sum += r;
            m_rate = sum / static_cast<double>(m_rateHistory.size());
        }
    }

    m_snapshotTime  = now;
    m_snapshotCount = m_count;

    // Format rate label
    if (m_rate >= 10.0) {
        m_rateLabel->setText(QString("%1 Hz").arg(m_rate, 0, 'f', 1));
    } else if (m_rate >= 0.001) {
        m_rateLabel->setText(QString("%1 Hz").arg(m_rate, 0, 'f', 3));
    } else {
        m_rateLabel->setText("0 Hz");
    }

    updateAlarmIndicator();
}

void ScalarWidget::updateAlarmIndicator() {
    if (!m_config.alarm.enabled) {
        m_alarmDot->setStyleSheet("color: #383838; font-size: 11pt; background: transparent;");
        m_alarmDot->setToolTip("Alarm disabled");
        return;
    }

    const double mn = m_config.alarm.min_rate;
    const double mx = m_config.alarm.max_rate;

    const bool tooLow  = (mn > 0.0 && m_rate < mn);
    const bool tooHigh = (mx > 0.0 && m_rate > mx);

    if (tooLow) {
        m_alarmDot->setStyleSheet("color: #ff3030; font-size: 11pt; background: transparent;");
        m_alarmDot->setToolTip(
            QString("ALARM LOW: %.3f Hz (min %.3f Hz)").arg(m_rate).arg(mn));
    } else if (tooHigh) {
        m_alarmDot->setStyleSheet("color: #ffaa00; font-size: 11pt; background: transparent;");
        m_alarmDot->setToolTip(
            QString("ALARM HIGH: %.3f Hz (max %.3f Hz)").arg(m_rate).arg(mx));
    } else {
        m_alarmDot->setStyleSheet("color: #30cc30; font-size: 11pt; background: transparent;");
        m_alarmDot->setToolTip(QString("OK: %1 Hz").arg(m_rate, 0, 'f', 3));
    }
}
