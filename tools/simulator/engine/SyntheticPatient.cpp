#include "SyntheticPatient.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace vsm::simulator {

namespace {

/// One Gaussian component of the ECG beat.
struct Wave {
    double offset;      ///< seconds relative to the R peak (scaled for P and T)
    double amplitudeMv; ///< peak amplitude in millivolts
    double width;       ///< standard deviation in seconds
    bool scalesWithRr;  ///< P and T move with the heart rate, QRS does not
};

// Typical lead-II morphology at 60 bpm.
constexpr std::array kWaves{
    Wave{-0.16, 0.12, 0.025, true},   // P
    Wave{-0.035, -0.12, 0.010, false}, // Q
    Wave{0.0, 1.10, 0.011, false},     // R
    Wave{0.035, -0.25, 0.011, false},  // S
    Wave{0.28, 0.30, 0.045, true},     // T
};

constexpr double kRPeakFraction = 0.4;         // R peak position inside the RR interval
constexpr double kHeartRateSlewPerSecond = 3.0; // bpm/s
constexpr double kSpo2SlewPerSecond = 0.5;      // %/s
constexpr double kBaselineWanderMv = 0.05;
constexpr double kBaselineWanderHz = 0.3;

[[nodiscard]] double approach(double current, double target, double maxStep)
{
    return current + std::clamp(target - current, -maxStep, maxStep);
}

} // namespace

SyntheticPatient::SyntheticPatient(int sampleRateHz, std::uint32_t seed)
    : m_sampleRateHz(std::max(1, sampleRateHz))
    , m_dt(1.0 / static_cast<double>(m_sampleRateHz))
    , m_random(seed)
{
}

std::int16_t SyntheticPatient::nextEcgSampleMicrovolts()
{
    advanceTrends();

    m_time += m_dt;
    m_timeInBeat += m_dt;
    if (m_timeInBeat >= m_beatInterval) {
        // New beat: the RR interval is fixed for the whole beat so its waves
        // stay consistent even while the heart rate is changing.
        m_timeInBeat -= m_beatInterval;
        m_beatInterval = 60.0 / m_heartRate;
    }

    if (m_targets.ecgLeadsOff) {
        return 0; // flat line, like a disconnected electrode
    }
    const double microvolts = std::round(ecgMillivolts() * 1000.0);
    return static_cast<std::int16_t>(std::clamp(microvolts, -32768.0, 32767.0));
}

SimulatedNumerics SyntheticPatient::numerics() const
{
    SimulatedNumerics numerics;
    numerics.ecgLeadsOff = m_targets.ecgLeadsOff;
    numerics.spo2ProbeOff = m_targets.spo2ProbeOff;
    if (!m_targets.ecgLeadsOff) {
        numerics.heartRateBpm = static_cast<std::uint16_t>(std::lround(m_heartRate));
    }
    if (!m_targets.spo2ProbeOff) {
        numerics.spo2Percent = static_cast<std::uint8_t>(std::clamp(std::lround(m_spo2), 0L, 100L));
    }
    return numerics;
}

double SyntheticPatient::ecgMillivolts() const
{
    // Bazett-like scaling: P and T are closer to the R peak at higher rates.
    const double rrScale = std::sqrt(m_beatInterval);
    const double rPeakTime = kRPeakFraction * m_beatInterval;
    const double t = m_timeInBeat - rPeakTime;

    double value = 0.0;
    for (const Wave& wave : kWaves) {
        const double offset = wave.scalesWithRr ? wave.offset * rrScale : wave.offset;
        const double width = wave.scalesWithRr ? wave.width * rrScale : wave.width;
        const double d = t - offset;
        value += wave.amplitudeMv * std::exp(-(d * d) / (2.0 * width * width));
    }

    value += kBaselineWanderMv * std::sin(2.0 * std::numbers::pi * kBaselineWanderHz * m_time);
    value += m_noise(m_random);
    return value;
}

void SyntheticPatient::advanceTrends()
{
    m_heartRate = approach(m_heartRate, m_targets.heartRateBpm, kHeartRateSlewPerSecond * m_dt);
    m_spo2 = approach(m_spo2, m_targets.spo2Percent, kSpo2SlewPerSecond * m_dt);
}

} // namespace vsm::simulator
