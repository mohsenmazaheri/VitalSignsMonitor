#include "vsm/protocol/Crc16.h"
#include "vsm/protocol/Frame.h"

#include <gtest/gtest.h>

#include <stdexcept>

using namespace vsm::protocol;

TEST(FrameCodec, EncodesHeaderPayloadAndCrcLittleEndian)
{
    const Frame frame{MessageType::Numerics, 0x1234, {0xDE, 0xAD}};
    const auto bytes = encodeFrame(frame);

    ASSERT_EQ(bytes.size(), kHeaderSize + 2 + kCrcSize);
    EXPECT_EQ(bytes[0], kSyncByte1);
    EXPECT_EQ(bytes[1], kSyncByte2);
    EXPECT_EQ(bytes[2], kProtocolVersion);
    EXPECT_EQ(bytes[3], static_cast<std::uint8_t>(MessageType::Numerics));
    EXPECT_EQ(bytes[4], 0x34); // sequence, low byte first
    EXPECT_EQ(bytes[5], 0x12);
    EXPECT_EQ(bytes[6], 0x02); // payload length
    EXPECT_EQ(bytes[7], 0x00);
    EXPECT_EQ(bytes[8], 0xDE);
    EXPECT_EQ(bytes[9], 0xAD);

    const auto crc = crc16CcittFalse(std::span(bytes).subspan(2, kHeaderSize - 2 + 2));
    EXPECT_EQ(bytes[10], static_cast<std::uint8_t>(crc & 0xFF));
    EXPECT_EQ(bytes[11], static_cast<std::uint8_t>(crc >> 8));
}

TEST(FrameCodec, AcceptsEmptyPayload)
{
    const auto bytes = encodeFrame(Frame{MessageType::Heartbeat, 0, {}});
    EXPECT_EQ(bytes.size(), kHeaderSize + kCrcSize);
}

TEST(FrameCodec, RejectsOversizedPayload)
{
    Frame frame;
    frame.payload.resize(kMaxPayloadSize + 1);
    EXPECT_THROW((void)encodeFrame(frame), std::length_error);
}
