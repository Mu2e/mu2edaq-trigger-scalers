#pragma once

#include <QObject>
#include <QHostAddress>
#include <cstdint>

class QUdpSocket;

// Receives TriggerMessage datagrams on a UDP port.
// Supports both unicast and broadcast (bind to 0.0.0.0).
// Runs in the caller's event loop; no separate thread required.
class UdpReceiver : public QObject {
    Q_OBJECT
public:
    explicit UdpReceiver(const QString& bindAddress, quint16 port, QObject* parent = nullptr);
    ~UdpReceiver() override;

    bool start();
    void stop();

signals:
    void messageReceived(quint32 category, quint64 value);
    void errorOccurred(const QString& error);

private slots:
    void onReadyRead();

private:
    QUdpSocket* m_socket{nullptr};
    QHostAddress m_address;
    quint16 m_port;
};
