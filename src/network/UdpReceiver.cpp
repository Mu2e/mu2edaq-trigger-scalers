#include "UdpReceiver.h"
#include "TriggerMessage.h"
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <cstring>

UdpReceiver::UdpReceiver(const QString& bindAddress, quint16 port, QObject* parent)
    : QObject(parent)
    , m_address(bindAddress == "0.0.0.0" ? QHostAddress::AnyIPv4 : QHostAddress(bindAddress))
    , m_port(port) {}

UdpReceiver::~UdpReceiver() {
    stop();
}

bool UdpReceiver::start() {
    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpReceiver::onReadyRead);

    if (!m_socket->bind(m_address, m_port,
                        QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
        emit errorOccurred(QString("UDP bind to %1:%2 failed: %3")
                               .arg(m_address.toString())
                               .arg(m_port)
                               .arg(m_socket->errorString()));
        return false;
    }
    return true;
}

void UdpReceiver::stop() {
    if (m_socket) {
        m_socket->close();
    }
}

void UdpReceiver::onReadyRead() {
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        const QByteArray data = datagram.data();

        if (data.size() < static_cast<int>(sizeof(TriggerMessage))) {
            continue;  // Ignore malformed packets
        }

        TriggerMessage msg;
        std::memcpy(&msg, data.constData(), sizeof(TriggerMessage));
        emit messageReceived(msg.category, msg.count);
    }
}
