#include "vsm/core/AlarmEvaluator.h"
#include "vsm/core/AlarmManager.h"
#include "vsm/core/AlarmRules.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>

using namespace vsm::core;
using namespace std::chrono_literals;

namespace {

class MemoryAuditLog final : public IAuditLog {
public:
    void record(const AuditEvent& event) override { events.push_back(event); }

    [[nodiscard]] std::size_t count(AuditEventType type) const
    {
        return static_cast<std::size_t>(std::ranges::count_if(events, [type](const AuditEvent& e) { return e.type == type; }));
    }

    std::vector<AuditEvent> events;
};

Alarm hrCritical()
{
    return {"HR_CRITICAL_HIGH", AlarmSource::HeartRate, AlarmCategory::Physiological, AlarmPriority::High, "HR extremely high"};
}

Alarm hrHigh()
{
    return {"HR_HIGH", AlarmSource::HeartRate, AlarmCategory::Physiological, AlarmPriority::Medium, "HR high"};
}

Alarm spo2Low()
{
    return {"SPO2_LOW", AlarmSource::Spo2, AlarmCategory::Physiological, AlarmPriority::Medium, "SpO2 low"};
}

Alarm leadsOff()
{
    return {"ECG_LEADS_OFF", AlarmSource::EcgLeads, AlarmCategory::Technical, AlarmPriority::Low, "ECG leads off"};
}

class AlarmManagerTest : public ::testing::Test {
protected:
    MemoryAuditLog audit;
    AlarmManager manager{audit};

    [[nodiscard]] AlarmState stateOf(const std::string& code) const
    {
        const auto& alarms = manager.alarms();
        const auto it = std::ranges::find_if(alarms, [&code](const ManagedAlarm& m) { return m.alarm.code == code; });
        EXPECT_NE(it, alarms.end()) << code;
        return it == alarms.end() ? AlarmState::Active : it->state;
    }
};

} // namespace

// --- Life cycle -------------------------------------------------------------------

TEST_F(AlarmManagerTest, StartsSilentWithoutAlarms)
{
    EXPECT_TRUE(manager.alarms().empty());
    EXPECT_FALSE(manager.hasUnacknowledged());
    EXPECT_FALSE(manager.soundingPriority().has_value());
}

TEST_F(AlarmManagerTest, NewAlarmIsActiveAudibleAndLogged)
{
    manager.update({hrHigh()}, 0ms);

    EXPECT_EQ(stateOf("HR_HIGH"), AlarmState::Active);
    EXPECT_EQ(manager.soundingPriority(), AlarmPriority::Medium);
    ASSERT_EQ(audit.count(AuditEventType::AlarmRaised), 1U);
    EXPECT_EQ(audit.events.back().code, "HR_HIGH");
    EXPECT_EQ(audit.events.back().priority, AlarmPriority::Medium);
}

TEST_F(AlarmManagerTest, RepeatedUpdateDoesNotLogAgain)
{
    manager.update({hrHigh()}, 0ms);
    manager.update({hrHigh()}, 1s);
    manager.update({hrHigh()}, 2s);

    EXPECT_EQ(audit.events.size(), 1U);
}

TEST_F(AlarmManagerTest, MediumAlarmEndsWithItsCondition)
{
    manager.update({hrHigh()}, 0ms);
    manager.update({}, 1s);

    EXPECT_TRUE(manager.alarms().empty());
    EXPECT_FALSE(manager.soundingPriority().has_value());
    EXPECT_EQ(audit.count(AuditEventType::AlarmEnded), 1U);
}

TEST_F(AlarmManagerTest, HighAlarmLatchesAfterItsConditionEnds)
{
    manager.update({hrCritical()}, 0ms);
    manager.update({}, 1s);

    EXPECT_EQ(stateOf("HR_CRITICAL_HIGH"), AlarmState::Latched);
    EXPECT_EQ(manager.soundingPriority(), AlarmPriority::High);
    EXPECT_EQ(audit.count(AuditEventType::AlarmLatched), 1U);
}

TEST_F(AlarmManagerTest, LatchedAlarmStaysUntilAcknowledged)
{
    manager.update({hrCritical()}, 0ms);
    for (int second = 1; second < 100; ++second) {
        manager.update({}, std::chrono::seconds(second));
    }
    EXPECT_EQ(stateOf("HR_CRITICAL_HIGH"), AlarmState::Latched);

    manager.acknowledgeAll();

    EXPECT_TRUE(manager.alarms().empty());
    EXPECT_FALSE(manager.soundingPriority().has_value());
    EXPECT_EQ(audit.count(AuditEventType::AlarmReset), 1U);
}

TEST_F(AlarmManagerTest, LatchedAlarmBecomesActiveWhenTheConditionReturns)
{
    manager.update({hrCritical()}, 0ms);
    manager.update({}, 1s);
    manager.update({hrCritical()}, 2s);

    EXPECT_EQ(stateOf("HR_CRITICAL_HIGH"), AlarmState::Active);
    EXPECT_EQ(audit.count(AuditEventType::AlarmReactivated), 1U);
}

// --- Acknowledge ------------------------------------------------------------------

TEST_F(AlarmManagerTest, AcknowledgeSilencesButKeepsTheAlarmVisible)
{
    manager.update({hrHigh()}, 0ms);
    manager.acknowledgeAll();

    EXPECT_EQ(stateOf("HR_HIGH"), AlarmState::Acknowledged);
    EXPECT_FALSE(manager.hasUnacknowledged());
    EXPECT_FALSE(manager.soundingPriority().has_value());
    EXPECT_EQ(audit.count(AuditEventType::AlarmAcknowledged), 1U);
}

TEST_F(AlarmManagerTest, AcknowledgedHighAlarmDoesNotLatch)
{
    manager.update({hrCritical()}, 0ms);
    manager.acknowledgeAll();
    manager.update({}, 1s);

    EXPECT_TRUE(manager.alarms().empty());
    EXPECT_EQ(audit.count(AuditEventType::AlarmLatched), 0U);
}

TEST_F(AlarmManagerTest, AlarmThatComesBackAfterAcknowledgeSoundsAgain)
{
    manager.update({hrHigh()}, 0ms);
    manager.acknowledgeAll();
    manager.update({}, 1s);
    manager.update({hrHigh()}, 2s);

    EXPECT_EQ(stateOf("HR_HIGH"), AlarmState::Active);
    EXPECT_EQ(manager.soundingPriority(), AlarmPriority::Medium);
}

TEST_F(AlarmManagerTest, EscalationAfterAcknowledgeSoundsAgain)
{
    manager.update({hrHigh()}, 0ms);
    manager.acknowledgeAll();
    manager.update({hrCritical()}, 1s); // the evaluator reports one alarm per source

    EXPECT_EQ(manager.alarms().size(), 1U);
    EXPECT_EQ(stateOf("HR_CRITICAL_HIGH"), AlarmState::Active);
    EXPECT_EQ(manager.soundingPriority(), AlarmPriority::High);
}

TEST_F(AlarmManagerTest, AcknowledgeWithNothingToDoLogsNothing)
{
    manager.acknowledgeAll();
    manager.update({hrHigh()}, 0ms);
    manager.acknowledgeAll();
    const std::size_t logged = audit.events.size();

    manager.acknowledgeAll();

    EXPECT_EQ(audit.events.size(), logged);
}

// --- Audio pause ------------------------------------------------------------------

TEST_F(AlarmManagerTest, AudioPauseSilencesForTwoMinutesThenSoundReturns)
{
    manager.update({hrHigh()}, 0ms);
    manager.pauseAudio(10s);

    EXPECT_TRUE(manager.audioPaused());
    EXPECT_FALSE(manager.soundingPriority().has_value());
    EXPECT_EQ(manager.audioPauseRemaining(70s), 60s);

    manager.tick(129s);
    EXPECT_TRUE(manager.audioPaused());

    manager.tick(130s);
    EXPECT_FALSE(manager.audioPaused());
    EXPECT_EQ(manager.soundingPriority(), AlarmPriority::Medium);
    EXPECT_EQ(manager.audioPauseRemaining(130s), 0ms);
    EXPECT_EQ(audit.count(AuditEventType::AudioPauseExpired), 1U);
}

TEST_F(AlarmManagerTest, AudioPauseDurationComesFromThePolicy)
{
    AlarmManager shortPause{audit, AlarmPolicy{5s}};
    shortPause.update({hrHigh()}, 0ms);
    shortPause.pauseAudio(0ms);

    shortPause.tick(5s);

    EXPECT_FALSE(shortPause.audioPaused());
}

TEST_F(AlarmManagerTest, AudioPauseWithoutAlarmsIsRefused)
{
    manager.pauseAudio(0ms); // otherwise it would hide the next alarm

    EXPECT_FALSE(manager.audioPaused());
    EXPECT_TRUE(audit.events.empty());
}

TEST_F(AlarmManagerTest, OperatorCanEndTheAudioPause)
{
    manager.update({hrHigh()}, 0ms);
    manager.pauseAudio(0ms);

    manager.resumeAudio();

    EXPECT_FALSE(manager.audioPaused());
    EXPECT_EQ(manager.soundingPriority(), AlarmPriority::Medium);
    EXPECT_EQ(audit.count(AuditEventType::AudioResumed), 1U);
}

TEST_F(AlarmManagerTest, NewMediumAlarmEndsTheAudioPause)
{
    manager.update({hrHigh()}, 0ms);
    manager.pauseAudio(0ms);

    manager.update({hrHigh(), spo2Low()}, 1s);

    EXPECT_FALSE(manager.audioPaused());
    EXPECT_EQ(manager.soundingPriority(), AlarmPriority::Medium);
    EXPECT_EQ(audit.count(AuditEventType::AudioPauseEnded), 1U);
    EXPECT_EQ(audit.count(AuditEventType::AudioResumed), 0U); // the operator did not do this
}

TEST_F(AlarmManagerTest, NewLowAlarmKeepsTheAudioPause)
{
    manager.update({hrHigh()}, 0ms);
    manager.pauseAudio(0ms);

    manager.update({hrHigh(), leadsOff()}, 1s);

    EXPECT_TRUE(manager.audioPaused());
    EXPECT_FALSE(manager.soundingPriority().has_value());
}

TEST_F(AlarmManagerTest, AudioPauseEndsWhenNothingIsLeftToSilence)
{
    manager.update({hrHigh()}, 0ms);
    manager.pauseAudio(0ms);

    manager.update({}, 1s);

    EXPECT_FALSE(manager.audioPaused());
}

TEST_F(AlarmManagerTest, AcknowledgingEverythingEndsTheAudioPause)
{
    manager.update({hrHigh()}, 0ms);
    manager.pauseAudio(0ms);

    manager.acknowledgeAll();

    EXPECT_FALSE(manager.audioPaused());
}

// --- Order and sound --------------------------------------------------------------

TEST_F(AlarmManagerTest, SoundFollowsTheMostUrgentUnacknowledgedAlarm)
{
    manager.update({hrCritical(), spo2Low(), leadsOff()}, 0ms);
    EXPECT_EQ(manager.soundingPriority(), AlarmPriority::High);

    manager.acknowledgeAll();
    manager.update({hrCritical(), spo2Low(), leadsOff()}, 1s);
    EXPECT_FALSE(manager.soundingPriority().has_value());
}

TEST_F(AlarmManagerTest, ListIsSortedByPriorityThenState)
{
    manager.update({spo2Low(), leadsOff()}, 0ms);
    manager.acknowledgeAll();
    manager.update({hrHigh(), spo2Low(), leadsOff()}, 1s);
    manager.update({hrCritical(), spo2Low(), leadsOff()}, 2s);

    const auto& alarms = manager.alarms();
    ASSERT_EQ(alarms.size(), 3U);
    EXPECT_EQ(alarms[0].alarm.code, "HR_CRITICAL_HIGH");
    EXPECT_EQ(alarms[1].alarm.code, "SPO2_LOW");
    EXPECT_EQ(alarms[2].alarm.code, "ECG_LEADS_OFF");
}

TEST_F(AlarmManagerTest, LatchedHighStaysAboveNewMediumOfTheSameSource)
{
    manager.update({hrCritical()}, 0ms);
    manager.update({hrHigh()}, 1s); // HR fell from > 150 to > 120

    const auto& alarms = manager.alarms();
    ASSERT_EQ(alarms.size(), 2U);
    EXPECT_EQ(alarms[0].alarm.code, "HR_CRITICAL_HIGH");
    EXPECT_EQ(alarms[0].state, AlarmState::Latched);
    EXPECT_EQ(alarms[1].alarm.code, "HR_HIGH");
    EXPECT_EQ(alarms[1].state, AlarmState::Active);
}

TEST_F(AlarmManagerTest, ClearForgetsEverythingIncludingLatchedAlarms)
{
    manager.update({hrCritical()}, 0ms);
    manager.update({}, 1s);
    manager.pauseAudio(1s);

    manager.clear();

    EXPECT_TRUE(manager.alarms().empty());
    EXPECT_FALSE(manager.audioPaused());
    EXPECT_EQ(audit.count(AuditEventType::AlarmsCleared), 1U);
}

// --- Together with the evaluator ----------------------------------------------------

TEST_F(AlarmManagerTest, ShortCriticalEpisodeIsNotLost)
{
    const AlarmEvaluator evaluator{makeDefaultAlarmRules(AlarmLimits{})};
    const auto feed = [&](int heartRate, std::chrono::seconds now) {
        manager.update(evaluator.evaluate(AlarmContext{VitalSigns{heartRate, 98, false, false}, false}), now);
    };

    feed(80, 0s);
    EXPECT_TRUE(manager.alarms().empty());

    feed(170, 1s); // one second of critical heart rate ...
    feed(80, 2s);  // ... and back to normal

    EXPECT_EQ(stateOf("HR_CRITICAL_HIGH"), AlarmState::Latched);
    EXPECT_EQ(manager.soundingPriority(), AlarmPriority::High);
}

TEST(AuditEvents, OnlyOperatorKeysCountAsOperatorActions)
{
    EXPECT_TRUE(isOperatorAction(AuditEventType::AlarmAcknowledged));
    EXPECT_TRUE(isOperatorAction(AuditEventType::AudioPaused));
    EXPECT_TRUE(isOperatorAction(AuditEventType::AudioResumed));
    EXPECT_TRUE(isOperatorAction(AuditEventType::ConnectRequested));
    EXPECT_FALSE(isOperatorAction(AuditEventType::AlarmRaised));
    EXPECT_FALSE(isOperatorAction(AuditEventType::LinkLost));
    EXPECT_FALSE(isOperatorAction(AuditEventType::AudioPauseEnded));
    EXPECT_FALSE(isOperatorAction(AuditEventType::AudioPauseExpired));
}
