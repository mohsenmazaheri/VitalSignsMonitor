#pragma once

#include "vsm/core/AlarmLimits.h"
#include "vsm/core/IAlarmRule.h"

#include <functional>
#include <memory>
#include <vector>

namespace vsm::core {

/// Direction in which a limit is violated.
enum class LimitDirection {
    Above, ///< value > limit raises the alarm
    Below, ///< value < limit raises the alarm
};

/// Raises an alarm when a numeric value crosses a fixed limit.
/// A missing value (std::nullopt) never raises a limit alarm; sensor problems
/// are reported by separate technical rules.
class LimitAlarmRule final : public IAlarmRule {
public:
    using ValueExtractor = std::function<std::optional<int>(const VitalSigns&)>;

    LimitAlarmRule(ValueExtractor extractor, LimitDirection direction, int limit, Alarm alarm);

    [[nodiscard]] std::optional<Alarm> evaluate(const AlarmContext& context) const override;

private:
    ValueExtractor m_extractor;
    LimitDirection m_direction;
    int m_limit;
    Alarm m_alarm;
};

/// Raises an alarm while a boolean condition is true (e.g. "ECG leads off").
class ConditionAlarmRule final : public IAlarmRule {
public:
    using Predicate = std::function<bool(const AlarmContext&)>;

    ConditionAlarmRule(Predicate predicate, Alarm alarm);

    [[nodiscard]] std::optional<Alarm> evaluate(const AlarmContext& context) const override;

private:
    Predicate m_predicate;
    Alarm m_alarm;
};

/// Builds the standard rule set (HR, SpO2, leads/probe off, device link lost).
/// @throws std::invalid_argument if @p limits is not valid.
[[nodiscard]] std::vector<std::unique_ptr<IAlarmRule>> makeDefaultAlarmRules(const AlarmLimits& limits);

} // namespace vsm::core
