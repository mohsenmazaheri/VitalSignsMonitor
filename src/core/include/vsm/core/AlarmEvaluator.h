#pragma once

#include "vsm/core/IAlarmRule.h"

#include <memory>
#include <vector>

namespace vsm::core {

/// Runs a set of alarm rules and produces the list of active alarms.
///
/// Responsibilities (and nothing more):
///  - evaluate every rule against the current context,
///  - keep only the highest-priority alarm per AlarmSource,
///  - sort the result: highest priority first, then by source.
///
/// The evaluator is stateless between calls; latching, silencing and audio
/// belong to a separate alarm manager (planned for week 3).
class AlarmEvaluator {
public:
    explicit AlarmEvaluator(std::vector<std::unique_ptr<IAlarmRule>> rules);

    AlarmEvaluator(AlarmEvaluator&&) noexcept = default;
    AlarmEvaluator& operator=(AlarmEvaluator&&) noexcept = default;
    AlarmEvaluator(const AlarmEvaluator&) = delete;
    AlarmEvaluator& operator=(const AlarmEvaluator&) = delete;
    ~AlarmEvaluator() = default;

    [[nodiscard]] std::vector<Alarm> evaluate(const AlarmContext& context) const;

    [[nodiscard]] std::size_t ruleCount() const noexcept { return m_rules.size(); }

private:
    std::vector<std::unique_ptr<IAlarmRule>> m_rules;
};

} // namespace vsm::core
