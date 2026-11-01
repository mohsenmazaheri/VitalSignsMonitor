#pragma once

#include "vsm/core/Alarm.h"

#include <chrono>
#include <cstdint>
#include <span>
#include <vector>

namespace vsm::core {

/// Number of sound pulses in one repetition: high 10, medium 3, low 2.
[[nodiscard]] int alarmTonePulseCount(AlarmPriority priority) noexcept;

/// Time from the start of one burst to the start of the next.
[[nodiscard]] std::chrono::milliseconds alarmTonePeriod(AlarmPriority priority) noexcept;

/// One repetition of the alarm melody as mono 16-bit samples, silence included,
/// so that looping it gives the correct repeat rate. The pulse pattern follows
/// the idea of IEC 60601-1-8: the more urgent, the more and the faster the
/// pulses. It is a learning model and was not measured against the standard.
[[nodiscard]] std::vector<std::int16_t> makeAlarmTone(AlarmPriority priority, int sampleRateHz);

/// Wraps mono 16-bit samples in a RIFF/WAVE file image.
[[nodiscard]] std::vector<std::uint8_t> encodeWav(std::span<const std::int16_t> samples, int sampleRateHz);

} // namespace vsm::core
