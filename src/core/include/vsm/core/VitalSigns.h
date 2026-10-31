#pragma once

#include <optional>

namespace vsm::core {

/// Latest numeric vital signs of the (synthetic) patient.
///
/// This is the domain representation, independent of the wire format in
/// vsm::protocol. An empty optional means the value is currently not
/// available (e.g. sensor disconnected or still measuring).
struct VitalSigns {
    std::optional<int> heartRateBpm;
    std::optional<int> spo2Percent;
    bool ecgLeadsOff{false};
    bool spo2ProbeOff{false};

    friend bool operator==(const VitalSigns&, const VitalSigns&) = default;
};

/// Everything the alarm rules need to make a decision.
struct AlarmContext {
    VitalSigns vitals;
    bool deviceLinkLost{false}; ///< No valid data from the device within the watchdog time.
};

} // namespace vsm::core
