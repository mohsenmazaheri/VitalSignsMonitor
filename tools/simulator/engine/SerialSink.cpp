#include "SerialSink.h"

#include <QSerialPort>

namespace vsm::simulator {

namespace {

/// If nobody reads the far end of a virtual port pair, the driver buffer fills
/// up. Drop data instead of growing the write queue without limit.
constexpr qint64 kMaxPendingBytes = 64 * 1024;

} // namespace

SerialSink::SerialSink(QString portName, qint32 baudRate, QObject* parent)
    : IByteSink(parent)
    , m_port(new QSerialPort(this))
    , m_portName(std::move(portName))
    , m_baudRate(baudRate)
{
    connect(m_port, &QSerialPort::errorOccurred, this, &SerialSink::onPortError);
}

bool SerialSink::open(QString& error)
{
    if (m_port->isOpen()) {
        return true;
    }
    m_port->setPortName(m_portName);
    m_port->setBaudRate(m_baudRate);
    m_port->setDataBits(QSerialPort::Data8);
    m_port->setParity(QSerialPort::NoParity);
    m_port->setStopBits(QSerialPort::OneStop);
    m_port->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port->open(QIODevice::WriteOnly)) {
        error = m_port->errorString();
        m_port->clearError();
        return false;
    }
    emit statusChanged();
    return true;
}

void SerialSink::close()
{
    if (m_port->isOpen()) {
        m_port->close();
        emit statusChanged();
    }
}

bool SerialSink::isOpen() const
{
    return m_port->isOpen();
}

void SerialSink::write(const QByteArray& bytes)
{
    if (m_port->isOpen() && m_port->bytesToWrite() < kMaxPendingBytes) {
        m_port->write(bytes);
    }
}

QString SerialSink::description() const
{
    return QStringLiteral("serial://%1@%2").arg(m_portName).arg(m_baudRate);
}

QString SerialSink::status() const
{
    return isOpen() ? QStringLiteral("%1 open, %2 baud 8N1").arg(m_portName).arg(m_baudRate)
                    : QStringLiteral("Closed");
}

void SerialSink::onPortError()
{
    const QSerialPort::SerialPortError code = m_port->error();
    if (code == QSerialPort::NoError || !m_port->isOpen()) {
        return; // errors while opening are reported by open() itself
    }
    emit errorOccurred(m_port->errorString());
    // ResourceError = the adapter disappeared; the handle is useless now.
    if (code == QSerialPort::ResourceError || code == QSerialPort::PermissionError) {
        close();
    }
    m_port->clearError();
}

} // namespace vsm::simulator
