#include "vsm/core/Alarm.h"

namespace vsm::core {

std::string_view toString(AlarmPriority priority) noexcept
{
    switch (priority) {
    case AlarmPriority::Low:
        return "Low";
    case AlarmPriority::Medium:
        return "Medium";
    case AlarmPriority::High:
        return "High";
    }
    return "Unknown";
}

std::string_view toString(AlarmCategory category) noexcept
{
    switch (category) {
    case AlarmCategory::Physiological:
        return "Physiological";
    case AlarmCategory::Technical:
        return "Technical";
    }
    return "Unknown";
}

} // namespace vsm::core
