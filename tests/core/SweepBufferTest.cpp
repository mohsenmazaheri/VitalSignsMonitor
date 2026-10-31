#include "vsm/core/SweepBuffer.h"

#include <gtest/gtest.h>

#include <array>

using vsm::core::SweepBuffer;

TEST(SweepBuffer, StartsEmpty)
{
    const SweepBuffer<float> buffer(4);
    EXPECT_EQ(buffer.capacity(), 4U);
    EXPECT_EQ(buffer.filledCount(), 0U);
    EXPECT_EQ(buffer.writeIndex(), 0U);
    EXPECT_FALSE(buffer.isFull());
}

TEST(SweepBuffer, WritesLeftToRightAndWraps)
{
    SweepBuffer<int> buffer(3);
    const std::array values{1, 2, 3, 4};
    buffer.push(std::span<const int>(values));

    // Screen order: the 4th sample overwrote column 0.
    const auto samples = buffer.samples();
    EXPECT_EQ(samples[0], 4);
    EXPECT_EQ(samples[1], 2);
    EXPECT_EQ(samples[2], 3);
    EXPECT_EQ(buffer.writeIndex(), 1U);
    EXPECT_TRUE(buffer.isFull());
}

TEST(SweepBuffer, ClearResetsPositionAndData)
{
    SweepBuffer<int> buffer(2);
    buffer.push(7);
    buffer.clear();

    EXPECT_EQ(buffer.writeIndex(), 0U);
    EXPECT_EQ(buffer.filledCount(), 0U);
    EXPECT_EQ(buffer.samples()[0], 0);
}

TEST(SweepBuffer, RejectsZeroCapacity)
{
    EXPECT_THROW(SweepBuffer<float>(0), std::invalid_argument);
}
