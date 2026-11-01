#include "DeviceSimulator.h"

#include "vsm/protocol/Frame.h"

#include <QElapsedTimer>
#include <QTimer>

#include <algorithm>
#include <memory>

namespace vsm::simulator {

namespace {

DeviceSimulator::Clock makeSteadyClock()
{
    auto timer = std::make_shared<QElapsedTimer>();
    timer->start();
    return [timer] { return timer->elapsed(); };
}

QString toQString(const std::string& text)
{
    return QString::fromStdString(text);
}

} // namespace

DeviceSimulator::DeviceSimulator(int sampleRateHz, QObject* parent)
    : DeviceSimulator(sampleRateHz, makeSteadyClock(), parent)
{
}

DeviceSimulator::DeviceSimulator(int sampleRateHz, Clock clock, QObject* parent)
    : QObject(parent)
    , m_clock(std::move(clock))
    , m_timer(new QTimer(this))
    , m_patient(sampleRateHz)
{
    m_timer->setTimerType(Qt::PreciseTimer);
    m_timer->setInterval(kTickIntervalMs);
    connect(m_timer, &QTimer::timeout, this, &DeviceSimulator::tick);
}

// --- Output --------------------------------------------------------------------

void DeviceSimulator::setSink(IByteSink* sink)
{
    m_sink = sink;
}

// --- Patient -------------------------------------------------------------------

void DeviceSimulator::runScenario(Scenario scenario)
{
    m_scenario = std::move(scenario);
    m_scenarioStartMs = m_clock();
    m_currentPhase = nullptr;
    emit modeChanged();
    updateScenarioPhase(m_scenarioStartMs);
}

void DeviceSimulator::setManualTargets(const PatientTargets& targets)
{
    const bool wasScenario = m_scenario.has_value();
    m_scenario.reset();
    m_currentPhase = nullptr;
    if (wasScenario) {
        emit modeChanged();
        emit phaseChanged(phaseLabel());
    }
    applyTargets(targets);
}

QString DeviceSimulator::scenarioName() const
{
    return m_scenario ? toQString(m_scenario->name()) : QString();
}

QString DeviceSimulator::phaseLabel() const
{
    if (!m_scenario) {
        return QStringLiteral("Manual");
    }
    return m_currentPhase != nullptr ? toQString(m_currentPhase->label) : QString();
}

qint64 DeviceSimulator::scenarioElapsedMs() const
{
    return m_scenario ? m_clock() - m_scenarioStartMs : 0;
}

void DeviceSimulator::applyTargets(const PatientTargets& targets)
{
    if (m_patient.targets() == targets) {
        return;
    }
    m_patient.setTargets(targets);
    emit targetsChanged();
}

void DeviceSimulator::updateScenarioPhase(qint64 nowMs)
{
    if (!m_scenario) {
        return;
    }
    const Scenario::Phase& phase = m_scenario->phaseAt(std::chrono::milliseconds(nowMs - m_scenarioStartMs));
    if (&phase == m_currentPhase) {
        return;
    }
    m_currentPhase = &phase;
    applyTargets(phase.targets);
    emit phaseChanged(phaseLabel());
}

// --- Faults --------------------------------------------------------------------

void DeviceSimulator::setCorruptionProbability(double probability)
{
    m_corruptionProbability = std::clamp(probability, 0.0, 1.0);
}

void DeviceSimulator::setTransmissionEnabled(bool enabled)
{
    if (m_transmissionEnabled == enabled) {
        return;
    }
    m_transmissionEnabled = enabled;
    emit transmissionEnabledChanged(enabled);
}

// --- Run control ---------------------------------------------------------------

void DeviceSimulator::start()
{
    if (isRunning()) {
        return;
    }
    // A powered-off device generates nothing; restarting begins a fresh
    // sample clock so that no "catch-up" burst of samples is sent.
    m_startMs = m_clock();
    m_samplesGenerated = 0;
    m_numericsDue = true;
    m_timer->start();
    emit runningChanged(true);
    tick();
}

void DeviceSimulator::stop()
{
    if (!isRunning()) {
        return;
    }
    m_timer->stop();
    emit runningChanged(false);
}

bool DeviceSimulator::isRunning() const
{
    return m_timer->isActive();
}

void DeviceSimulator::tick()
{
    const qint64 nowMs = m_clock();

    // 1. Scenario -> patient targets
    updateScenarioPhase(nowMs);

    // 2. ECG: exactly as many samples as real time requires
    generateEcg(nowMs - m_startMs);

    // 3. Numerics + heartbeat once per second
    if (m_numericsDue || nowMs - m_lastNumericsMs >= kNumericsIntervalMs) {
        m_numericsDue = false;
        m_lastNumericsMs = nowMs;
        const SimulatedNumerics numerics = m_patient.numerics();
        send(protocol::NumericsMessage{numerics.heartRateBpm, numerics.spo2Percent, numerics.ecgLeadsOff,
                                       numerics.spo2ProbeOff});
        send(protocol::HeartbeatMessage{static_cast<std::uint32_t>(nowMs - m_startMs)});
        emit numericsGenerated();
    }
    emit statisticsChanged();
}

void DeviceSimulator::generateEcg(qint64 elapsedMs)
{
    // Several frames per tick are possible after a long stall (e.g. a dragged
    // window on Windows blocks the event loop); each frame holds at most
    // kMaxEcgSamplesPerFrame samples.
    qint64 samplesDue = elapsedMs * m_patient.sampleRateHz() / 1000 - m_samplesGenerated;
    QList<float> preview;
    while (samplesDue > 0) {
        protocol::EcgWaveformMessage ecg;
        ecg.sampleRateHz = static_cast<std::uint16_t>(m_patient.sampleRateHz());
        const auto count = std::min<qint64>(samplesDue, static_cast<qint64>(protocol::kMaxEcgSamplesPerFrame));
        ecg.samplesMicrovolts.reserve(static_cast<std::size_t>(count));
        for (qint64 i = 0; i < count; ++i) {
            const std::int16_t microvolts = m_patient.nextEcgSampleMicrovolts();
            ecg.samplesMicrovolts.push_back(microvolts);
            preview.append(static_cast<float>(microvolts) / 1000.0F);
        }
        m_samplesGenerated += count;
        samplesDue -= count;
        send(ecg);
    }
    if (!preview.isEmpty()) {
        emit ecgSamplesGenerated(preview, m_patient.sampleRateHz());
    }
}

void DeviceSimulator::send(const protocol::Message& message)
{
    // The sequence number advances even when nothing is sent: a real device
    // does not know that its cable is unplugged, and the monitor must detect
    // the gap after reconnecting.
    const std::uint16_t sequence = m_sequence++;

    if (!m_transmissionEnabled || m_sink == nullptr || !m_sink->isOpen()) {
        ++m_statistics.framesDropped;
        return;
    }

    const std::vector<std::uint8_t> bytes = protocol::encodeFrame(protocol::toFrame(message, sequence));
    QByteArray data(reinterpret_cast<const char*>(bytes.data()), static_cast<qsizetype>(bytes.size()));

    if (m_corruptionProbability > 0.0 && std::bernoulli_distribution(m_corruptionProbability)(m_random)) {
        const qsizetype index = std::uniform_int_distribution<qsizetype>(0, data.size() - 1)(m_random);
        data[index] = static_cast<char>(data[index] ^ 0x5A);
        ++m_statistics.framesCorrupted;
    }
    m_sink->write(data);
    ++m_statistics.framesSent;
    m_statistics.bytesSent += static_cast<quint64>(data.size());
}

} // namespace vsm::simulator
