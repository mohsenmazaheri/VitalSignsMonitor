#pragma once

#include "IByteSink.h"
#include "Scenario.h"
#include "SyntheticPatient.h"

#include "vsm/protocol/Messages.h"

#include <QList>
#include <QObject>
#include <QPointer>

#include <functional>
#include <optional>
#include <random>

class QTimer;

namespace vsm::simulator {

/// Counters of what the simulated device has produced and sent.
struct TransmitStatistics {
    quint64 framesSent{0};      ///< frames written to the sink
    quint64 bytesSent{0};
    quint64 framesCorrupted{0}; ///< sent with one deliberately damaged byte
    quint64 framesDropped{0};   ///< produced while the "cable" was unplugged or no sink was set

    friend bool operator==(const TransmitStatistics&, const TransmitStatistics&) = default;
};

/// The simulated bedside device: generates the synthetic patient in real time
/// and sends protocol frames to an IByteSink.
///
/// The patient is driven either by a Scenario (scripted phases that loop) or
/// manually (fixed targets set from the UI or the command line). Two fault
/// switches help to test the monitor: frame corruption (CRC errors) and an
/// "unplugged cable" (frames are produced and numbered but not sent, so the
/// monitor sees a link loss and then a sequence gap).
///
/// Timing: a 40 ms timer calls tick(). The number of ECG samples is derived
/// from a monotonic clock, not from counting ticks, so the sample rate stays
/// exact even when the timer jitters. The clock is injectable for tests.
class DeviceSimulator final : public QObject {
    Q_OBJECT

public:
    static constexpr int kTickIntervalMs = 40;
    static constexpr int kNumericsIntervalMs = 1000;

    /// Monotonic time in milliseconds.
    using Clock = std::function<qint64()>;

    explicit DeviceSimulator(int sampleRateHz, QObject* parent = nullptr);
    DeviceSimulator(int sampleRateHz, Clock clock, QObject* parent = nullptr);

    // --- Output ----------------------------------------------------------------
    /// Sets the destination (not owned). nullptr = generate without sending.
    void setSink(IByteSink* sink);
    [[nodiscard]] IByteSink* sink() const { return m_sink; }

    // --- Patient -----------------------------------------------------------------
    /// Starts @p scenario from its first phase.
    void runScenario(Scenario scenario);
    /// Stops the scenario (if any) and holds @p targets.
    void setManualTargets(const PatientTargets& targets);

    [[nodiscard]] bool isScenarioRunning() const noexcept { return m_scenario.has_value(); }
    /// Name of the running scenario, empty in manual mode.
    [[nodiscard]] QString scenarioName() const;
    /// Label of the current scenario phase, "Manual" in manual mode.
    [[nodiscard]] QString phaseLabel() const;
    /// Time since the scenario was started (0 in manual mode).
    [[nodiscard]] qint64 scenarioElapsedMs() const;

    /// What the patient is moving towards.
    [[nodiscard]] const PatientTargets& targets() const noexcept { return m_patient.targets(); }
    /// What the device reports right now (HR/SpO2 change gradually towards the targets).
    [[nodiscard]] SimulatedNumerics numerics() const { return m_patient.numerics(); }
    [[nodiscard]] int sampleRateHz() const noexcept { return m_patient.sampleRateHz(); }

    // --- Faults ------------------------------------------------------------------
    /// Probability (0..1) that a sent frame gets one corrupted byte.
    void setCorruptionProbability(double probability);
    [[nodiscard]] double corruptionProbability() const noexcept { return m_corruptionProbability; }

    /// false = "cable unplugged": frames are still produced and numbered, but not sent.
    void setTransmissionEnabled(bool enabled);
    [[nodiscard]] bool isTransmissionEnabled() const noexcept { return m_transmissionEnabled; }

    [[nodiscard]] const TransmitStatistics& statistics() const noexcept { return m_statistics; }

    // --- Run control ---------------------------------------------------------------
    void start();
    void stop();
    [[nodiscard]] bool isRunning() const;

    /// One simulation step. Called by the internal timer; public so that tests
    /// can drive the simulator with a fake clock and no event loop.
    void tick();

signals:
    void runningChanged(bool running);
    /// Scenario started/stopped or manual targets changed.
    void modeChanged();
    void targetsChanged();
    void phaseChanged(const QString& label);
    /// The samples that were just generated (for a local preview), in millivolts.
    void ecgSamplesGenerated(const QList<float>& millivolts, int sampleRateHz);
    /// Once per second, after the numerics frame was produced.
    void numericsGenerated();
    void transmissionEnabledChanged(bool enabled);
    void statisticsChanged();

private:
    void applyTargets(const PatientTargets& targets);
    void updateScenarioPhase(qint64 nowMs);
    void generateEcg(qint64 elapsedMs);
    void send(const protocol::Message& message);

    Clock m_clock;
    QTimer* m_timer;
    QPointer<IByteSink> m_sink;
    SyntheticPatient m_patient;

    std::optional<Scenario> m_scenario;
    qint64 m_scenarioStartMs{0};
    const Scenario::Phase* m_currentPhase{nullptr};

    qint64 m_startMs{0};
    qint64 m_samplesGenerated{0};
    qint64 m_lastNumericsMs{0};
    bool m_numericsDue{true};
    std::uint16_t m_sequence{0};

    bool m_transmissionEnabled{true};
    double m_corruptionProbability{0.0};
    std::mt19937 m_random{1234};
    TransmitStatistics m_statistics;
};

} // namespace vsm::simulator
