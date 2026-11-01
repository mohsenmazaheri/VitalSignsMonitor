#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

namespace vsm::device {

/// Abstract byte stream to the device (Dependency Inversion Principle).
///
/// DeviceSession only knows this interface, so TCP, serial port or a test
/// fake can be used without changing the session. Implementations only move
/// bytes - framing, validation and reconnect policy live in DeviceSession.
///
/// Threading: an ITransport lives in the thread of its owner (the I/O thread
/// in production) and must only be used from that thread.
class ITransport : public QObject {
    Q_OBJECT

public:
    enum class State {
        Disconnected,
        Connecting,
        Connected,
    };
    Q_ENUM(State)

    using QObject::QObject;
    ~ITransport() override = default;

    /// Starts connecting. Progress is reported through stateChanged().
    virtual void open() = 0;
    /// Closes the connection immediately; emits stateChanged(Disconnected).
    virtual void close() = 0;

    [[nodiscard]] virtual State state() const = 0;
    /// Human-readable endpoint, e.g. "tcp://127.0.0.1:5555" or "serial://COM5@115200".
    [[nodiscard]] virtual QString description() const = 0;

signals:
    void dataReceived(const QByteArray& data);
    void stateChanged(vsm::device::ITransport::State state);
    void errorOccurred(const QString& message);
};

} // namespace vsm::device
