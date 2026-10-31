#include "DeviceSimulator.h"

#include "vsm/protocol/FrameDecoder.h"


#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

namespace vsm::test {

using namespace simulator;

/// In-memory IByteSink that keeps everything it is given.
class FakeSink final : public IByteSink {
    Q_OBJECT

public:
    using IByteSink::IByteSink;

    bool open(QString& /*error*/) override
    {
        m_open = true;
        return true;
    }
    void close() override { m_open = false; }
    [[nodiscard]] bool isOpen() const override { return m_open; }
    void write(const QByteArray& bytes) override { data.append(bytes); }
    [[nodiscard]] QString description() const override { return QStringLiteral("fake://sink"); }
    [[nodiscard]] QString status() const override { return {}; }
    [[nodiscard]] bool hasReceiver() const override { return m_open; }

    QByteArray data;

private:
    bool m_open{false};
};

/// Decoded content of the byte stream a test produced.
struct Received {
    std::vector<protocol::Frame> frames;
    std::vector<protocol::NumericsMessage> numerics;
    std::vector<std::int16_t> ecgSamples;
    protocol::DecoderStatistics decoder;
};

class DeviceSimulatorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        QString error;
        ASSERT_TRUE(sink.open(error));
        simulator.setSink(&sink);
    }

    /// Advances the fake clock in 40 ms steps (like the real timer) for @p ms.
    void run(qint64 ms)
    {
        const qint64 end = now + ms;
        while (now < end) {
            now += DeviceSimulator::kTickIntervalMs;
            simulator.tick();
        }
    }

    [[nodiscard]] Received decode() const
    {
        protocol::FrameDecoder decoder;
        Received result;
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(sink.data.constData());
        result.frames = decoder.feed({bytes, static_cast<std::size_t>(sink.data.size())});
        result.decoder = decoder.statistics();
        for (const protocol::Frame& frame : result.frames) {
            const auto message = protocol::parseMessage(frame);
            if (!message) {
                continue;
            }
            if (const auto* numerics = std::get_if<protocol::NumericsMessage>(&*message)) {
                result.numerics.push_back(*numerics);
            } else if (const auto* ecg = std::get_if<protocol::EcgWaveformMessage>(&*message)) {
                result.ecgSamples.insert(result.ecgSamples.end(), ecg->samplesMicrovolts.begin(),
                                         ecg->samplesMicrovolts.end());
            }
        }
        return result;
    }

    qint64 now{0};
    FakeSink sink;
    DeviceSimulator simulator{250, [this] { return now; }};
};

TEST_F(DeviceSimulatorTest, GeneratesExactlyTheSampleRate)
{
    simulator.start();
    run(2000);

    EXPECT_EQ(decode().ecgSamples.size(), 500U); // 2 s x 250 Hz
}

TEST_F(DeviceSimulatorTest, SendsNumericsOncePerSecondWithManualTargets)
{
    simulator.setManualTargets({75, 98, false, false});
    simulator.start();
    run(2000);

    const Received received = decode();
    ASSERT_EQ(received.numerics.size(), 3U); // t = 0, 1 s, 2 s
    EXPECT_EQ(received.numerics.back().heartRateBpm, 75);
    EXPECT_EQ(received.numerics.back().spo2Percent, 98);
}

TEST_F(DeviceSimulatorTest, HeartRateMovesGraduallyTowardsTheTarget)
{
    simulator.setManualTargets({75, 98, false, false});
    simulator.start();
    simulator.setManualTargets({135, 98, false, false});
    run(10000);

    // 3 bpm per second: about 105 after 10 s, not a jump to 135.
    const auto heartRate = decode().numerics.back().heartRateBpm;
    ASSERT_TRUE(heartRate.has_value());
    EXPECT_GE(*heartRate, 100);
    EXPECT_LE(*heartRate, 110);
}

TEST_F(DeviceSimulatorTest, SensorsOffReportInvalidValuesAndFlatEcg)
{
    simulator.setManualTargets({75, 98, true, true});
    simulator.start();
    run(1000);

    const Received received = decode();
    ASSERT_FALSE(received.numerics.empty());
    EXPECT_FALSE(received.numerics.back().heartRateBpm.has_value());
    EXPECT_FALSE(received.numerics.back().spo2Percent.has_value());
    EXPECT_TRUE(received.numerics.back().ecgLeadsOff);
    EXPECT_TRUE(received.numerics.back().spo2ProbeOff);
    EXPECT_TRUE(std::ranges::all_of(received.ecgSamples, [](std::int16_t s) { return s == 0; }));
}

TEST_F(DeviceSimulatorTest, UnpluggedCableSendsNothingButKeepsNumbering)
{
    simulator.start();
    run(1000);
    const qsizetype bytesBefore = sink.data.size();
    const std::size_t framesBefore = decode().frames.size();

    simulator.setTransmissionEnabled(false);
    run(1000);
    EXPECT_EQ(sink.data.size(), bytesBefore);
    const quint64 dropped = simulator.statistics().framesDropped;
    EXPECT_GT(dropped, 0U);

    simulator.setTransmissionEnabled(true);
    run(200);

    // The first frame after the reconnect skips exactly the dropped sequence numbers.
    const Received received = decode();
    ASSERT_GT(received.frames.size(), framesBefore);
    const std::uint16_t lastBefore = received.frames[framesBefore - 1].sequence;
    const std::uint16_t firstAfter = received.frames[framesBefore].sequence;
    EXPECT_EQ(static_cast<std::uint16_t>(firstAfter - lastBefore - 1), dropped);
}

TEST_F(DeviceSimulatorTest, NoSinkDropsFramesWithoutCrashing)
{
    simulator.setSink(nullptr);
    simulator.start();
    run(500);

    EXPECT_EQ(simulator.statistics().framesSent, 0U);
    EXPECT_GT(simulator.statistics().framesDropped, 0U);
}

TEST_F(DeviceSimulatorTest, FullCorruptionMakesEveryFrameFailTheCrc)
{
    simulator.setCorruptionProbability(1.0);
    simulator.start();
    run(1000);

    const Received received = decode();
    EXPECT_EQ(simulator.statistics().framesCorrupted, simulator.statistics().framesSent);
    // A flipped byte can also hit the header; either way no frame may pass.
    EXPECT_TRUE(received.frames.empty());
    EXPECT_GT(received.decoder.crcErrors + received.decoder.invalidHeaders + received.decoder.bytesDiscarded, 0U);
}

TEST_F(DeviceSimulatorTest, ScenarioDrivesTargetsAndPhase)
{
    int phaseChanges = 0;
    QObject::connect(&simulator, &DeviceSimulator::phaseChanged, [&] { ++phaseChanges; });

    simulator.runScenario(*makeScenario("tachycardia"));
    simulator.start();
    EXPECT_TRUE(simulator.isScenarioRunning());
    EXPECT_EQ(simulator.scenarioName(), QStringLiteral("tachycardia"));
    EXPECT_EQ(simulator.phaseLabel(), QStringLiteral("Normal"));

    run(21000); // first phase lasts 20 s
    EXPECT_EQ(simulator.phaseLabel(), QStringLiteral("Tachycardia"));
    EXPECT_EQ(simulator.targets().heartRateBpm, 145);
    EXPECT_EQ(phaseChanges, 2);
}

TEST_F(DeviceSimulatorTest, ManualTargetsEndTheScenario)
{
    int modeChanges = 0;
    QObject::connect(&simulator, &DeviceSimulator::modeChanged, [&] { ++modeChanges; });
    simulator.runScenario(*makeScenario("demo"));
    simulator.setManualTargets({60, 95, false, false});

    EXPECT_FALSE(simulator.isScenarioRunning());
    EXPECT_TRUE(simulator.scenarioName().isEmpty());
    EXPECT_EQ(simulator.phaseLabel(), QStringLiteral("Manual"));
    EXPECT_EQ(simulator.targets().heartRateBpm, 60);
    EXPECT_EQ(modeChanges, 2);
}

TEST_F(DeviceSimulatorTest, PowerOffStopsGeneration)
{
    simulator.start();
    EXPECT_TRUE(simulator.isRunning());
    simulator.stop();
    EXPECT_FALSE(simulator.isRunning());
}

} // namespace vsm::test

#include "DeviceSimulatorTest.moc"
