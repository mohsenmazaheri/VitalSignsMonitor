#include "MonitorViewModel.h"

#include <QSerialPortInfo>

#include <algorithm>
#include <chrono>

namespace vsm::app {

namespace {

constexpr int kDefaultEcgSampleRateHz = 250;
constexpr double kEcgSweepSeconds = 6.0;
constexpr int kTickIntervalMs = 500; // audio pause countdown

} // namespace

MonitorViewModel::MonitorViewModel(device::DeviceController& device, core::AlarmEvaluator evaluator,
                                   core::AlarmManager& alarmManager, IAlarmSound& sound, AuditLogModel& auditLog,
                                   QObject* parent)
    : QObject(parent)
    , m_device(device)
    , m_evaluator(std::move(evaluator))
    , m_alarmManager(alarmManager)
    , m_sound(sound)
    , m_auditLog(auditLog)
    , m_alarms(this) // parented so QML never assumes ownership
    , m_ecg(kDefaultEcgSampleRateHz, kEcgSweepSeconds, this)
{
    m_clock.start();
    m_ticker.setInterval(kTickIntervalMs);
    connect(&m_ticker, &QTimer::timeout, this, &MonitorViewModel::onTick);
    m_ticker.start();

    using device::DeviceController;
    connect(&m_device, &DeviceController::vitalsReceived, this, &MonitorViewModel::onVitals);
    connect(&m_device, &DeviceController::ecgSamplesReceived, &m_ecg, &ui::WaveformSeries::append);
    connect(&m_device, &DeviceController::connectionStateChanged, this, &MonitorViewModel::onConnectionState);
    connect(&m_device, &DeviceController::linkLostChanged, this, &MonitorViewModel::onLinkLost);
    connect(&m_device, &DeviceController::statisticsChanged, this, &MonitorViewModel::onStatistics);
    connect(&m_device, &DeviceController::errorOccurred, this, &MonitorViewModel::setLastError);
}

// --- Commands from QML ---------------------------------------------------------

void MonitorViewModel::connectTcp(const QString& host, int port)
{
    if (host.trimmed().isEmpty() || port <= 0 || port > 65535) {
        setLastError(tr("Enter a valid host and a port between 1 and 65535."));
        return;
    }
    device::ConnectionSettings settings;
    settings.kind = device::ConnectionSettings::Kind::Tcp;
    settings.host = host.trimmed();
    settings.tcpPort = static_cast<quint16>(port);
    connectWith(settings);
}

void MonitorViewModel::connectSerial(const QString& portName, int baudRate)
{
    if (portName.isEmpty() || baudRate <= 0) {
        setLastError(tr("Select a serial port and a baud rate."));
        return;
    }
    device::ConnectionSettings settings;
    settings.kind = device::ConnectionSettings::Kind::Serial;
    settings.serialPortName = portName;
    settings.baudRate = baudRate;
    connectWith(settings);
}

void MonitorViewModel::disconnectDevice()
{
    if (m_device.hasSession()) {
        recordEvent(core::AuditEventType::DisconnectRequested, m_endpoint);
    }
    m_device.disconnectFromDevice();
    m_endpoint.clear();
    m_connectionState = device::ITransport::State::Disconnected;
    m_linkLost = false;
    m_alarmManager.clear(); // a new patient session must not inherit latched alarms
    resetPatientData();
    emit connectionChanged();
}

void MonitorViewModel::acknowledgeAlarms()
{
    m_alarmManager.acknowledgeAll();
    publishAlarms();
}

void MonitorViewModel::toggleAudioPause()
{
    if (m_alarmManager.audioPaused()) {
        m_alarmManager.resumeAudio();
    } else {
        m_alarmManager.pauseAudio(now());
    }
    publishAlarms();
    emit audioPauseTimeChanged();
}

QStringList MonitorViewModel::availableSerialPorts() const
{
    QStringList names;
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : ports) {
        names.append(info.portName());
    }
    return names;
}

// --- Property helpers ----------------------------------------------------------

int MonitorViewModel::heartRateAlarmPriority() const
{
    return priorityFor(core::AlarmSource::HeartRate);
}

int MonitorViewModel::spo2AlarmPriority() const
{
    return priorityFor(core::AlarmSource::Spo2);
}

bool MonitorViewModel::alarmFlashing() const
{
    // Only an unacknowledged high-priority alarm flashes: acknowledging calms the screen.
    const auto& alarms = m_alarmManager.alarms();
    return std::ranges::any_of(alarms, [](const core::ManagedAlarm& managed) {
        return managed.alarm.priority == core::AlarmPriority::High && managed.state != core::AlarmState::Acknowledged;
    });
}

QString MonitorViewModel::audioPauseRemaining() const
{
    using namespace std::chrono;
    const auto left = ceil<seconds>(m_alarmManager.audioPauseRemaining(now())).count();
    return QStringLiteral("%1:%2").arg(left / 60).arg(left % 60, 2, 10, QLatin1Char('0'));
}

QString MonitorViewModel::connectionStatus() const
{
    using State = device::ITransport::State;
    if (!m_device.hasSession()) {
        return tr("Not connected");
    }
    switch (m_connectionState) {
    case State::Connecting:
        return tr("Connecting to %1 ...").arg(m_endpoint);
    case State::Connected:
        return m_linkLost ? tr("Connected to %1 - no data").arg(m_endpoint) : tr("Connected to %1").arg(m_endpoint);
    case State::Disconnected:
        break;
    }
    return tr("Disconnected from %1 - retrying").arg(m_endpoint);
}

QString MonitorViewModel::version()
{
    return QStringLiteral(VSM_VERSION);
}

// --- Device events -------------------------------------------------------------

void MonitorViewModel::connectWith(const device::ConnectionSettings& settings)
{
    setLastError({});
    m_alarmManager.clear();
    resetPatientData();
    m_linkLost = false;
    m_connectionState = device::ITransport::State::Connecting;
    m_endpoint = settings.kind == device::ConnectionSettings::Kind::Tcp
                     ? QStringLiteral("%1:%2").arg(settings.host).arg(settings.tcpPort)
                     : settings.serialPortName;
    recordEvent(core::AuditEventType::ConnectRequested, m_endpoint);
    m_device.connectToDevice(settings);
    emit connectionChanged();
}

void MonitorViewModel::onVitals(const core::VitalSigns& vitals)
{
    if (vitals == m_vitals) {
        return;
    }
    m_vitals = vitals;
    emit vitalsChanged();
    reevaluateAlarms();
}

void MonitorViewModel::onConnectionState(device::ITransport::State state)
{
    if (!m_device.hasSession()) {
        return; // late signal from a session that was already stopped
    }
    using State = device::ITransport::State;
    // Only real changes: the reconnect loop must not fill the log every 2 seconds.
    if (state == State::Connected && m_connectionState != State::Connected) {
        recordEvent(core::AuditEventType::Connected, m_endpoint);
    } else if (state == State::Disconnected && m_connectionState == State::Connected) {
        recordEvent(core::AuditEventType::ConnectionDropped, m_endpoint);
    }
    m_connectionState = state;
    emit connectionChanged();
}

void MonitorViewModel::onLinkLost(bool lost)
{
    if (!m_device.hasSession() || m_linkLost == lost) {
        return;
    }
    m_linkLost = lost;
    recordEvent(lost ? core::AuditEventType::LinkLost : core::AuditEventType::LinkRestored, m_endpoint);
    if (lost) {
        // Stale numbers must never be shown as if they were current.
        m_vitals = {};
        emit vitalsChanged();
    }
    emit connectionChanged();
    reevaluateAlarms();
}

void MonitorViewModel::onStatistics(const device::LinkStatistics& statistics)
{
    m_linkStatistics = tr("Frames %1 | CRC errors %2 | Lost frames %3 in %4 gaps | Discarded bytes %5")
                           .arg(statistics.framesReceived)
                           .arg(statistics.crcErrors)
                           .arg(statistics.framesLost)
                           .arg(statistics.sequenceGaps)
                           .arg(statistics.bytesDiscarded);
    emit linkStatisticsChanged();
}

void MonitorViewModel::setLastError(const QString& message)
{
    if (m_lastError == message) {
        return;
    }
    m_lastError = message;
    emit lastErrorChanged();
}

void MonitorViewModel::resetPatientData()
{
    m_vitals = {};
    m_ecg.clear();
    emit vitalsChanged();
    reevaluateAlarms();
}

void MonitorViewModel::reevaluateAlarms()
{
    const core::AlarmContext context{m_vitals, m_device.hasSession() && m_linkLost};
    m_alarmManager.update(m_evaluator.evaluate(context), now());
    publishAlarms();
}

void MonitorViewModel::publishAlarms()
{
    m_alarms.setAlarms(m_alarmManager.alarms());
    m_sound.setPriority(m_alarmManager.soundingPriority());
    emit alarmsChanged();
}

void MonitorViewModel::onTick()
{
    if (m_alarmManager.audioPaused()) {
        m_alarmManager.tick(now());
        if (!m_alarmManager.audioPaused()) {
            publishAlarms(); // the pause just ended: the sound comes back
        }
        emit audioPauseTimeChanged();
    }
}

void MonitorViewModel::recordEvent(core::AuditEventType type, const QString& detail)
{
    m_auditLog.record({type, {}, std::nullopt, detail.toStdString()});
}

core::AlarmManager::Milliseconds MonitorViewModel::now() const
{
    return core::AlarmManager::Milliseconds(m_clock.elapsed());
}

int MonitorViewModel::priorityFor(core::AlarmSource source) const
{
    // A latched alarm is about the past: the tile shows the present value, so it
    // only takes an alarm colour for a condition that exists now.
    int priority = 0;
    for (const core::ManagedAlarm& managed : m_alarmManager.alarms()) {
        if (managed.alarm.source == source && managed.state != core::AlarmState::Latched) {
            priority = std::max(priority, static_cast<int>(managed.alarm.priority));
        }
    }
    return priority;
}

} // namespace vsm::app
