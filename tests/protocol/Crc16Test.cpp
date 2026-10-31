#include "vsm/protocol/Crc16.h"

#include <gtest/gtest.h>

#include <array>
#include <string_view>

using vsm::protocol::crc16CcittFalse;

namespace {

std::span<const std::uint8_t> bytesOf(std::string_view text)
{
    return {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()};
}

} // namespace

TEST(Crc16, MatchesStandardCheckValue)
{
    // Reference check value of CRC-16/CCITT-FALSE.
    EXPECT_EQ(crc16CcittFalse(bytesOf("123456789")), 0x29B1);
}

TEST(Crc16, EmptyInputReturnsInitialValue)
{
    EXPECT_EQ(crc16CcittFalse({}), 0xFFFF);
}

TEST(Crc16, DetectsSingleBitFlip)
{
    std::array<std::uint8_t, 4> data{0x01, 0x02, 0x03, 0x04};
    const auto original = crc16CcittFalse(data);
    data[2] ^= 0x01;
    EXPECT_NE(crc16CcittFalse(data), original);
}
