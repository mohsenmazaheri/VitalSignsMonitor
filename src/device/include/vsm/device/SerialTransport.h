#pragma once

#include "vsm/device/ITransport.h"

class QSerialPort;

namespace vsm::device {

/// ITransport over a serial port (8 data bits, no parity, 1 stop bit, no flow control).
///
/// On Windows a virtual COM-port pair (e.g. com0com) connects the simulator
/// and the monitor without hardware.
class SerialTransport final : public ITransport {
    Q_OBJECT

public:
    SerialTransport(QString portName, qint32 baudRate, QObject* parent = nullptr);
    ~SerialTransport() override;

    void open() override;
    void close() override;
    [[nodiscard]] State state() const override { return m_state; }
    [[nodiscard]] QString description() const override;

private:
    void setState(State state);

    QSerialPort* m_port; // child object - moves with this transport between threads
    QString m_portName;
    qint32 m_baudRate;
    State m_state{State::Disconnected};
};

} // namespace vsm::device
