#include "vsm/core/AlarmRules.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace vsm::core {

// --- LimitAlarmRule -----------------------------------------------------------

LimitAlarmRule::LimitAlarmRule(ValueExtractor extractor, LimitDirection direction, int limit, Alarm alarm)
    : m_extractor(std::move(extractor))
    , m_direction(direction)
    , m_limit(limit)
    , m_alarm(std::move(alarm))
{
    if (!m_extractor) {
        throw std::invalid_argument("LimitAlarmRule: extractor must not be empty");
    }
}

std::optional<Alarm> LimitAlarmRule::evaluate(const AlarmContext& context) const
{
    const std::optional<int> value = m_extractor(context.vitals);
    if (!value) {
        return std::nullopt;
    }
    const bool violated = (m_direction == LimitDirection::Above) ? (*value > m_limit) : (*value < m_limit);
    return violated ? std::optional<Alarm>(m_alarm) : std::nullopt;
}

// --- ConditionAlarmRule -------------------------------------------------------

ConditionAlarmRule::ConditionAlarmRule(Predicate predicate, Alarm alarm)
    : m_predicate(std::move(predicate))
    , m_alarm(std::move(alarm))
{
    if (!m_predicate) {
        throw std::invalid_argument("ConditionAlarmRule: predicate must not be empty");
    }
}

std::optional<Alarm> ConditionAlarmRule::evaluate(const AlarmContext& context) const
{
    return m_predicate(context) ? std::optional<Alarm>(m_alarm) : std::nullopt;
}

// --- Default rule set ---------------------------------------------------------

namespace {

std::optional<int> heartRate(const VitalSigns& vitals)
{
    return vitals.heartRateBpm;
}

std::optional<int> spo2(const VitalSigns& vitals)
{
    return vitals.spo2Percent;
}

Alarm makeAlarm(std::string code, AlarmSource source, AlarmCategory category, AlarmPriority priority,
                std::string message)
{
    return Alarm{std::move(code), source, category, priority, std::move(message)};
}

} // namespace

std::vector<std::unique_ptr<IAlarmRule>> makeDefaultAlarmRules(const AlarmLimits& limits)
{
    if (!limits.isValid()) {
        throw std::invalid_argument("makeDefaultAlarmRules: inconsistent alarm limits");
    }

    using enum AlarmSource;
    using enum AlarmPriority;
    constexpr auto physiological = AlarmCategory::Physiological;
    constexpr auto technical = AlarmCategory::Technical;
    const auto str = [](int value) { return std::to_string(value); };

    std::vector<std::unique_ptr<IAlarmRule>> rules;

    // Heart rate
    rules.push_back(std::make_unique<LimitAlarmRule>(
        heartRate, LimitDirection::Above, limits.heartRateCriticalHigh,
        makeAlarm("HR_CRITICAL_HIGH", HeartRate, physiological, High,
                  "HR extremely high (> " + str(limits.heartRateCriticalHigh) + " bpm)")));
    rules.push_back(std::make_unique<LimitAlarmRule>(
        heartRate, LimitDirection::Above, limits.heartRateHigh,
        makeAlarm("HR_HIGH", HeartRate, physiological, Medium,
                  "HR high (> " + str(limits.heartRateHigh) + " bpm)")));
    rules.push_back(std::make_unique<LimitAlarmRule>(
        heartRate, LimitDirection::Below, limits.heartRateLow,
        makeAlarm("HR_LOW", HeartRate, physiological, Medium,
                  "HR low (< " + str(limits.heartRateLow) + " bpm)")));
    rules.push_back(std::make_unique<LimitAlarmRule>(
        heartRate, LimitDirection::Below, limits.heartRateCriticalLow,
        makeAlarm("HR_CRITICAL_LOW", HeartRate, physiological, High,
                  "HR extremely low (< " + str(limits.heartRateCriticalLow) + " bpm)")));

    // SpO2
    rules.push_back(std::make_unique<LimitAlarmRule>(
        spo2, LimitDirection::Below, limits.spo2Low,
        makeAlarm("SPO2_LOW", Spo2, physiological, Medium,
                  "SpO2 low (< " + str(limits.spo2Low) + " %)")));
    rules.push_back(std::make_unique<LimitAlarmRule>(
        spo2, LimitDirection::Below, limits.spo2CriticalLow,
        makeAlarm("SPO2_CRITICAL_LOW", Spo2, physiological, High,
                  "SpO2 critically low (< " + str(limits.spo2CriticalLow) + " %)")));

    // Technical conditions
    rules.push_back(std::make_unique<ConditionAlarmRule>(
        [](const AlarmContext& c) { return !c.deviceLinkLost && c.vitals.ecgLeadsOff; },
        makeAlarm("ECG_LEADS_OFF", EcgLeads, technical, Low, "ECG leads off")));
    rules.push_back(std::make_unique<ConditionAlarmRule>(
        [](const AlarmContext& c) { return !c.deviceLinkLost && c.vitals.spo2ProbeOff; },
        makeAlarm("SPO2_PROBE_OFF", Spo2Probe, technical, Low, "SpO2 probe off")));
    rules.push_back(std::make_unique<ConditionAlarmRule>(
        [](const AlarmContext& c) { return c.deviceLinkLost; },
        makeAlarm("DEVICE_LINK_LOST", DeviceLink, technical, Medium, "No data from device")));

    return rules;
}

} // namespace vsm::core
