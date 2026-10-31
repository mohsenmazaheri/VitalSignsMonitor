#pragma once

#include "vsm/core/Alarm.h"
#include "vsm/core/VitalSigns.h"

#include <optional>

namespace vsm::core {

/// A single, independent alarm condition (Strategy pattern).
///
/// New alarm types are added by implementing this interface - the
/// AlarmEvaluator never has to change (Open/Closed Principle).
class IAlarmRule {
public:
    virtual ~IAlarmRule() = default;

    /// @return the alarm if the condition is active for @p context, otherwise std::nullopt.
    [[nodiscard]] virtual std::optional<Alarm> evaluate(const AlarmContext& context) const = 0;

protected:
    IAlarmRule() = default;
    IAlarmRule(const IAlarmRule&) = default;
    IAlarmRule& operator=(const IAlarmRule&) = default;
    IAlarmRule(IAlarmRule&&) = default;
    IAlarmRule& operator=(IAlarmRule&&) = default;
};

} // namespace vsm::core
