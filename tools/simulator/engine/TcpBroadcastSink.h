#pragma once

#include "IByteSink.h"

#include <QHostAddress>
#include <QList>

class QTcpServer;
class QTcpSocket;

namespace vsm::simulator {

/// TCP server that sends the same stream to every connected monitor.
class TcpBroadcastSink final : public IByteSink {
    Q_OBJECT

public:
    TcpBroadcastSink(QHostAddress address, quint16 port, QObject* parent = nullptr);
    ~TcpBroadcastSink() override;

    [[nodiscard]] bool open(QString& error) override;
    void close() override;
    [[nodiscard]] bool isOpen() const override;
    void write(const QByteArray& bytes) override;
    [[nodiscard]] QString description() const override;
    [[nodiscard]] QString status() const override;
    [[nodiscard]] bool hasReceiver() const override { return !m_clients.isEmpty(); }

    [[nodiscard]] int clientCount() const noexcept { return static_cast<int>(m_clients.size()); }

private:
    void onNewConnection();

    QTcpServer* m_server;
    QList<QTcpSocket*> m_clients;
    QHostAddress m_address;
    quint16 m_port;
};

} // namespace vsm::simulator
