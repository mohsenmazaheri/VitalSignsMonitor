#include "TcpBroadcastSink.h"

#include <QTcpServer>
#include <QTcpSocket>

#include <utility>

namespace vsm::simulator {

namespace {

/// A slow client must not make the simulator buffer without limit.
constexpr qint64 kMaxPendingBytesPerClient = 256 * 1024;

} // namespace

TcpBroadcastSink::TcpBroadcastSink(QHostAddress address, quint16 port, QObject* parent)
    : IByteSink(parent)
    , m_server(new QTcpServer(this))
    , m_address(std::move(address))
    , m_port(port)
{
    connect(m_server, &QTcpServer::newConnection, this, &TcpBroadcastSink::onNewConnection);
}

TcpBroadcastSink::~TcpBroadcastSink()
{
    // The client sockets are children and die with us; their "disconnected"
    // signal must not call back into a half-destroyed object.
    for (QTcpSocket* client : std::as_const(m_clients)) {
        client->disconnect(this);
    }
}

bool TcpBroadcastSink::open(QString& error)
{
    if (m_server->isListening()) {
        return true;
    }
    if (!m_server->listen(m_address, m_port)) {
        error = m_server->errorString();
        return false;
    }
    emit statusChanged();
    return true;
}

void TcpBroadcastSink::close()
{
    const QList<QTcpSocket*> clients = std::exchange(m_clients, {});
    for (QTcpSocket* client : clients) {
        client->disconnect(this);
        client->abort();
        client->deleteLater();
    }
    m_server->close();
    emit statusChanged();
}

bool TcpBroadcastSink::isOpen() const
{
    return m_server->isListening();
}

void TcpBroadcastSink::write(const QByteArray& bytes)
{
    for (QTcpSocket* client : std::as_const(m_clients)) {
        if (client->bytesToWrite() > kMaxPendingBytesPerClient) {
            continue; // drop data for this client; the monitor will count a sequence gap
        }
        client->write(bytes);
    }
}

QString TcpBroadcastSink::description() const
{
    return QStringLiteral("tcp://%1:%2").arg(m_address.toString()).arg(m_port);
}

QString TcpBroadcastSink::status() const
{
    if (!isOpen()) {
        return QStringLiteral("Closed");
    }
    if (m_clients.isEmpty()) {
        return QStringLiteral("Listening, no monitor connected");
    }
    return m_clients.size() == 1 ? QStringLiteral("1 monitor connected")
                                 : QStringLiteral("%1 monitors connected").arg(m_clients.size());
}

void TcpBroadcastSink::onNewConnection()
{
    while (QTcpSocket* client = m_server->nextPendingConnection()) {
        client->setSocketOption(QAbstractSocket::LowDelayOption, 1);
        m_clients.append(client);

        connect(client, &QTcpSocket::disconnected, this, [this, client] {
            m_clients.removeOne(client);
            client->deleteLater();
            emit statusChanged();
        });
        emit statusChanged();
    }
}

} // namespace vsm::simulator
