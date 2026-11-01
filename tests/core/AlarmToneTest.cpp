#include "vsm/core/AlarmTone.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <string>

using namespace vsm::core;

namespace {

constexpr int kRate = 8000;

/// Counts groups of sound separated by at least 40 ms of silence.
int countPulses(const std::vector<std::int16_t>& samples)
{
    constexpr std::size_t kMinSilence = kRate * 40 / 1000;
    int pulses = 0;
    std::size_t silence = kMinSilence;
    for (const std::int16_t sample : samples) {
        if (sample == 0) {
            ++silence;
        } else {
            if (silence >= kMinSilence) {
                ++pulses;
            }
            silence = 0;
        }
    }
    return pulses;
}

std::uint32_t readU32(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::uint32_t>(bytes[offset]) | static_cast<std::uint32_t>(bytes[offset + 1]) << 8U
           | static_cast<std::uint32_t>(bytes[offset + 2]) << 16U | static_cast<std::uint32_t>(bytes[offset + 3]) << 24U;
}

class AlarmToneByPriority : public ::testing::TestWithParam<AlarmPriority> {};

} // namespace

TEST_P(AlarmToneByPriority, HasTheDocumentedNumberOfPulses)
{
    EXPECT_EQ(countPulses(makeAlarmTone(GetParam(), kRate)), alarmTonePulseCount(GetParam()));
}

TEST_P(AlarmToneByPriority, LengthEqualsTheRepeatPeriod)
{
    const auto samples = makeAlarmTone(GetParam(), kRate);
    const auto expected = static_cast<std::size_t>(alarmTonePeriod(GetParam()).count()) * kRate / 1000U;
    EXPECT_EQ(samples.size(), expected);
}

TEST_P(AlarmToneByPriority, EndsWithSilenceSoThatItCanLoop)
{
    const auto samples = makeAlarmTone(GetParam(), kRate);
    ASSERT_GT(samples.size(), 100U);
    EXPECT_EQ(samples.back(), 0);
    EXPECT_EQ(samples[samples.size() - 100], 0);
}

TEST_P(AlarmToneByPriority, IsLoudButDoesNotClip)
{
    const auto samples = makeAlarmTone(GetParam(), kRate);
    const auto [low, high] = std::ranges::minmax(samples);
    EXPECT_GT(high, 5000);
    EXPECT_LT(high, 32767);
    EXPECT_GT(low, -32768);
}

INSTANTIATE_TEST_SUITE_P(AllPriorities, AlarmToneByPriority,
                         ::testing::Values(AlarmPriority::Low, AlarmPriority::Medium, AlarmPriority::High));

TEST(AlarmTone, MoreUrgentMeansMorePulses)
{
    EXPECT_GT(alarmTonePulseCount(AlarmPriority::High), alarmTonePulseCount(AlarmPriority::Medium));
    EXPECT_GT(alarmTonePulseCount(AlarmPriority::Medium), alarmTonePulseCount(AlarmPriority::Low));
    EXPECT_LT(alarmTonePeriod(AlarmPriority::High), alarmTonePeriod(AlarmPriority::Low));
}

TEST(AlarmTone, RejectsInvalidSampleRate)
{
    EXPECT_THROW((void)makeAlarmTone(AlarmPriority::Low, 0), std::invalid_argument);
}

TEST(Wav, HeaderDescribesMono16BitPcm)
{
    const std::vector<std::int16_t> samples{1, -1, 300};
    const auto wav = encodeWav(samples, 22050);

    ASSERT_EQ(wav.size(), 44U + 6U);
    EXPECT_EQ(std::string(wav.begin(), wav.begin() + 4), "RIFF");
    EXPECT_EQ(std::string(wav.begin() + 8, wav.begin() + 16), "WAVEfmt ");
    EXPECT_EQ(readU32(wav, 4), wav.size() - 8U);
    EXPECT_EQ(readU32(wav, 24), 22050U);
    EXPECT_EQ(readU32(wav, 28), 44100U); // bytes per second
    EXPECT_EQ(std::string(wav.begin() + 36, wav.begin() + 40), "data");
    EXPECT_EQ(readU32(wav, 40), 6U);
}

TEST(Wav, SamplesAreLittleEndian)
{
    const std::vector<std::int16_t> samples{0x0102, -2};
    const auto wav = encodeWav(samples, 8000);

    EXPECT_EQ(wav[44], 0x02);
    EXPECT_EQ(wav[45], 0x01);
    EXPECT_EQ(wav[46], 0xFE);
    EXPECT_EQ(wav[47], 0xFF);
}
