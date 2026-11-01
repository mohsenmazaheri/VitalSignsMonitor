#include "vsm/device/TcpTransport.h"

#include <QLoggingCategory>
#include <QTcpSocket>

Q_LOGGING_CATEGORY(lcTcp, "vsm.device.tcp")

namespace vsm::device {

TcpTransport::TcpTransport(QString host, quint16 port, QObject* parent)
    : ITransport(parent)
    , m_socket(new QTcpSocket(this))
    , m_host(std::move(host))
    , m_port(port)
{
    // Low latency matters more than throughput for a live waveform.
    m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    connect(m_socket, &QTcpSocket::readyRead, this, [this] { emit dataReceived(m_socket->readAll()); });

    // QTcpSocket::disconnected() is not emitted when a connection attempt
    // fails, so the socket state is the single source of truth.
    connect(m_socket, &QAbstractSocket::stateChanged, this, [this](QAbstractSocket::SocketState socketState) {
        switch (socketState) {
        case QAbstractSocket::UnconnectedState:
            setState(State::Disconnected);
            break;
        case QAbstractSocket::HostLookupState:
        case QAbstractSocket::ConnectingState:
            setState(State::Connecting);
            break;
        case QAbstractSocket::ConnectedState:
            setState(State::Connected);
            break;
        default:
            break; // Bound/Listening/Closing: transient, no change for our clients
        }
    });

    connect(m_socket, &QAbstractSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        qCWarning(lcTcp) << description() << m_socket->errorString();
        emit errorOccurred(m_socket->errorString());
    });
}

TcpTransport::~TcpTransport() = default;

void TcpTransport::open()
{
    if (m_state != State::Disconnected) {
        return;
    }
    qCInfo(lcTcp) << "Connecting to" << description();
    m_socket->connectToHost(m_host, m_port);
}

void TcpTransport::close()
{
    m_socket->abort();
    setState(State::Disconnected);
}

QString TcpTransport::description() const
{
    return QStringLiteral("tcp://%1:%2").arg(m_host).arg(m_port);
}

void TcpTransport::setState(State state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged(state);
}

} // namespace vsm::device
