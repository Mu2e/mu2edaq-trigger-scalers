#pragma once

#include <QThread>
#include <QString>
#include <atomic>
#include <cstdint>

// Receives TriggerMessages via a ZeroMQ socket in a dedicated background thread.
//
// socket_type "sub"  — connects to a PUB socket (broadcast fan-out).
//                      Subscribes to all topics (empty prefix filter).
// socket_type "pull" — connects to a PUSH socket (pipeline).
//
// The endpoint should be the address of the publishing DAQ process, e.g.:
//   "tcp://daq-host:5556"
class ZmqReceiver : public QThread {
    Q_OBJECT
public:
    explicit ZmqReceiver(const QString& endpoint, const QString& socketType,
                         QObject* parent = nullptr);
    ~ZmqReceiver() override;

    // Request the receive loop to exit and wait up to 2 s for the thread.
    void stop();

signals:
    void messageReceived(quint32 category, quint64 value);
    void errorOccurred(const QString& error);

protected:
    void run() override;

private:
    QString m_endpoint;
    QString m_socketType;
    std::atomic<bool> m_stopping{false};
};
