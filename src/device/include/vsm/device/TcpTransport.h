#pragma once

#include "vsm/device/ITransport.h"

class QTcpSocket;

namespace vsm::device {

/// ITransport over a TCP client socket (e.g. to the simulator or a device gateway).
class TcpTransport final : public ITransport {
    Q_OBJECT

public:
    TcpTransport(QString host, quint16 port, QObject* parent = nullptr);
    ~TcpTransport() override;

    void open() override;
    void close() override;
    [[nodiscard]] State state() const override { return m_state; }
    [[nodiscard]] QString description() const override;

private:
    void setState(State state);

    QTcpSocket* m_socket; // child object - moves with this transport between threads
    QString m_host;
    quint16 m_port;
    State m_state{State::Disconnected};
};

} // namespace vsm::device
