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

[[nodiscard]] core::VitalSigns toDomain(const protocol::NumericsMessage& message)
{
    core::VitalSigns vitals;
    if (message.heartRateBpm) {
        vitals.heartRateBpm = static_cast<int>(*message.heartRateBpm);
    }
    if (message.spo2Percent) {
        vitals.spo2Percent = static_cast<int>(*message.spo2Percent);
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
    , m_watchdog(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_statisticsTimer(new QTimer(this))
{
    Q_ASSERT(m_transport != nullptr);
    m_transport->setParent(this);

    m_watchdog->setSingleShot(true);
    m_watchdog->setInterval(kDefaultWatchdogTimeout);
    connect(m_watchdog, &QTimer::timeout, this, [this] {
        qCWarning(lcSession) << "No valid data for" << m_watchdog->interval() << "ms from" << description();
        setLinkLost(true);
    });

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
    m_watchdog->setInterval(timeout);
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

    m_watchdog->start();
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

    m_watchdog->stop();
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

    // Any valid message proves the device is alive.
    if (m_running) {
        m_watchdog->start();
    }
    setLinkLost(false);

    std::visit(
        [this](const auto& msg) {
            using T = std::decay_t<decltype(msg)>;
            if constexpr (std::is_same_v<T, protocol::NumericsMessage>) {
                emit vitalsReceived(toDomain(msg));
            } else if constexpr (std::is_same_v<T, protocol::EcgWaveformMessage>) {
                emit ecgSamplesReceived(toMillivolts(msg.samplesMicrovolts), msg.sampleRateHz);
            }
            // HeartbeatMessage: only feeds the watchdog (done above).
        },
        *message);
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
