#include "Scenario.h"

#include <stdexcept>

namespace vsm::simulator {

using namespace std::chrono_literals;

Scenario::Scenario(std::string name, std::vector<Phase> phases)
    : m_name(std::move(name))
    , m_phases(std::move(phases))
{
    if (m_phases.empty()) {
        throw std::invalid_argument("Scenario: at least one phase is required");
    }
    for (const Phase& phase : m_phases) {
        if (phase.duration <= 0s) {
            throw std::invalid_argument("Scenario: phase duration must be > 0");
        }
        m_totalDuration += phase.duration;
    }
}

const Scenario::Phase& Scenario::phaseAt(std::chrono::milliseconds elapsed) const
{
    auto offset = elapsed % m_totalDuration;
    for (const Phase& phase : m_phases) {
        if (offset < phase.duration) {
            return phase;
        }
        offset -= phase.duration;
    }
    return m_phases.back(); // unreachable, keeps the compiler happy
}

std::vector<std::string_view> scenarioNames()
{
    return {"normal", "tachycardia", "bradycardia", "desaturation", "sensor-off", "demo"};
}

std::optional<Scenario> makeScenario(std::string_view name)
{
    //                       HR  SpO2  leadsOff probeOff
    const PatientTargets normal{75, 98, false, false};
    const PatientTargets tachy{145, 96, false, false};
    const PatientTargets severeTachy{165, 94, false, false};
    const PatientTargets brady{44, 97, false, false};
    const PatientTargets desat{88, 87, false, false};
    const PatientTargets severeDesat{95, 82, false, false};
    const PatientTargets leadsOff{75, 98, true, false};
    const PatientTargets probeOff{75, 98, false, true};

    if (name == "normal") {
        return Scenario("normal", {{60s, normal, "Normal sinus rhythm"}});
    }
    if (name == "tachycardia") {
        return Scenario("tachycardia", {{20s, normal, "Normal"}, {40s, tachy, "Tachycardia"},
                                        {30s, severeTachy, "Severe tachycardia"}});
    }
    if (name == "bradycardia") {
        return Scenario("bradycardia", {{20s, normal, "Normal"}, {40s, brady, "Bradycardia"}});
    }
    if (name == "desaturation") {
        return Scenario("desaturation", {{20s, normal, "Normal"}, {30s, desat, "Desaturation"},
                                         {30s, severeDesat, "Severe desaturation"}});
    }
    if (name == "sensor-off") {
        return Scenario("sensor-off", {{15s, normal, "Normal"}, {10s, leadsOff, "ECG leads off"},
                                       {15s, normal, "Normal"}, {10s, probeOff, "SpO2 probe off"}});
    }
    if (name == "demo") {
        return Scenario("demo", {{20s, normal, "Normal"},
                                 {25s, tachy, "Tachycardia"},
                                 {15s, normal, "Recovery"},
                                 {25s, desat, "Desaturation"},
                                 {20s, severeDesat, "Severe desaturation"},
                                 {15s, normal, "Recovery"},
                                 {25s, brady, "Bradycardia"},
                                 {10s, leadsOff, "ECG leads off"},
                                 {10s, probeOff, "SpO2 probe off"}});
    }
    return std::nullopt;
}

} // namespace vsm::simulator
