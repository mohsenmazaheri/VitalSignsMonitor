#include "vsm/device/DeviceSession.h"

#include "vsm/protocol/FrameDecoder.h"
#include "vsm/protocol/Messages.h"

#include <QLoggingCategory>
#include <QTimer>

#include <span>
#include <type_traits>

Q_LOGGING_CATEGORY(lcSession, "vsm.device.session")

namespace vsm::device {

namespace {

constexpr float kMicrovoltsPerMillivolt = 1000.0F;

// A correct CRC only proves the bytes arrived unchanged, not that the device
// measured something possible. An impossible number must not look like a reading:
// SpO2 150 would otherwise be shown as a value and raise no alarm at all.
constexpr int kMaxPlausibleHeartRateBpm = 300;
constexpr int kMaxPlausibleSpo2Percent = 100;

[[nodiscard]] core::VitalSigns toDomain(const protocol::NumericsMessage& message, quint64& implausibleValues)
{
    core::VitalSigns vitals;
    if (message.heartRateBpm) {
        if (*message.heartRateBpm <= kMaxPlausibleHeartRateBpm) {
            vitals.heartRateBpm = static_cast<int>(*message.heartRateBpm);
        } else {
            ++implausibleValues;
        }
    }
    if (message.spo2Percent) {
        if (*message.spo2Percent <= kMaxPlausibleSpo2Percent) {
            vitals.spo2Percent = static_cast<int>(*message.spo2Percent);
        } else {
            ++implausibleValues;
        }
    }
    vitals.ecgLeadsOff = message.ecgLeadsOff;
    vitals.spo2ProbeOff = message.spo2ProbeOff;
    return vitals;
}

[[nodiscard]] QList<float> toMillivolts(const std::vector<std::int16_t>& microvolts)
{
    QList<float> millivolts;
    millivolts.reserve(static_cast<qsizetype>(microvolts.size()));
    for (const std::int16_t sample : microvolts) {
        millivolts.append(static_cast<float>(sample) / kMicrovoltsPerMillivolt);
    }
    return millivolts;
}

} // namespace

DeviceSession::DeviceSession(std::unique_ptr<ITransport> transport, QObject* parent)
    : QObject(parent)
    , m_transport(transport.release())
    , m_decoder(std::make_unique<protocol::FrameDecoder>())
    , m_numericsWatchdog(new QTimer(this))
    , m_ecgWatchdog(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_statisticsTimer(new QTimer(this))
{
    Q_ASSERT(m_transport != nullptr);
    m_transport->setParent(this);

    m_numericsWatchdog->setSingleShot(true);
    m_numericsWatchdog->setInterval(kDefaultWatchdogTimeout);
    connect(m_numericsWatchdog, &QTimer::timeout, this, [this] { onDataExpired("numerics"); });

    m_ecgWatchdog->setSingleShot(true);
    m_ecgWatchdog->setInterval(kDefaultWatchdogTimeout);
    connect(m_ecgWatchdog, &QTimer::timeout, this, [this] { onDataExpired("ECG"); });

    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->setInterval(kDefaultReconnectDelay);
    connect(m_reconnectTimer, &QTimer::timeout, m_transport, &ITransport::open);

    m_statisticsTimer->setInterval(kStatisticsInterval);
    connect(m_statisticsTimer, &QTimer::timeout, this, [this] { emit statisticsChanged(statistics()); });

    connect(m_transport, &ITransport::dataReceived, this, &DeviceSession::onDataReceived);
    connect(m_transport, &ITransport::stateChanged, this, &DeviceSession::onTransportStateChanged);
    connect(m_transport, &ITransport::errorOccurred, this, &DeviceSession::errorOccurred);
}

DeviceSession::~DeviceSession()
{
    stop();
}

void DeviceSession::setWatchdogTimeout(std::chrono::milliseconds timeout)
{
    m_numericsWatchdog->setInterval(timeout);
    m_ecgWatchdog->setInterval(timeout);
}

void DeviceSession::setReconnectDelay(std::chrono::milliseconds delay)
{
    m_reconnectTimer->setInterval(delay);
}

LinkStatistics DeviceSession::statistics() const
{
    const auto& decoder = m_decoder->statistics();
    LinkStatistics stats;
    stats.framesReceived = decoder.framesDecoded;
    stats.crcErrors = decoder.crcErrors;
    stats.bytesDiscarded = decoder.bytesDiscarded;
    stats.sequenceGaps = m_sequenceGaps;
    stats.framesLost = m_framesLost;
    stats.malformedMessages = m_malformedMessages;
    stats.implausibleValues = m_implausibleValues;
    return stats;
}

QString DeviceSession::description() const
{
    return m_transport->description();
}

void DeviceSession::start()
{
    if (m_running) {
        return;
    }
    m_running = true;
    qCInfo(lcSession) << "Starting session" << description();

    m_numericsWatchdog->start();
    m_ecgWatchdog->start();
    m_statisticsTimer->start();
    m_transport->open();
}

void DeviceSession::stop()
{
    if (!m_running) {
        return;
    }
    m_running = false; // set first: the Disconnected callback must not reconnect
    qCInfo(lcSession) << "Stopping session" << description();

    m_numericsWatchdog->stop();
    m_ecgWatchdog->stop();
    m_reconnectTimer->stop();
    m_statisticsTimer->stop();
    m_transport->close();
    setLinkLost(false); // a stopped session is "off", not "lost"
    emit statisticsChanged(statistics());
}

void DeviceSession::onDataReceived(const QByteArray& data)
{
    const auto bytes = std::span(reinterpret_cast<const std::uint8_t*>(data.constData()),
                                 static_cast<std::size_t>(data.size()));
    for (const protocol::Frame& frame : m_decoder->feed(bytes)) {
        handleFrame(frame);
    }
}

void DeviceSession::onTransportStateChanged(ITransport::State state)
{
    emit connectionStateChanged(state);

    if (state == ITransport::State::Connected) {
        // A new connection is a new stream: forget the last sequence number of
        // the previous one so the reconnect is not counted as a gap. A partial
        // frame left in the decoder is dropped by its normal re-sync logic.
        m_lastSequence.reset();
    } else if (state == ITransport::State::Disconnected && m_running) {
        qCInfo(lcSession) << "Disconnected, retrying in" << m_reconnectTimer->interval() << "ms";
        m_reconnectTimer->start();
    }
}

void DeviceSession::handleFrame(const protocol::Frame& frame)
{
    trackSequence(frame.sequence);

    const std::optional<protocol::Message> message = protocol::parseMessage(frame);
    if (!message) {
        ++m_malformedMessages;
        return;
    }

    // While the link counts as lost, a partly working device is not trusted:
    // nothing it sends reaches the display until numerics and ECG are both back.
    std::visit(
        [this](const auto& msg) {
            using T = std::decay_t<decltype(msg)>;
            if constexpr (std::is_same_v<T, protocol::NumericsMessage>) {
                noteDataArrived(m_numericsWatchdog);
                const quint64 implausibleBefore = m_implausibleValues;
                const core::VitalSigns vitals = toDomain(msg, m_implausibleValues);
                if (m_implausibleValues != implausibleBefore) {
                    qCWarning(lcSession) << "Implausible numerics from" << description();
                }
                if (!m_linkLost) {
                    emit vitalsReceived(vitals);
                }
            } else if constexpr (std::is_same_v<T, protocol::EcgWaveformMessage>) {
                noteDataArrived(m_ecgWatchdog);
                if (!m_linkLost) {
                    emit ecgSamplesReceived(toMillivolts(msg.samplesMicrovolts), msg.sampleRateHz);
                }
            }
            // HeartbeatMessage: deliberately feeds no watchdog. A device that still
            // says "I am alive" but measures nothing must not keep old numbers on screen.
        },
        *message);
}

void DeviceSession::onDataExpired(const char* kind)
{
    qCWarning(lcSession) << "No" << kind << "data for" << m_numericsWatchdog->interval() << "ms from" << description();
    setLinkLost(true);
}

void DeviceSession::noteDataArrived(QTimer* watchdog)
{
    if (m_running) {
        watchdog->start();
    }
    if (m_numericsWatchdog->isActive() && m_ecgWatchdog->isActive()) {
        setLinkLost(false);
    }
}

void DeviceSession::trackSequence(std::uint16_t sequence)
{
    if (m_lastSequence && sequence != static_cast<std::uint16_t>(*m_lastSequence + 1)) {
        ++m_sequenceGaps;
        // uint16 arithmetic handles the wrap-around 65535 -> 0.
        m_framesLost += static_cast<std::uint16_t>(sequence - *m_lastSequence - 1);
    }
    m_lastSequence = sequence;
}

void DeviceSession::setLinkLost(bool lost)
{
    if (m_linkLost == lost) {
        return;
    }
    m_linkLost = lost;
    emit linkLostChanged(lost);
}

} // namespace vsm::device
