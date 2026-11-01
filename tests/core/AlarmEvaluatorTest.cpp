#include "vsm/core/AlarmEvaluator.h"
#include "vsm/core/AlarmRules.h"

#include <gtest/gtest.h>

#include <algorithm>

using namespace vsm::core;

namespace {

/// Fixture with the default rule set and default limits.
class DefaultAlarmsTest : public ::testing::Test {
protected:
    AlarmEvaluator evaluator{makeDefaultAlarmRules(AlarmLimits{})};

    std::vector<Alarm> evaluate(std::optional<int> hr, std::optional<int> spo2, bool leadsOff = false,
                                bool probeOff = false, bool linkLost = false) const
    {
        return evaluator.evaluate(AlarmContext{VitalSigns{hr, spo2, leadsOff, probeOff}, linkLost});
    }

    static std::vector<std::string> codes(const std::vector<Alarm>& alarms)
    {
        std::vector<std::string> result;
        std::ranges::transform(alarms, std::back_inserter(result), &Alarm::code);
        return result;
    }
};

using Codes = std::vector<std::string>;

} // namespace

TEST_F(DefaultAlarmsTest, NormalVitalsRaiseNoAlarm)
{
    EXPECT_TRUE(evaluate(75, 98).empty());
}

TEST_F(DefaultAlarmsTest, LimitsAreExclusive)
{
    // Exactly on the limit is still OK; one step beyond raises the alarm.
    EXPECT_TRUE(evaluate(120, 90).empty());
    EXPECT_TRUE(evaluate(50, 90).empty());
    EXPECT_EQ(codes(evaluate(121, 98)), Codes{"HR_HIGH"});
    EXPECT_EQ(codes(evaluate(49, 98)), Codes{"HR_LOW"});
    EXPECT_EQ(codes(evaluate(75, 89)), Codes{"SPO2_LOW"});
}

TEST_F(DefaultAlarmsTest, CriticalAlarmReplacesMediumAlarmOfSameSource)
{
    const auto alarms = evaluate(160, 98);
    ASSERT_EQ(alarms.size(), 1U);
    EXPECT_EQ(alarms[0].code, "HR_CRITICAL_HIGH");
    EXPECT_EQ(alarms[0].priority, AlarmPriority::High);

    EXPECT_EQ(codes(evaluate(35, 98)), Codes{"HR_CRITICAL_LOW"});
    EXPECT_EQ(codes(evaluate(75, 80)), Codes{"SPO2_CRITICAL_LOW"});
}

TEST_F(DefaultAlarmsTest, AlarmsAreSortedByPriorityDescending)
{
    // SpO2 critical (High) + HR high (Medium) + SpO2 probe off (Low)
    const auto alarms = evaluate(130, 80, false, true);

    ASSERT_EQ(alarms.size(), 3U);
    EXPECT_EQ(alarms[0].code, "SPO2_CRITICAL_LOW");
    EXPECT_EQ(alarms[1].code, "HR_HIGH");
    EXPECT_EQ(alarms[2].code, "SPO2_PROBE_OFF");
    EXPECT_TRUE(std::ranges::is_sorted(alarms, std::ranges::greater{}, &Alarm::priority));
}

TEST_F(DefaultAlarmsTest, MissingValuesNeverRaiseLimitAlarms)
{
    EXPECT_TRUE(evaluate(std::nullopt, std::nullopt).empty());
}

TEST_F(DefaultAlarmsTest, SensorOffRaisesLowPriorityTechnicalAlarms)
{
    const auto alarms = evaluate(std::nullopt, std::nullopt, true, true);

    ASSERT_EQ(alarms.size(), 2U);
    for (const Alarm& alarm : alarms) {
        EXPECT_EQ(alarm.category, AlarmCategory::Technical);
        EXPECT_EQ(alarm.priority, AlarmPriority::Low);
    }
    EXPECT_EQ(codes(alarms), (Codes{"ECG_LEADS_OFF", "SPO2_PROBE_OFF"}));
}

TEST_F(DefaultAlarmsTest, LinkLostSuppressesSensorAlarms)
{
    // With no link, the last sensor flags are stale - only report the link.
    EXPECT_EQ(codes(evaluate(std::nullopt, std::nullopt, true, true, true)), Codes{"DEVICE_LINK_LOST"});
}

TEST(AlarmEvaluator, CustomLimitsAreUsed)
{
    AlarmLimits limits;
    limits.heartRateHigh = 100;
    const AlarmEvaluator evaluator(makeDefaultAlarmRules(limits));

    const auto alarms = evaluator.evaluate(AlarmContext{VitalSigns{105, 98, false, false}, false});
    ASSERT_EQ(alarms.size(), 1U);
    EXPECT_EQ(alarms[0].message, "HR high (> 100 bpm)");
}

TEST(AlarmEvaluator, RejectsInvalidLimits)
{
    AlarmLimits limits;
    limits.heartRateLow = 130; // above heartRateHigh
    EXPECT_THROW((void)makeDefaultAlarmRules(limits), std::invalid_argument);
}

TEST(AlarmEvaluator, RejectsNullRule)
{
    std::vector<std::unique_ptr<IAlarmRule>> rules;
    rules.push_back(nullptr);
    EXPECT_THROW(AlarmEvaluator{std::move(rules)}, std::invalid_argument);
}

TEST(AlarmEvaluator, AcceptsCustomRuleThroughInterface)
{
    // Open/Closed: a new rule type plugs in without touching the evaluator.
    class AlwaysOnRule final : public IAlarmRule {
    public:
        std::optional<Alarm> evaluate(const AlarmContext&) const override
        {
            return Alarm{"TEST", AlarmSource::DeviceLink, AlarmCategory::Technical, AlarmPriority::Low, "test"};
        }
    };
    std::vector<std::unique_ptr<IAlarmRule>> rules;
    rules.push_back(std::make_unique<AlwaysOnRule>());
    const AlarmEvaluator evaluator(std::move(rules));

    EXPECT_EQ(evaluator.evaluate({}).size(), 1U);
}
