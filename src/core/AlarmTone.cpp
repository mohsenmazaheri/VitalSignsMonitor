#include "vsm/core/AlarmTone.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <string_view>

namespace vsm::core {

namespace {

using std::chrono::milliseconds;

constexpr double kFundamentalHz = 523.25; // C5
constexpr double kPeakAmplitude = 0.6 * 32767.0;
constexpr double kRampSeconds = 0.02; // soft edges avoid clicks

// Harmonics make the tone easy to locate and to hear over background noise.
constexpr std::array kHarmonicWeights{1.0, 0.6, 0.4, 0.3};

struct TonePattern {
    int pulsesPerGroup; // short pause after each group
    int groups;
    int pulses;
    milliseconds pulseLength;
    milliseconds pulseGap;
    milliseconds groupGap;
    milliseconds period;
};

[[nodiscard]] TonePattern patternFor(AlarmPriority priority) noexcept
{
    using namespace std::chrono_literals;
    switch (priority) {
    case AlarmPriority::High:
        return {5, 2, 10, 150ms, 100ms, 350ms, 6s};
    case AlarmPriority::Medium:
        return {3, 1, 3, 200ms, 150ms, 0ms, 8s};
    case AlarmPriority::Low:
        break;
    }
    return {2, 1, 2, 250ms, 200ms, 0ms, 20s};
}

[[nodiscard]] std::size_t toSamples(milliseconds time, int sampleRateHz) noexcept
{
    return static_cast<std::size_t>(time.count()) * static_cast<std::size_t>(sampleRateHz) / 1000U;
}

void writePulse(std::span<std::int16_t> target, int sampleRateHz)
{
    const double weightSum = [] {
        double sum = 0.0;
        for (const double weight : kHarmonicWeights) {
            sum += weight;
        }
        return sum;
    }();
    const double rate = static_cast<double>(sampleRateHz);
    const double length = static_cast<double>(target.size());

    for (std::size_t i = 0; i < target.size(); ++i) {
        const double position = static_cast<double>(i);
        const double time = position / rate;

        double value = 0.0;
        for (std::size_t harmonic = 0; harmonic < kHarmonicWeights.size(); ++harmonic) {
            const double frequency = kFundamentalHz * static_cast<double>(harmonic + 1);
            value += kHarmonicWeights[harmonic] * std::sin(2.0 * std::numbers::pi * frequency * time);
        }

        const double envelope = std::min({1.0, position / (kRampSeconds * rate), (length - position) / (kRampSeconds * rate)});
        target[i] = static_cast<std::int16_t>(std::lround(kPeakAmplitude * envelope * value / weightSum));
    }
}

template <typename Value>
void appendLittleEndian(std::vector<std::uint8_t>& out, Value value)
{
    for (std::size_t byte = 0; byte < sizeof(Value); ++byte) {
        out.push_back(static_cast<std::uint8_t>((value >> (8U * byte)) & 0xFFU));
    }
}

void appendText(std::vector<std::uint8_t>& out, std::string_view text)
{
    for (const char c : text) {
        out.push_back(static_cast<std::uint8_t>(c));
    }
}

} // namespace

int alarmTonePulseCount(AlarmPriority priority) noexcept
{
    return patternFor(priority).pulses;
}

milliseconds alarmTonePeriod(AlarmPriority priority) noexcept
{
    return patternFor(priority).period;
}

std::vector<std::int16_t> makeAlarmTone(AlarmPriority priority, int sampleRateHz)
{
    if (sampleRateHz <= 0) {
        throw std::invalid_argument("makeAlarmTone: sample rate must be > 0");
    }
    const TonePattern pattern = patternFor(priority);
    std::vector<std::int16_t> samples(toSamples(pattern.period, sampleRateHz), 0);
    std::span<std::int16_t> all(samples);

    milliseconds start{0};
    for (int pulse = 0; pulse < pattern.pulses; ++pulse) {
        writePulse(all.subspan(toSamples(start, sampleRateHz), toSamples(pattern.pulseLength, sampleRateHz)), sampleRateHz);
        start += pattern.pulseLength + pattern.pulseGap;
        if ((pulse + 1) % pattern.pulsesPerGroup == 0) {
            start += pattern.groupGap;
        }
    }
    return samples;
}

std::vector<std::uint8_t> encodeWav(std::span<const std::int16_t> samples, int sampleRateHz)
{
    constexpr std::uint16_t kPcmFormat = 1;
    constexpr std::uint16_t kChannels = 1;
    constexpr std::uint16_t kBitsPerSample = 16;
    constexpr std::uint16_t kBlockAlign = kChannels * kBitsPerSample / 8;
    constexpr std::uint32_t kFmtChunkSize = 16;
    constexpr std::uint32_t kHeaderBytesAfterRiffSize = 36;

    const auto rate = static_cast<std::uint32_t>(sampleRateHz);
    const auto dataBytes = static_cast<std::uint32_t>(samples.size() * kBlockAlign);

    std::vector<std::uint8_t> out;
    out.reserve(44U + dataBytes);
    appendText(out, "RIFF");
    appendLittleEndian<std::uint32_t>(out, kHeaderBytesAfterRiffSize + dataBytes);
    appendText(out, "WAVEfmt ");
    appendLittleEndian<std::uint32_t>(out, kFmtChunkSize);
    appendLittleEndian<std::uint16_t>(out, kPcmFormat);
    appendLittleEndian<std::uint16_t>(out, kChannels);
    appendLittleEndian<std::uint32_t>(out, rate);
    appendLittleEndian<std::uint32_t>(out, rate * kBlockAlign);
    appendLittleEndian<std::uint16_t>(out, kBlockAlign);
    appendLittleEndian<std::uint16_t>(out, kBitsPerSample);
    appendText(out, "data");
    appendLittleEndian<std::uint32_t>(out, dataBytes);
    for (const std::int16_t sample : samples) {
        appendLittleEndian<std::uint16_t>(out, static_cast<std::uint16_t>(sample));
    }
    return out;
}

} // namespace vsm::core
