#pragma once

#include "IByteSink.h"

class QSerialPort;

namespace vsm::simulator {

/// Writes the stream to a serial port (8N1, no flow control) - the way a real
/// bedside device with an RS-232 / USB-serial interface would send it.
class SerialSink final : public IByteSink {
    Q_OBJECT

public:
    SerialSink(QString portName, qint32 baudRate, QObject* parent = nullptr);

    [[nodiscard]] bool open(QString& error) override;
    void close() override;
    [[nodiscard]] bool isOpen() const override;
    void write(const QByteArray& bytes) override;
    [[nodiscard]] QString description() const override;
    [[nodiscard]] QString status() const override;
    [[nodiscard]] bool hasReceiver() const override { return isOpen(); }

private:
    void onPortError();

    QSerialPort* m_port;
    QString m_portName;
    qint32 m_baudRate;
};

} // namespace vsm::simulator
