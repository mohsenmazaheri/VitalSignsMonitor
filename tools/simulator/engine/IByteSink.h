#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

namespace vsm::simulator {

/// Destination for the encoded byte stream (TCP clients, serial port, test fake).
///
/// DeviceSimulator only depends on this interface (Dependency Inversion), so the
/// same generator feeds a TCP server, a serial port or an in-memory test sink.
/// It is a QObject so that sinks can report status changes and I/O errors
/// (e.g. a USB-serial adapter being unplugged) to the UI.
class IByteSink : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~IByteSink() override = default;

    IByteSink(const IByteSink&) = delete;
    IByteSink& operator=(const IByteSink&) = delete;
    IByteSink(IByteSink&&) = delete;
    IByteSink& operator=(IByteSink&&) = delete;

    /// Starts listening / opens the port. @return false and sets @p error on failure.
    [[nodiscard]] virtual bool open(QString& error) = 0;
    virtual void close() = 0;
    [[nodiscard]] virtual bool isOpen() const = 0;

    /// Sends @p bytes. Must never block; a sink that cannot keep up drops data.
    virtual void write(const QByteArray& bytes) = 0;

    /// Where the data goes, e.g. "tcp://127.0.0.1:5555" or "serial://COM5@115200".
    [[nodiscard]] virtual QString description() const = 0;
    /// Short human-readable state, e.g. "2 monitors connected".
    [[nodiscard]] virtual QString status() const = 0;
    /// True when at least one receiver can get the data (a TCP client is
    /// connected; for a serial port: the port is open, the far end is unknown).
    [[nodiscard]] virtual bool hasReceiver() const = 0;

signals:
    void statusChanged();
    /// A runtime I/O error. The sink closes itself when the error is fatal.
    void errorOccurred(const QString& message);
};

} // namespace vsm::simulator
