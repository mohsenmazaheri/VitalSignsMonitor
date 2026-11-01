#include "vsm/core/AlarmEvaluator.h"

#include <algorithm>
#include <stdexcept>

namespace vsm::core {

AlarmEvaluator::AlarmEvaluator(std::vector<std::unique_ptr<IAlarmRule>> rules)
    : m_rules(std::move(rules))
{
    if (std::ranges::any_of(m_rules, [](const auto& rule) { return rule == nullptr; })) {
        throw std::invalid_argument("AlarmEvaluator: rules must not contain null pointers");
    }
}

std::vector<Alarm> AlarmEvaluator::evaluate(const AlarmContext& context) const
{
    std::vector<Alarm> active;

    for (const auto& rule : m_rules) {
        std::optional<Alarm> alarm = rule->evaluate(context);
        if (!alarm) {
            continue;
        }

        // Keep only the most severe alarm per source.
        const auto sameSource = std::ranges::find(active, alarm->source, &Alarm::source);
        if (sameSource == active.end()) {
            active.push_back(std::move(*alarm));
        } else if (alarm->priority > sameSource->priority) {
            *sameSource = std::move(*alarm);
        }
    }

    std::ranges::sort(active, [](const Alarm& a, const Alarm& b) {
        if (a.priority != b.priority) {
            return a.priority > b.priority;
        }
        return a.source < b.source;
    });
    return active;
}

} // namespace vsm::core
