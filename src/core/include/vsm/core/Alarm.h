#pragma once

#include <string>
#include <string_view>

namespace vsm::core {

/// Alarm priority. The three-level scheme follows the idea of IEC 60601-1-8
/// (high / medium / low); it is a learning model, not a certified implementation.
enum class AlarmPriority {
    Low = 1,
    Medium = 2,
    High = 3,
};

/// Physiological alarms describe the patient, technical alarms the equipment.
enum class AlarmCategory {
    Physiological,
    Technical,
};

/// What the alarm is about. The evaluator keeps at most one alarm per source
/// (the one with the highest priority), so "SpO2 low" and "SpO2 critically
/// low" are never shown at the same time.
enum class AlarmSource {
    HeartRate,
    Spo2,
    EcgLeads,
    Spo2Probe,
    DeviceLink,
};

/// One active alarm condition.
struct Alarm {
    std::string code;    ///< Stable identifier, e.g. "HR_HIGH" (for logs and tests).
    AlarmSource source{AlarmSource::HeartRate};
    AlarmCategory category{AlarmCategory::Physiological};
    AlarmPriority priority{AlarmPriority::Low};
    std::string message; ///< Human-readable text shown to the user.

    friend bool operator==(const Alarm&, const Alarm&) = default;
};

[[nodiscard]] std::string_view toString(AlarmPriority priority) noexcept;
[[nodiscard]] std::string_view toString(AlarmCategory category) noexcept;

} // namespace vsm::core
