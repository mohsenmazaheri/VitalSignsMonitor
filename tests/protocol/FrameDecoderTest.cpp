#include "vsm/protocol/FrameDecoder.h"

#include <gtest/gtest.h>

using namespace vsm::protocol;

namespace {

Frame makeFrame(std::uint16_t sequence, std::vector<std::uint8_t> payload = {0x01, 0x02, 0x03})
{
    return Frame{MessageType::EcgWaveform, sequence, std::move(payload)};
}

std::vector<std::uint8_t> concat(std::initializer_list<std::vector<std::uint8_t>> parts)
{
    std::vector<std::uint8_t> out;
    for (const auto& part : parts) {
        out.insert(out.end(), part.begin(), part.end());
    }
    return out;
}

} // namespace

TEST(FrameDecoder, DecodesSingleFrame)
{
    FrameDecoder decoder;
    const Frame frame = makeFrame(7);

    const auto frames = decoder.feed(encodeFrame(frame));

    ASSERT_EQ(frames.size(), 1U);
    EXPECT_EQ(frames[0], frame);
    EXPECT_EQ(decoder.statistics().framesDecoded, 1U);
    EXPECT_EQ(decoder.bufferedBytes(), 0U);
}

TEST(FrameDecoder, DecodesSeveralFramesFromOneChunk)
{
    FrameDecoder decoder;
    const auto bytes = concat({encodeFrame(makeFrame(1)), encodeFrame(makeFrame(2)), encodeFrame(makeFrame(3))});

    const auto frames = decoder.feed(bytes);

    ASSERT_EQ(frames.size(), 3U);
    EXPECT_EQ(frames[0].sequence, 1);
    EXPECT_EQ(frames[2].sequence, 3);
}

TEST(FrameDecoder, ReassemblesFrameDeliveredByteByByte)
{
    FrameDecoder decoder;
    const Frame frame = makeFrame(42);
    const auto bytes = encodeFrame(frame);

    std::vector<Frame> frames;
    for (const std::uint8_t byte : bytes) {
        auto decoded = decoder.feed(std::span(&byte, 1));
        frames.insert(frames.end(), decoded.begin(), decoded.end());
    }

    ASSERT_EQ(frames.size(), 1U);
    EXPECT_EQ(frames[0], frame);
}

TEST(FrameDecoder, SkipsLeadingGarbage)
{
    FrameDecoder decoder;
    const auto bytes = concat({{0x00, 0x13, 0xAA, 0x37, 0xFF}, encodeFrame(makeFrame(5))});

    const auto frames = decoder.feed(bytes);

    ASSERT_EQ(frames.size(), 1U);
    EXPECT_EQ(frames[0].sequence, 5);
    EXPECT_EQ(decoder.statistics().bytesDiscarded, 5U);
}

TEST(FrameDecoder, DropsFrameWithBadCrcAndRecoversOnNextFrame)
{
    FrameDecoder decoder;
    auto corrupted = encodeFrame(makeFrame(1));
    corrupted[kHeaderSize] ^= 0xFF; // flip payload bits
    const auto bytes = concat({corrupted, encodeFrame(makeFrame(2))});

    const auto frames = decoder.feed(bytes);

    ASSERT_EQ(frames.size(), 1U);
    EXPECT_EQ(frames[0].sequence, 2);
    EXPECT_EQ(decoder.statistics().crcErrors, 1U);
}

TEST(FrameDecoder, RejectsInvalidVersionAndResynchronises)
{
    FrameDecoder decoder;
    auto wrongVersion = encodeFrame(makeFrame(1));
    wrongVersion[2] = 0x7F;
    const auto bytes = concat({wrongVersion, encodeFrame(makeFrame(2))});

    const auto frames = decoder.feed(bytes);

    ASSERT_EQ(frames.size(), 1U);
    EXPECT_EQ(frames[0].sequence, 2);
    EXPECT_GE(decoder.statistics().invalidHeaders, 1U);
}

TEST(FrameDecoder, RejectsOversizedLengthWithoutWaitingForIt)
{
    FrameDecoder decoder;
    // Header claims 0xFFFF bytes of payload - must not block the stream.
    const std::vector<std::uint8_t> bogusHeader{kSyncByte1, kSyncByte2, kProtocolVersion, 0x10, 0, 0, 0xFF, 0xFF};
    const auto bytes = concat({bogusHeader, encodeFrame(makeFrame(9))});

    const auto frames = decoder.feed(bytes);

    ASSERT_EQ(frames.size(), 1U);
    EXPECT_EQ(frames[0].sequence, 9);
}

TEST(FrameDecoder, KeepsPartialSyncByteAtEndOfChunk)
{
    FrameDecoder decoder;
    const auto bytes = encodeFrame(makeFrame(3));

    EXPECT_TRUE(decoder.feed(std::span(bytes).first(1)).empty()); // only 0xAA
    EXPECT_EQ(decoder.bufferedBytes(), 1U);

    const auto frames = decoder.feed(std::span(bytes).subspan(1));
    ASSERT_EQ(frames.size(), 1U);
    EXPECT_EQ(decoder.statistics().bytesDiscarded, 0U);
}

TEST(FrameDecoder, BoundsMemoryOnEndlessGarbage)
{
    FrameDecoder decoder(/*maxBufferedBytes=*/64);
    // A valid-looking header followed by an unfinished payload keeps bytes buffered.
    auto partial = encodeFrame(makeFrame(1, std::vector<std::uint8_t>(500, 0x11)));
    partial.resize(200);

    (void)decoder.feed(partial);

    EXPECT_LE(decoder.bufferedBytes(), 64U);
}

TEST(FrameDecoder, ResetClearsBufferAndStatistics)
{
    FrameDecoder decoder;
    (void)decoder.feed(encodeFrame(makeFrame(1)));
    (void)decoder.feed(std::vector<std::uint8_t>{kSyncByte1, kSyncByte2, kProtocolVersion});

    decoder.reset();

    EXPECT_EQ(decoder.bufferedBytes(), 0U);
    EXPECT_EQ(decoder.statistics(), DecoderStatistics{});
}
