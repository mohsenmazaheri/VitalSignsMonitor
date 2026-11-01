#include "MonitorViewModel.h"

#include <QSerialPortInfo>

namespace vsm::app {

namespace {

constexpr int kDefaultEcgSampleRateHz = 250;
constexpr double kEcgSweepSeconds = 6.0;

} // namespace

MonitorViewModel::MonitorViewModel(device::DeviceController& device, core::AlarmEvaluator evaluator, QObject* parent)
    : QObject(parent)
    , m_device(device)
    , m_evaluator(std::move(evaluator))
    , m_alarms(this) // parented so QML never assumes ownership
    , m_ecg(kDefaultEcgSampleRateHz, kEcgSweepSeconds, this)
{
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
    m_device.disconnectFromDevice();
    m_endpoint.clear();
    m_connectionState = device::ITransport::State::Disconnected;
    m_linkLost = false;
    resetPatientData();
    emit connectionChanged();
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
    resetPatientData();
    m_linkLost = false;
    m_connectionState = device::ITransport::State::Connecting;
    m_endpoint = settings.kind == device::ConnectionSettings::Kind::Tcp
                     ? QStringLiteral("%1:%2").arg(settings.host).arg(settings.tcpPort)
                     : settings.serialPortName;
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
    m_connectionState = state;
    emit connectionChanged();
}

void MonitorViewModel::onLinkLost(bool lost)
{
    if (!m_device.hasSession() || m_linkLost == lost) {
        return;
    }
    m_linkLost = lost;
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
    m_alarms.setAlarms(m_evaluator.evaluate(context));
    emit alarmsChanged();
}

int MonitorViewModel::priorityFor(core::AlarmSource source) const
{
    for (const core::Alarm& alarm : m_alarms.alarms()) {
        if (alarm.source == source) {
            return static_cast<int>(alarm.priority);
        }
    }
    return 0;
}

} // namespace vsm::app
