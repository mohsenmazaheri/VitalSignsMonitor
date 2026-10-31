#include "vsm/core/AlarmLimits.h"

#include <gtest/gtest.h>

using vsm::core::AlarmLimits;

TEST(AlarmLimits, DefaultsAreValid)
{
    EXPECT_TRUE(AlarmLimits{}.isValid());
}

TEST(AlarmLimits, HeartRateLimitsMustBeOrdered)
{
    AlarmLimits limits;
    limits.heartRateCriticalHigh = limits.heartRateHigh; // must be strictly greater
    EXPECT_FALSE(limits.isValid());
}

TEST(AlarmLimits, Spo2LimitsMustBeOrderedAndAtMostHundred)
{
    AlarmLimits limits;
    limits.spo2CriticalLow = 95; // above spo2Low
    EXPECT_FALSE(limits.isValid());

    limits = AlarmLimits{};
    limits.spo2Low = 101;
    EXPECT_FALSE(limits.isValid());
}
