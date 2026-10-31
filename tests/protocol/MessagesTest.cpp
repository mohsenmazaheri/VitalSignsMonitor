#include "vsm/protocol/Messages.h"

#include <gtest/gtest.h>

using namespace vsm::protocol;

namespace {

/// Message -> frame -> bytes -> frame -> message.
Message roundTrip(const Message& message)
{
    const Frame frame = toFrame(message, 1);
    const auto parsed = parseMessage(frame);
    EXPECT_TRUE(parsed.has_value());
    return parsed.value_or(Message{});
}

} // namespace

TEST(Messages, HeartbeatRoundTrip)
{
    const Message message = HeartbeatMessage{123456789U};
    EXPECT_EQ(roundTrip(message), message);
    EXPECT_EQ(toFrame(message, 0).type, MessageType::Heartbeat);
}

TEST(Messages, NumericsRoundTripWithValidValues)
{
    const Message message = NumericsMessage{72, 97, false, false};
    EXPECT_EQ(roundTrip(message), message);
}

TEST(Messages, NumericsRoundTripWithInvalidValuesAndFlags)
{
    const Message message = NumericsMessage{std::nullopt, std::nullopt, true, true};
    const auto result = std::get<NumericsMessage>(roundTrip(message));

    EXPECT_FALSE(result.heartRateBpm.has_value());
    EXPECT_FALSE(result.spo2Percent.has_value());
    EXPECT_TRUE(result.ecgLeadsOff);
    EXPECT_TRUE(result.spo2ProbeOff);
}

TEST(Messages, EcgRoundTripKeepsNegativeSamples)
{
    const Message message = EcgWaveformMessage{250, {0, 1100, -250, -32768, 32767}};
    EXPECT_EQ(roundTrip(message), message);
}

TEST(Messages, LargestEcgBlockFitsIntoOneFrame)
{
    EcgWaveformMessage ecg;
    ecg.samplesMicrovolts.resize(kMaxEcgSamplesPerFrame);
    EXPECT_NO_THROW((void)encodeFrame(toFrame(ecg, 0)));
}

TEST(Messages, RejectsNumericsWithWrongPayloadSize)
{
    const Frame frame{MessageType::Numerics, 0, {0x48, 0x00, 0x61}}; // 3 instead of 4 bytes
    EXPECT_FALSE(parseMessage(frame).has_value());
}

TEST(Messages, RejectsEcgWhoseSampleCountDoesNotMatchPayload)
{
    // sample rate 250, count 3, but only 2 samples present
    const Frame frame{MessageType::EcgWaveform, 0, {0xFA, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00}};
    EXPECT_FALSE(parseMessage(frame).has_value());
}

TEST(Messages, RejectsEcgWithZeroSampleRate)
{
    const Frame frame{MessageType::EcgWaveform, 0, {0x00, 0x00, 0x00, 0x00}};
    EXPECT_FALSE(parseMessage(frame).has_value());
}

TEST(Messages, IgnoresUnknownMessageType)
{
    const Frame frame{static_cast<MessageType>(0x7E), 0, {}};
    EXPECT_FALSE(parseMessage(frame).has_value());
}
