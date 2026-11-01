#include "vsm/device/SerialTransport.h"

#include <QLoggingCategory>
#include <QSerialPort>

Q_LOGGING_CATEGORY(lcSerial, "vsm.device.serial")

namespace vsm::device {

SerialTransport::SerialTransport(QString portName, qint32 baudRate, QObject* parent)
    : ITransport(parent)
    , m_port(new QSerialPort(this))
    , m_portName(std::move(portName))
    , m_baudRate(baudRate)
{
    connect(m_port, &QSerialPort::readyRead, this, [this] { emit dataReceived(m_port->readAll()); });

    connect(m_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error == QSerialPort::NoError) {
            return;
        }
        qCWarning(lcSerial) << description() << m_port->errorString();
        emit errorOccurred(m_port->errorString());

        // ResourceError = device unplugged / port vanished: the port is unusable.
        if (error == QSerialPort::ResourceError) {
            close();
        }
    });
}

SerialTransport::~SerialTransport() = default;

void SerialTransport::open()
{
    if (m_state != State::Disconnected) {
        return;
    }
    setState(State::Connecting);

    m_port->setPortName(m_portName);
    m_port->setBaudRate(m_baudRate);
    m_port->setDataBits(QSerialPort::Data8);
    m_port->setParity(QSerialPort::NoParity);
    m_port->setStopBits(QSerialPort::OneStop);
    m_port->setFlowControl(QSerialPort::NoFlowControl);

    // On failure QSerialPort reports the reason through errorOccurred().
    if (m_port->open(QIODevice::ReadOnly)) {
        qCInfo(lcSerial) << "Opened" << description();
        setState(State::Connected);
    } else {
        setState(State::Disconnected);
    }
}

void SerialTransport::close()
{
    if (m_port->isOpen()) {
        m_port->close();
    }
    setState(State::Disconnected);
}

QString SerialTransport::description() const
{
    return QStringLiteral("serial://%1@%2").arg(m_portName).arg(m_baudRate);
}

void SerialTransport::setState(State state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged(state);
}

} // namespace vsm::device
