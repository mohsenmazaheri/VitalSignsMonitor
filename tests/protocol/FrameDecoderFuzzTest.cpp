#include "vsm/protocol/FrameDecoder.h"
#include "vsm/protocol/Messages.h"

#include <gtest/gtest.h>

#include <random>

using namespace vsm::protocol;

// Randomised robustness tests: fixed seeds, so a failure can be reproduced.
// They complement the example-based tests in FrameDecoderTest.cpp.

namespace {

std::vector<std::uint8_t> randomPayload(std::mt19937& random)
{
    std::uniform_int_distribution<std::size_t> length(0, 64);
    std::uniform_int_distribution<int> byte(0, 255);
    std::vector<std::uint8_t> payload(length(random));
    for (std::uint8_t& value : payload) {
        value = static_cast<std::uint8_t>(byte(random));
    }
    return payload;
}

/// Feeds the stream in random chunk sizes, as a socket would deliver it.
std::vector<Frame> feedInChunks(FrameDecoder& decoder, const std::vector<std::uint8_t>& stream, std::mt19937& random)
{
    std::vector<Frame> frames;
    std::uniform_int_distribution<std::size_t> chunk(1, 97);
    for (std::size_t position = 0; position < stream.size();) {
        const std::size_t size = std::min(chunk(random), stream.size() - position);
        auto decoded = decoder.feed(std::span(stream).subspan(position, size));
        frames.insert(frames.end(), std::make_move_iterator(decoded.begin()), std::make_move_iterator(decoded.end()));
        position += size;
    }
    return frames;
}

} // namespace

TEST(FrameDecoderFuzz, RandomBytesNeverCrashAndStayBounded)
{
    for (unsigned seed = 1; seed <= 20; ++seed) {
        std::mt19937 random(seed);
        std::uniform_int_distribution<int> byte(0, 255);
        std::vector<std::uint8_t> stream(20000);
        for (std::uint8_t& value : stream) {
            value = static_cast<std::uint8_t>(byte(random));
        }

        FrameDecoder decoder;
        (void)feedInChunks(decoder, stream, random);

        // Whatever the input was, the decoder must not hoard memory.
        EXPECT_LE(decoder.bufferedBytes(), 2048U) << "seed " << seed;
    }
}

TEST(FrameDecoderFuzz, ChunkingDoesNotChangeTheResult)
{
    std::mt19937 random(42);
    std::vector<std::uint8_t> stream;
    std::vector<Frame> sent;
    for (std::uint16_t sequence = 0; sequence < 200; ++sequence) {
        sent.push_back({MessageType::EcgWaveform, sequence, randomPayload(random)});
        const auto bytes = encodeFrame(sent.back());
        stream.insert(stream.end(), bytes.begin(), bytes.end());
    }

    FrameDecoder decoder;
    const auto received = feedInChunks(decoder, stream, random);

    ASSERT_EQ(received.size(), sent.size());
    for (std::size_t i = 0; i < sent.size(); ++i) {
        EXPECT_EQ(received[i].sequence, sent[i].sequence);
        EXPECT_EQ(received[i].payload, sent[i].payload);
    }
    EXPECT_EQ(decoder.statistics().crcErrors, 0U);
    EXPECT_EQ(decoder.statistics().bytesDiscarded, 0U);
}

TEST(FrameDecoderFuzz, CorruptedStreamNeverDeliversAWrongFrame)
{
    for (unsigned seed = 1; seed <= 20; ++seed) {
        std::mt19937 random(seed);
        std::vector<std::uint8_t> stream;
        std::vector<Frame> sent;
        for (std::uint16_t sequence = 0; sequence < 300; ++sequence) {
            sent.push_back({MessageType::Numerics, sequence, randomPayload(random)});
            const auto bytes = encodeFrame(sent.back());
            stream.insert(stream.end(), bytes.begin(), bytes.end());
        }

        // Flip about 1 byte in 200.
        std::uniform_int_distribution<std::size_t> position(0, stream.size() - 1);
        for (std::size_t flip = 0; flip < stream.size() / 200; ++flip) {
            stream[position(random)] ^= 0x5A;
        }

        FrameDecoder decoder;
        const auto received = feedInChunks(decoder, stream, random);

        // Most frames survive, and every delivered frame is exactly what was sent.
        EXPECT_GT(received.size(), sent.size() / 2) << "seed " << seed;
        EXPECT_GT(decoder.statistics().bytesDiscarded, 0U) << "seed " << seed;
        for (const Frame& frame : received) {
            ASSERT_LT(frame.sequence, sent.size()) << "seed " << seed;
            EXPECT_EQ(frame.payload, sent[frame.sequence].payload) << "seed " << seed;
        }
    }
}

TEST(FrameDecoderFuzz, RandomFramesNeverCrashTheMessageParser)
{
    std::mt19937 random(7);
    const std::array types{MessageType::Heartbeat, MessageType::Numerics, MessageType::EcgWaveform};
    std::uniform_int_distribution<std::size_t> type(0, types.size() - 1);

    for (int i = 0; i < 5000; ++i) {
        const Frame frame{types[type(random)], 0, randomPayload(random)};
        (void)parseMessage(frame); // accepted or rejected, but never undefined behaviour
    }
    SUCCEED();
}
