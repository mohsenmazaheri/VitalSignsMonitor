#pragma once

#include "vsm/core/VitalSigns.h"
#include "vsm/device/ITransport.h"
#include "vsm/device/LinkStatistics.h"

#include <QList>
#include <QObject>

#include <chrono>
#include <memory>
#include <optional>

class QTimer;

namespace vsm::protocol {
class FrameDecoder;
struct Frame;
} // namespace vsm::protocol

namespace vsm::device {

/// One live connection to a device: bytes in -> domain objects out.
///
/// Responsibilities:
///  - drives an ITransport (open, automatic reconnect after a delay),
///  - decodes the byte stream into frames and messages (vsm::protocol),
///  - maps wire messages to domain types (vsm::core::VitalSigns, ECG in mV),
///  - watchdog: reports a lost link if no valid frame arrives in time,
///  - keeps link statistics (CRC errors, sequence gaps, ...).
///
/// Threading: lives in the I/O thread (see DeviceController). All signals are
/// emitted from that thread; connect with Qt::AutoConnection from the GUI.
class DeviceSession final : public QObject {
    Q_OBJECT

public:
    static constexpr std::chrono::milliseconds kDefaultWatchdogTimeout{3000};
    static constexpr std::chrono::milliseconds kDefaultReconnectDelay{2000};
    static constexpr std::chrono::milliseconds kStatisticsInterval{1000};

    /// Takes ownership of @p transport (re-parented to this session, so both
    /// move to the I/O thread together).
    explicit DeviceSession(std::unique_ptr<ITransport> transport, QObject* parent = nullptr);
    ~DeviceSession() override;

    void setWatchdogTimeout(std::chrono::milliseconds timeout);
    void setReconnectDelay(std::chrono::milliseconds delay);

    [[nodiscard]] LinkStatistics statistics() const;
    [[nodiscard]] bool isLinkLost() const noexcept { return m_linkLost; }
    [[nodiscard]] QString description() const;

public slots:
    /// Opens the transport and starts the watchdog. Idempotent.
    void start();
    /// Closes the transport and stops all timers. Idempotent.
    void stop();

signals:
    void vitalsReceived(const vsm::core::VitalSigns& vitals);
    /// @param millivolts consecutive ECG samples, @param sampleRateHz their rate.
    void ecgSamplesReceived(const QList<float>& millivolts, int sampleRateHz);
    void connectionStateChanged(vsm::device::ITransport::State state);
    void linkLostChanged(bool lost);
    void statisticsChanged(const vsm::device::LinkStatistics& statistics);
    void errorOccurred(const QString& message);

private:
    void onDataReceived(const QByteArray& data);
    void onTransportStateChanged(ITransport::State state);
    void handleFrame(const protocol::Frame& frame);
    void trackSequence(std::uint16_t sequence);
    void setLinkLost(bool lost);

    ITransport* m_transport; // owned via QObject parent
    std::unique_ptr<protocol::FrameDecoder> m_decoder;
    QTimer* m_watchdog;
    QTimer* m_reconnectTimer;
    QTimer* m_statisticsTimer;

    std::optional<std::uint16_t> m_lastSequence;
    quint64 m_sequenceGaps{0};
    quint64 m_framesLost{0};
    quint64 m_malformedMessages{0};
    bool m_running{false};
    bool m_linkLost{false};
};

} // namespace vsm::device

Q_DECLARE_METATYPE(vsm::core::VitalSigns)
