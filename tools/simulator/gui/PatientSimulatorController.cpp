#include "PatientSimulatorController.h"

#include <QSerialPortInfo>

#include <algorithm>
#include <cmath>

namespace vsm::simulator::gui {

namespace {

constexpr double kPreviewSweepSeconds = 4.0;

/// Serial 8N1 needs 10 bit times per byte (start + 8 data + stop).
constexpr int kBitsPerSerialByte = 10;

} // namespace

PatientSimulatorController::PatientSimulatorController(DeviceSimulator& simulator, SinkFactory sinkFactory,
                                                       QObject* parent)
    : QObject(parent)
    , m_simulator(simulator)
    , m_sinkFactory(std::move(sinkFactory))
    , m_ecg(simulator.sampleRateHz(), kPreviewSweepSeconds)
{
    connect(&m_simulator, &DeviceSimulator::runningChanged, this, [this](bool running) {
        if (!running) {
            m_ecg.clear(); // the screen of a switched-off device is blank
            m_bytesPerSecond = 0;
            emit liveChanged();
        }
        emit poweredChanged();
    });
    connect(&m_simulator, &DeviceSimulator::modeChanged, this, &PatientSimulatorController::modeChanged);
    connect(&m_simulator, &DeviceSimulator::phaseChanged, this, &PatientSimulatorController::phaseChanged);
    connect(&m_simulator, &DeviceSimulator::targetsChanged, this, &PatientSimulatorController::targetsChanged);
    connect(&m_simulator, &DeviceSimulator::ecgSamplesGenerated, &m_ecg, &ui::WaveformSeries::append);
    connect(&m_simulator, &DeviceSimulator::numericsGenerated, this, &PatientSimulatorController::onNumericsGenerated);
    connect(&m_simulator, &DeviceSimulator::transmissionEnabledChanged, this,
            &PatientSimulatorController::cableConnectedChanged);
    connect(&m_simulator, &DeviceSimulator::statisticsChanged, this, &PatientSimulatorController::statisticsChanged);
}

PatientSimulatorController::~PatientSimulatorController()
{
    // The simulator may outlive us; it must not write to a deleted sink.
    m_simulator.setSink(nullptr);
}

// --- Commands --------------------------------------------------------------------

bool PatientSimulatorController::runScenario(const QString& name)
{
    std::optional<Scenario> scenario = makeScenario(name.toStdString());
    if (!scenario) {
        setLastError(tr("Unknown scenario '%1'").arg(name));
        return false;
    }
    m_simulator.runScenario(std::move(*scenario));
    emit liveChanged();
    return true;
}

void PatientSimulatorController::stopScenario()
{
    m_simulator.setManualTargets(m_simulator.targets());
    emit liveChanged();
}

QStringList PatientSimulatorController::availableSerialPorts() const
{
    QStringList names;
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : ports) {
        names.append(info.portName());
    }
    names.sort();
    return names;
}

QList<int> PatientSimulatorController::standardBaudRates() const
{
    return {9600, 19200, 38400, 57600, 115200, 230400};
}

bool PatientSimulatorController::openTcp(int port)
{
    if (port <= 0 || port > 65535) {
        setLastError(tr("Invalid TCP port %1").arg(port));
        return false;
    }
    OutputSettings settings;
    settings.kind = OutputSettings::Kind::Tcp;
    settings.tcpPort = static_cast<quint16>(port);
    return openOutput(settings);
}

bool PatientSimulatorController::openSerial(const QString& portName, int baudRate)
{
    if (portName.isEmpty()) {
        setLastError(tr("No serial port selected"));
        return false;
    }
    OutputSettings settings;
    settings.kind = OutputSettings::Kind::Serial;
    settings.serialPortName = portName;
    settings.baudRate = baudRate;
    return openOutput(settings);
}

bool PatientSimulatorController::openOutput(const OutputSettings& settings)
{
    closeOutput();
    m_settings = settings;

    std::unique_ptr<IByteSink> sink = m_sinkFactory(settings);
    QString error;
    if (!sink || !sink->open(error)) {
        setLastError(tr("Cannot open %1: %2").arg(sink ? sink->description() : tr("output"), error));
        emit outputChanged();
        return false;
    }

    connect(sink.get(), &IByteSink::statusChanged, this, &PatientSimulatorController::outputChanged);
    connect(sink.get(), &IByteSink::errorOccurred, this, &PatientSimulatorController::setLastError);
    m_sink = std::move(sink);
    m_simulator.setSink(m_sink.get());
    setLastError({});
    emit outputChanged();
    return true;
}

void PatientSimulatorController::closeOutput()
{
    if (!m_sink) {
        return;
    }
    m_simulator.setSink(nullptr);
    m_sink->disconnect(this);
    m_sink->close();
    m_sink.reset();
    emit outputChanged();
}

// --- Power / mode ------------------------------------------------------------------

void PatientSimulatorController::setPowered(bool on)
{
    if (on) {
        m_simulator.start();
    } else {
        m_simulator.stop();
    }
}

QStringList PatientSimulatorController::scenarioNames() const
{
    QStringList names;
    for (const std::string_view name : simulator::scenarioNames()) {
        names.append(QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size())));
    }
    return names;
}

int PatientSimulatorController::scenarioSeconds() const
{
    return static_cast<int>(m_simulator.scenarioElapsedMs() / 1000);
}

// --- Patient settings --------------------------------------------------------------

void PatientSimulatorController::changeTargets(const std::function<void(PatientTargets&)>& change)
{
    // Touching a control on a real patient simulator takes over from the
    // running script; the new targets are held until a scenario is started.
    PatientTargets targets = m_simulator.targets();
    change(targets);
    m_simulator.setManualTargets(targets);
}

void PatientSimulatorController::setTargetHeartRate(int bpm)
{
    changeTargets([bpm](PatientTargets& t) { t.heartRateBpm = std::clamp(bpm, kMinimumHeartRate, kMaximumHeartRate); });
}

void PatientSimulatorController::setTargetSpo2(int percent)
{
    changeTargets([percent](PatientTargets& t) { t.spo2Percent = std::clamp(percent, kMinimumSpo2, kMaximumSpo2); });
}

void PatientSimulatorController::setLeadsOff(bool off)
{
    changeTargets([off](PatientTargets& t) { t.ecgLeadsOff = off; });
}

void PatientSimulatorController::setProbeOff(bool off)
{
    changeTargets([off](PatientTargets& t) { t.spo2ProbeOff = off; });
}

// --- Live values -------------------------------------------------------------------

int PatientSimulatorController::heartRate() const
{
    const auto value = m_simulator.numerics().heartRateBpm;
    return powered() && value ? static_cast<int>(*value) : -1;
}

int PatientSimulatorController::spo2() const
{
    const auto value = m_simulator.numerics().spo2Percent;
    return powered() && value ? static_cast<int>(*value) : -1;
}

void PatientSimulatorController::onNumericsGenerated()
{
    // Called once per second, so the byte difference is bytes per second.
    const quint64 bytes = m_simulator.statistics().bytesSent;
    m_bytesPerSecond = static_cast<int>(bytes - std::min(bytes, m_bytesAtLastSecond));
    m_bytesAtLastSecond = bytes;
    emit liveChanged();
}

// --- Output / faults ---------------------------------------------------------------

bool PatientSimulatorController::outputOpen() const
{
    return m_sink && m_sink->isOpen();
}

bool PatientSimulatorController::receiverConnected() const
{
    return m_sink && m_sink->hasReceiver();
}

QString PatientSimulatorController::outputDescription() const
{
    return m_sink ? m_sink->description() : tr("No output");
}

QString PatientSimulatorController::outputStatus() const
{
    return m_sink ? m_sink->status() : tr("Closed");
}

int PatientSimulatorController::lineLoadPercent() const
{
    if (!serialOutput() || m_settings.baudRate <= 0) {
        return -1;
    }
    const double capacity = static_cast<double>(m_settings.baudRate) / kBitsPerSerialByte;
    return static_cast<int>(std::lround(100.0 * m_bytesPerSecond / capacity));
}

void PatientSimulatorController::setCorruptionPercent(double percent)
{
    const double probability = std::clamp(percent, 0.0, 100.0) / 100.0;
    if (qFuzzyCompare(1.0 + probability, 1.0 + m_simulator.corruptionProbability())) {
        return;
    }
    m_simulator.setCorruptionProbability(probability);
    emit corruptionPercentChanged();
}

void PatientSimulatorController::setLastError(const QString& error)
{
    if (m_lastError == error) {
        return;
    }
    m_lastError = error;
    emit lastErrorChanged();
}

QString PatientSimulatorController::version() const
{
    return QStringLiteral(VSM_VERSION);
}

} // namespace vsm::simulator::gui
