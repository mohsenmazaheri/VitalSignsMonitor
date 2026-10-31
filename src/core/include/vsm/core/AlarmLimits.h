#pragma once

namespace vsm::core {

/// Configurable alarm thresholds (adult defaults for a demo, not clinical advice).
///
/// A value strictly beyond a limit raises the alarm, e.g. heart rate 121 bpm
/// raises "HR high" with the default heartRateHigh = 120.
struct AlarmLimits {
    int heartRateCriticalLow{40};  ///< below -> High priority
    int heartRateLow{50};          ///< below -> Medium priority
    int heartRateHigh{120};        ///< above -> Medium priority
    int heartRateCriticalHigh{150}; ///< above -> High priority

    int spo2CriticalLow{85}; ///< below -> High priority
    int spo2Low{90};         ///< below -> Medium priority

    /// True if the limits are ordered consistently and inside physical ranges.
    [[nodiscard]] bool isValid() const noexcept;

    friend bool operator==(const AlarmLimits&, const AlarmLimits&) = default;
};

} // namespace vsm::core
