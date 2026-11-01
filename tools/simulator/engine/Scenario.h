#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vsm::simulator {

/// What the synthetic patient should look like at a point in time.
struct PatientTargets {
    int heartRateBpm{75};
    int spo2Percent{98};
    bool ecgLeadsOff{false};
    bool spo2ProbeOff{false};

    friend bool operator==(const PatientTargets&, const PatientTargets&) = default;
};

/// A timed sequence of patient states that loops forever.
///
/// Scenarios are data, not code: a new clinical picture is a new list of
/// phases in makeScenario(), no new class is needed (Open/Closed Principle).
class Scenario {
public:
    struct Phase {
        std::chrono::seconds duration;
        PatientTargets targets;
        std::string label;
    };

    Scenario(std::string name, std::vector<Phase> phases);

    [[nodiscard]] const std::string& name() const noexcept { return m_name; }

    /// Phase that is active @p elapsed after the start (the scenario loops).
    [[nodiscard]] const Phase& phaseAt(std::chrono::milliseconds elapsed) const;

private:
    std::string m_name;
    std::vector<Phase> m_phases;
    std::chrono::milliseconds m_totalDuration{0};
};

/// Names accepted by makeScenario(), for --help and --list-scenarios.
[[nodiscard]] std::vector<std::string_view> scenarioNames();

/// @return the scenario with @p name, or std::nullopt if it does not exist.
[[nodiscard]] std::optional<Scenario> makeScenario(std::string_view name);

} // namespace vsm::simulator
