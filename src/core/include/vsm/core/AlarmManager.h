#pragma once

#include "vsm/core/Alarm.h"
#include "vsm/core/AuditLog.h"

#include <chrono>
#include <optional>
#include <vector>

namespace vsm::core {

/// Life-cycle state of an alarm known to the AlarmManager.
enum class AlarmState {
    Active,       ///< Condition present, not acknowledged: visible and audible.
    Acknowledged, ///< Condition present, acknowledged: visible, silent.
    Latched,      ///< Condition over, not acknowledged yet: visible and audible.
};

[[nodiscard]] std::string_view toString(AlarmState state) noexcept;

struct ManagedAlarm {
    Alarm alarm;
    AlarmState state{AlarmState::Active};

    friend bool operator==(const ManagedAlarm&, const ManagedAlarm&) = default;
};

struct AlarmPolicy {
    /// How long one press of the audio-pause key silences the alarms.
    std::chrono::milliseconds audioPauseDuration{std::chrono::minutes(2)};
};

/// Adds memory to the stateless AlarmEvaluator: latching, acknowledgement and
/// audio pause, in the style of IEC 60601-1-8 (a learning model, not certified).
///
/// Rules:
///  - High-priority alarms latch: when the condition ends they stay visible and
///    audible until acknowledged, so a short critical event cannot go unnoticed.
///  - Medium and low alarms end with their condition.
///  - Acknowledging silences an alarm while its condition lasts and removes
///    latched alarms. A later occurrence is a new alarm and sounds again.
///  - The audio pause silences everything for a limited time. A new alarm of
///    medium or high priority ends the pause early.
///
/// The class is free of Qt and of clocks: every time-dependent call receives a
/// monotonic "now", so tests can run two minutes of alarm time in microseconds.
class AlarmManager {
public:
    using Milliseconds = std::chrono::milliseconds;

    explicit AlarmManager(IAuditLog& audit, AlarmPolicy policy = {});

    /// Applies the latest result of AlarmEvaluator::evaluate().
    void update(const std::vector<Alarm>& currentAlarms, Milliseconds now);

    /// Ends an expired audio pause. Call it regularly.
    void tick(Milliseconds now);

    /// Operator: acknowledge everything that is not acknowledged yet.
    void acknowledgeAll();

    /// Operator: silence for AlarmPolicy::audioPauseDuration. Does nothing when
    /// nothing is unacknowledged: a pause must not hide an alarm that comes later.
    void pauseAudio(Milliseconds now);

    /// Operator: end the audio pause early.
    void resumeAudio();

    /// Forgets all alarms when the session ends, including latched ones.
    void clear();

    /// All known alarms: highest priority first, then Active, Latched, Acknowledged.
    [[nodiscard]] const std::vector<ManagedAlarm>& alarms() const noexcept { return m_alarms; }

    [[nodiscard]] bool hasUnacknowledged() const noexcept;
    [[nodiscard]] bool audioPaused() const noexcept { return m_pauseUntil.has_value(); }
    [[nodiscard]] Milliseconds audioPauseRemaining(Milliseconds now) const noexcept;

    /// The priority the sound must signal now; empty means silence.
    [[nodiscard]] std::optional<AlarmPriority> soundingPriority() const noexcept;

private:
    void record(AuditEventType type, const Alarm& alarm);
    void record(AuditEventType type, std::string detail);
    void endPauseIfNothingLeft();
    void sortAlarms();

    IAuditLog& m_audit;
    AlarmPolicy m_policy;
    std::vector<ManagedAlarm> m_alarms;
    std::optional<Milliseconds> m_pauseUntil;
};

} // namespace vsm::core
