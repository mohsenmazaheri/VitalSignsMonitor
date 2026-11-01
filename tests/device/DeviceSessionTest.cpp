#include "FakeTransport.h"

#include "vsm/device/DeviceSession.h"
#include "vsm/protocol/Frame.h"
#include "vsm/protocol/Messages.h"

#include <QElapsedTimer>
#include <QEventLoop>
#include <QTimer>

#include <gtest/gtest.h>

using namespace std::chrono_literals;
using vsm::device::DeviceSession;
using vsm::device::ITransport;
using vsm::test::FakeTransport;
namespace protocol = vsm::protocol;

namespace {

std::vector<std::uint8_t> encode(const protocol::Message& message, std::uint16_t sequence)
{
    return protocol::encodeFrame(protocol::toFrame(message, sequence));
}

/// Runs the Qt event loop until @p condition is true or @p timeout expires.
template <typename Condition>
bool waitUntil(Condition condition, std::chrono::milliseconds timeout = 1000ms)
{
    QElapsedTimer clock;
    clock.start();
    while (!condition() && clock.elapsed() < timeout.count()) {
        QEventLoop loop;
        QTimer::singleShot(5, &loop, &QEventLoop::quit);
        loop.exec();
    }
    return condition();
}

/// Session wired to a FakeTransport; the fake stays owned by the session.
class DeviceSessionTest : public ::testing::Test {
protected:
    DeviceSessionTest()
    {
        auto transport = std::make_unique<FakeTransport>();
        fake = transport.get();
        session = std::make_unique<DeviceSession>(std::move(transport));

        QObject::connect(session.get(), &DeviceSession::vitalsReceived,
                         [this](const vsm::core::VitalSigns& v) { vitals.push_back(v); });
        QObject::connect(session.get(), &DeviceSession::ecgSamplesReceived,
                         [this](const QList<float>& mv, int rate) {
                             ecg.append(mv);
                             sampleRate = rate;
                         });
        QObject::connect(session.get(), &DeviceSession::connectionStateChanged,
                         [this](ITransport::State s) { states.push_back(s); });
        QObject::connect(session.get(), &DeviceSession::linkLostChanged,
                         [this](bool lost) { linkLost.push_back(lost); });
    }

    // Recorders are declared before the session: the session emits signals
    // from its destructor (stop()), so they must still be alive then.
    std::vector<vsm::core::VitalSigns> vitals;
    QList<float> ecg;
    int sampleRate{0};
    std::vector<ITransport::State> states;
    std::vector<bool> linkLost;

    FakeTransport* fake{nullptr};
    std::unique_ptr<DeviceSession> session;
};

} // namespace

TEST_F(DeviceSessionTest, StartOpensTransportOnce)
{
    session->start();
    session->start(); // idempotent

    EXPECT_EQ(fake->openCount, 1);
    ASSERT_FALSE(states.empty());
    EXPECT_EQ(states.back(), ITransport::State::Connected);
}

TEST_F(DeviceSessionTest, MapsNumericsToDomainVitals)
{
    session->start();
    fake->inject(encode(protocol::NumericsMessage{72, 97, false, true}, 0));

    ASSERT_EQ(vitals.size(), 1U);
    EXPECT_EQ(vitals[0].heartRateBpm, 72);
    EXPECT_EQ(vitals[0].spo2Percent, 97);
    EXPECT_FALSE(vitals[0].ecgLeadsOff);
    EXPECT_TRUE(vitals[0].spo2ProbeOff);
}

TEST_F(DeviceSessionTest, InvalidNumericsBecomeEmptyOptionals)
{
    session->start();
    fake->inject(encode(protocol::NumericsMessage{std::nullopt, std::nullopt, true, false}, 0));

    ASSERT_EQ(vitals.size(), 1U);
    EXPECT_FALSE(vitals[0].heartRateBpm.has_value());
    EXPECT_FALSE(vitals[0].spo2Percent.has_value());
}

TEST_F(DeviceSessionTest, ConvertsEcgMicrovoltsToMillivolts)
{
    session->start();
    fake->inject(encode(protocol::EcgWaveformMessage{250, {1000, -500, 0}}, 0));

    ASSERT_EQ(ecg.size(), 3);
    EXPECT_FLOAT_EQ(ecg[0], 1.0F);
    EXPECT_FLOAT_EQ(ecg[1], -0.5F);
    EXPECT_FLOAT_EQ(ecg[2], 0.0F);
    EXPECT_EQ(sampleRate, 250);
}

TEST_F(DeviceSessionTest, DecodesFramesSplitAcrossChunks)
{
    session->start();
    const auto bytes = encode(protocol::NumericsMessage{80, 95, false, false}, 0);
    for (const std::uint8_t byte : bytes) {
        fake->inject({byte});
    }
    EXPECT_EQ(vitals.size(), 1U);
}

TEST_F(DeviceSessionTest, CountsSequenceGapsAndCrcErrors)
{
    session->start();
    fake->inject(encode(protocol::HeartbeatMessage{1}, 10));
    fake->inject(encode(protocol::HeartbeatMessage{2}, 13)); // 11 and 12 lost

    auto corrupted = encode(protocol::HeartbeatMessage{3}, 14);
    corrupted.back() ^= 0xFF;
    fake->inject(corrupted);

    const auto stats = session->statistics();
    EXPECT_EQ(stats.framesReceived, 2U);
    EXPECT_EQ(stats.sequenceGaps, 1U);
    EXPECT_EQ(stats.framesLost, 2U);
    EXPECT_EQ(stats.crcErrors, 1U);
}

TEST_F(DeviceSessionTest, SequenceWrapAroundIsNotAGap)
{
    session->start();
    fake->inject(encode(protocol::HeartbeatMessage{1}, 0xFFFF));
    fake->inject(encode(protocol::HeartbeatMessage{2}, 0x0000));

    EXPECT_EQ(session->statistics().sequenceGaps, 0U);
}

TEST_F(DeviceSessionTest, WatchdogReportsLostLinkAndRecoveryClearsIt)
{
    session->setWatchdogTimeout(50ms);
    session->start();

    ASSERT_TRUE(waitUntil([&] { return session->isLinkLost(); }));

    fake->inject(encode(protocol::HeartbeatMessage{1}, 0));
    EXPECT_FALSE(session->isLinkLost());
    EXPECT_EQ(linkLost, (std::vector<bool>{true, false}));
}

TEST_F(DeviceSessionTest, ReconnectsAfterUnexpectedDisconnect)
{
    session->setReconnectDelay(10ms);
    session->start();
    fake->simulateDrop();

    EXPECT_TRUE(waitUntil([&] { return fake->openCount == 2; }));
}

TEST_F(DeviceSessionTest, StopDoesNotReconnect)
{
    session->setReconnectDelay(10ms);
    session->start();
    session->stop();

    waitUntil([] { return false; }, 50ms);
    EXPECT_EQ(fake->openCount, 1);
    EXPECT_EQ(fake->state(), ITransport::State::Disconnected);
}
