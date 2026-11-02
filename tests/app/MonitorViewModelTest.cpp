#include "AuditLogModel.h"
#include "FakeTransport.h"
#include "IAlarmSound.h"
#include "MonitorViewModel.h"

#include "vsm/core/AlarmRules.h"

#include <QElapsedTimer>
#include <QEventLoop>
#include <QTimer>

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <memory>

using namespace vsm;
using State = device::ITransport::State;

namespace {

class MemoryAuditLog final : public core::IAuditLog {
public:
    void record(const core::AuditEvent& event) override { events.push_back(event); }

    [[nodiscard]] std::size_t count(core::AuditEventType type) const
    {
        return static_cast<std::size_t>(
            std::ranges::count_if(events, [type](const core::AuditEvent& e) { return e.type == type; }));
    }

    std::vector<core::AuditEvent> events;
};

class FakeSound final : public app::IAlarmSound {
public:
    void setPriority(std::optional<core::AlarmPriority> priority) override { current = priority; }

    std::optional<core::AlarmPriority> current;
};

/// The real view model and alarm chain. Only the transport and the sound are fake.
/// The tests emit the DeviceController's signals themselves, so they decide when
/// data arrives and when the link is lost.
class MonitorViewModelTest : public ::testing::Test {
protected:
    MonitorViewModelTest()
    {
        monitor.connectTcp(QStringLiteral("127.0.0.1"), 5555);

        QElapsedTimer clock;
        clock.start();
        while (!monitor.connected() && clock.elapsed() < 1000) {
            QEventLoop loop;
            QTimer::singleShot(5, &loop, &QEventLoop::quit);
            loop.exec();
        }
    }

    void receive(int heartRate, int spo2 = 98)
    {
        emit device.vitalsReceived(core::VitalSigns{heartRate, spo2, false, false});
    }

    [[nodiscard]] std::size_t ecgSamples()
    {
        const auto& buffer = monitor.ecg()->buffer();
        return buffer.filledCount();
    }

    MemoryAuditLog audit;
    app::AuditLogModel auditModel{audit, QStringLiteral("memory")};
    core::AlarmManager manager{auditModel};
    FakeSound sound;
    device::DeviceController device{[](const device::ConnectionSettings&) -> std::unique_ptr<device::ITransport> {
        return std::make_unique<test::FakeTransport>();
    }};
    app::MonitorViewModel monitor{device, core::AlarmEvaluator(core::makeDefaultAlarmRules(core::AlarmLimits{})), manager,
                                  sound, auditModel};
};

} // namespace

TEST_F(MonitorViewModelTest, VitalsReachTheTilesAndRaiseTheAlarm)
{
    receive(160, 85);

    EXPECT_EQ(monitor.heartRate(), 160);
    EXPECT_EQ(monitor.spo2(), 85);
    EXPECT_EQ(monitor.heartRateAlarmPriority(), static_cast<int>(core::AlarmPriority::High));
    EXPECT_EQ(monitor.spo2AlarmPriority(), static_cast<int>(core::AlarmPriority::Medium));
    EXPECT_EQ(monitor.alarms()->topMessage().contains(QStringLiteral("HR extremely high")), true);
}

TEST_F(MonitorViewModelTest, LinkLossClearsNumbersAndWaveform)
{
    receive(80);
    emit device.ecgSamplesReceived({0.1F, 0.2F, 0.3F}, 250);
    ASSERT_EQ(monitor.heartRate(), 80);
    ASSERT_GT(ecgSamples(), 0U);

    emit device.linkLostChanged(true);

    EXPECT_EQ(monitor.heartRate(), -1); // QML shows "---"
    EXPECT_EQ(monitor.spo2(), -1);
    EXPECT_EQ(ecgSamples(), 0U);
}

TEST_F(MonitorViewModelTest, LinkLossRaisesAndLogsTheAlarm)
{
    emit device.linkLostChanged(true);

    EXPECT_EQ(monitor.alarms()->topMessage(), QStringLiteral("No data from device"));
    EXPECT_EQ(sound.current, core::AlarmPriority::Medium);
    EXPECT_EQ(audit.count(core::AuditEventType::LinkLost), 1U);

    emit device.linkLostChanged(false);

    EXPECT_EQ(monitor.alarms()->rowCount(), 0);
    EXPECT_EQ(audit.count(core::AuditEventType::LinkRestored), 1U);
}

TEST_F(MonitorViewModelTest, SoundFollowsTheAlarmAndStopsOnAcknowledge)
{
    receive(80);
    EXPECT_FALSE(sound.current.has_value());

    receive(160);
    EXPECT_EQ(sound.current, core::AlarmPriority::High);
    EXPECT_TRUE(monitor.alarmFlashing());

    monitor.acknowledgeAlarms();

    EXPECT_FALSE(sound.current.has_value());
    EXPECT_FALSE(monitor.alarmFlashing());
    EXPECT_FALSE(monitor.canAcknowledge());
    EXPECT_EQ(monitor.alarms()->rowCount(), 1); // still visible while the condition lasts
    EXPECT_EQ(audit.count(core::AuditEventType::AlarmAcknowledged), 1U);
}

TEST_F(MonitorViewModelTest, LatchedAlarmStaysOnTheBannerButNotOnTheTile)
{
    receive(160);
    receive(80);

    EXPECT_EQ(monitor.alarms()->topState(), QStringLiteral("LATCHED"));
    EXPECT_EQ(sound.current, core::AlarmPriority::High);
    EXPECT_EQ(monitor.heartRateAlarmPriority(), 0); // the number shows the present
}

TEST_F(MonitorViewModelTest, AudioPauseCountsDownAndCanBeEnded)
{
    receive(160);

    monitor.toggleAudioPause();

    EXPECT_TRUE(monitor.audioPaused());
    EXPECT_EQ(monitor.audioPauseRemaining(), QStringLiteral("2:00"));
    EXPECT_FALSE(sound.current.has_value());
    EXPECT_TRUE(monitor.alarmFlashing()); // the visual signal keeps running

    monitor.toggleAudioPause();

    EXPECT_FALSE(monitor.audioPaused());
    EXPECT_EQ(sound.current, core::AlarmPriority::High);
}

TEST_F(MonitorViewModelTest, DisconnectForgetsLatchedAlarms)
{
    receive(160);
    receive(80);
    ASSERT_EQ(monitor.alarms()->rowCount(), 1);

    monitor.disconnectDevice();

    EXPECT_EQ(monitor.alarms()->rowCount(), 0);
    EXPECT_FALSE(sound.current.has_value());
    EXPECT_EQ(audit.count(core::AuditEventType::DisconnectRequested), 1U);
    EXPECT_EQ(audit.count(core::AuditEventType::AlarmsCleared), 1U);
}

TEST_F(MonitorViewModelTest, ReconnectLoopDoesNotFloodTheLog)
{
    ASSERT_EQ(audit.count(core::AuditEventType::Connected), 1U);

    emit device.connectionStateChanged(State::Disconnected);
    for (int attempt = 0; attempt < 10; ++attempt) {
        emit device.connectionStateChanged(State::Connecting);
        emit device.connectionStateChanged(State::Disconnected);
    }
    emit device.connectionStateChanged(State::Connected);

    EXPECT_EQ(audit.count(core::AuditEventType::ConnectionDropped), 1U);
    EXPECT_EQ(audit.count(core::AuditEventType::Connected), 2U);
}

TEST_F(MonitorViewModelTest, InvalidPortIsRefusedWithAMessage)
{
    monitor.disconnectDevice();

    monitor.connectTcp(QStringLiteral("127.0.0.1"), 0);

    EXPECT_FALSE(monitor.sessionActive());
    EXPECT_FALSE(monitor.lastError().isEmpty());
}

TEST_F(MonitorViewModelTest, EverythingLoggedAlsoAppearsOnScreen)
{
    receive(160);
    monitor.acknowledgeAlarms();

    EXPECT_EQ(auditModel.rowCount(), static_cast<int>(audit.events.size()));
    const QModelIndex newest = auditModel.index(0);
    EXPECT_EQ(auditModel.data(newest, app::AuditLogModel::EventRole).toString(), QStringLiteral("ALARM_ACKNOWLEDGED"));
    EXPECT_TRUE(auditModel.data(newest, app::AuditLogModel::OperatorRole).toBool());
}
