#pragma once

#include "vsm/device/ConnectionSettings.h"
#include "vsm/device/DeviceSession.h"
#include "vsm/device/TransportFactory.h"

#include <QObject>
#include <QThread>

namespace vsm::device {

/// GUI-thread facade for the device I/O (Facade pattern).
///
/// Owns a dedicated I/O thread and at most one DeviceSession living in it.
/// Parsing and socket handling therefore never block the UI, and the UI never
/// touches the session directly - it only sees the signals relayed here
/// (queued across the thread boundary by Qt).
class DeviceController final : public QObject {
    Q_OBJECT

public:
    /// @param transportFactory creates the transport for each connection;
    ///        tests inject a factory that returns fake transports.
    explicit DeviceController(TransportFactory transportFactory = createDefaultTransport,
                              QObject* parent = nullptr);
    ~DeviceController() override;

    DeviceController(const DeviceController&) = delete;
    DeviceController& operator=(const DeviceController&) = delete;

    /// Stops any running session and starts a new one with @p settings.
    void connectToDevice(const ConnectionSettings& settings);
    /// Stops and destroys the running session (blocks until the I/O thread has closed it).
    void disconnectFromDevice();

    [[nodiscard]] bool hasSession() const noexcept { return m_session != nullptr; }

signals:
    void vitalsReceived(const vsm::core::VitalSigns& vitals);
    void ecgSamplesReceived(const QList<float>& millivolts, int sampleRateHz);
    void connectionStateChanged(vsm::device::ITransport::State state);
    void linkLostChanged(bool lost);
    void statisticsChanged(const vsm::device::LinkStatistics& statistics);
    void errorOccurred(const QString& message);

private:
    TransportFactory m_transportFactory;
    QThread m_ioThread;
    DeviceSession* m_session{nullptr};
};

} // namespace vsm::device
