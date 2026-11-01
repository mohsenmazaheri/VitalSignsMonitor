#pragma once

#include "vsm/core/Alarm.h"

#include <optional>

namespace vsm::app {

/// Plays the alarm melody of one priority until told otherwise. The view model
/// only knows this interface, so the monitor also runs without an audio device.
class IAlarmSound {
public:
    virtual ~IAlarmSound() = default;

    /// Empty means silence. Calling it again with the same value must not
    /// restart the melody.
    virtual void setPriority(std::optional<core::AlarmPriority> priority) = 0;

protected:
    IAlarmSound() = default;
    IAlarmSound(const IAlarmSound&) = default;
    IAlarmSound& operator=(const IAlarmSound&) = default;
    IAlarmSound(IAlarmSound&&) = default;
    IAlarmSound& operator=(IAlarmSound&&) = default;
};

/// Used with --no-sound.
class NullAlarmSound final : public IAlarmSound {
public:
    void setPriority(std::optional<core::AlarmPriority> /*priority*/) override {}
};

} // namespace vsm::app
