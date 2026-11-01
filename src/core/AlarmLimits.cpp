#include "vsm/core/AlarmLimits.h"

namespace vsm::core {

bool AlarmLimits::isValid() const noexcept
{
    const bool heartRateOrdered = 0 < heartRateCriticalLow && heartRateCriticalLow < heartRateLow
                                  && heartRateLow < heartRateHigh && heartRateHigh < heartRateCriticalHigh
                                  && heartRateCriticalHigh <= 300;

    const bool spo2Ordered = 0 < spo2CriticalLow && spo2CriticalLow < spo2Low && spo2Low <= 100;

    return heartRateOrdered && spo2Ordered;
}

} // namespace vsm::core
