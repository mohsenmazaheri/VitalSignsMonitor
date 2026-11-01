#pragma once

#include "Scenario.h"

#include <cstdint>
#include <optional>
#include <random>

namespace vsm::simulator {

/// Numerics as the simulated device would report them.
struct SimulatedNumerics {
    std::optional<std::uint16_t> heartRateBpm;
    std::optional<std::uint8_t> spo2Percent;
    bool ecgLeadsOff{false};
    bool spo2ProbeOff{false};
};

/// Generates a SYNTHETIC ECG signal and matching numerics.
///
/// The ECG is a sum of Gaussian pulses for the P, Q, R, S and T waves (a
/// simplified version of the McSharry et al. dynamical model), plus baseline
/// wander and noise. Heart rate and SpO2 move gradually towards the targets
/// so that alarms are crossed realistically instead of jumping.
///
/// Deterministic for a given seed - useful for repeatable demos and tests.
class SyntheticPatient {
public:
    explicit SyntheticPatient(int sampleRateHz, std::uint32_t seed = 42);

    void setTargets(const PatientTargets& targets) noexcept { m_targets = targets; }
    [[nodiscard]] const PatientTargets& targets() const noexcept { return m_targets; }

    /// Advances time by one sample period and returns the ECG value in microvolts.
    [[nodiscard]] std::int16_t nextEcgSampleMicrovolts();

    /// Current numerics (HR and SpO2 rounded, invalid while a sensor is off).
    [[nodiscard]] SimulatedNumerics numerics() const;

    [[nodiscard]] int sampleRateHz() const noexcept { return m_sampleRateHz; }

private:
    [[nodiscard]] double ecgMillivolts() const;
    void advanceTrends();

    int m_sampleRateHz;
    double m_dt; // seconds per sample
    PatientTargets m_targets;

    double m_heartRate{75.0};
    double m_spo2{98.0};

    double m_time{0.0};        // seconds since start (baseline wander)
    double m_timeInBeat{0.0};  // seconds since the current beat started
    double m_beatInterval{0.8}; // RR interval of the current beat, seconds

    mutable std::mt19937 m_random;
    mutable std::normal_distribution<double> m_noise{0.0, 0.01}; // mV
};

} // namespace vsm::simulator
