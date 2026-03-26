#include "ZmqReceiver.h"
#include "TriggerMessage.h"
#include <zmq.hpp>
#include <cstring>

ZmqReceiver::ZmqReceiver(const QString& endpoint, const QString& socketType, QObject* parent)
    : QThread(parent)
    , m_endpoint(endpoint)
    , m_socketType(socketType) {}

void ZmqReceiver::stop() {
    m_stopping = true;
    if (!wait(2000)) {
        terminate();
        wait();
    }
}

void ZmqReceiver::run() {
    m_stopping = false;

    try {
        zmq::context_t ctx(1);

        const zmq::socket_type type = (m_socketType == "pull")
            ? zmq::socket_type::pull
            : zmq::socket_type::sub;

        zmq::socket_t socket(ctx, type);

        // 100 ms receive timeout lets the loop check m_stopping regularly.
        socket.set(zmq::sockopt::rcvtimeo, 100);

        socket.connect(m_endpoint.toStdString());

        if (type == zmq::socket_type::sub) {
            socket.set(zmq::sockopt::subscribe, "");  // Subscribe to all topics
        }

        while (!m_stopping) {
            zmq::message_t msg;
            const auto result = socket.recv(msg, zmq::recv_flags::none);

            if (!result.has_value()) {
                // Timeout (EAGAIN) — loop back and check m_stopping
                continue;
            }

            if (msg.size() == sizeof(TriggerMessage)) {
                TriggerMessage tm;
                std::memcpy(&tm, msg.data(), sizeof(TriggerMessage));
                // Qt queues this to the main thread because the receiver lives there
                emit messageReceived(tm.category, tm.count);
            }
        }
    } catch (const zmq::error_t& e) {
        if (!m_stopping) {
            emit errorOccurred(QString("ZMQ error: %1").arg(e.what()));
        }
    }
}
