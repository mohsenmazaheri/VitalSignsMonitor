#include "vsm/core/AlarmManager.h"

#include <algorithm>
#include <tuple>

namespace vsm::core {

namespace {

[[nodiscard]] bool latches(const Alarm& alarm) noexcept
{
    return alarm.priority == AlarmPriority::High;
}

[[nodiscard]] int stateRank(AlarmState state) noexcept
{
    switch (state) {
    case AlarmState::Active:
        return 0;
    case AlarmState::Latched:
        return 1;
    case AlarmState::Acknowledged:
        return 2;
    }
    return 3;
}

} // namespace

std::string_view toString(AlarmState state) noexcept
{
    switch (state) {
    case AlarmState::Active:
        return "ACTIVE";
    case AlarmState::Acknowledged:
        return "ACKNOWLEDGED";
    case AlarmState::Latched:
        return "LATCHED";
    }
    return "UNKNOWN";
}

AlarmManager::AlarmManager(IAuditLog& audit, AlarmPolicy policy)
    : m_audit(audit)
    , m_policy(policy)
{
}

void AlarmManager::update(const std::vector<Alarm>& currentAlarms, Milliseconds now)
{
    tick(now);

    const auto isCurrent = [&currentAlarms](const Alarm& alarm) {
        return std::ranges::any_of(currentAlarms, [&alarm](const Alarm& current) { return current.code == alarm.code; });
    };

    // Conditions that are over.
    for (auto it = m_alarms.begin(); it != m_alarms.end();) {
        if (isCurrent(it->alarm) || it->state == AlarmState::Latched) {
            ++it;
        } else if (it->state == AlarmState::Active && latches(it->alarm)) {
            it->state = AlarmState::Latched;
            record(AuditEventType::AlarmLatched, it->alarm);
            ++it;
        } else {
            record(AuditEventType::AlarmEnded, it->alarm);
            it = m_alarms.erase(it);
        }
    }

    // New conditions and conditions that came back.
    bool newSeriousAlarm = false;
    for (const Alarm& alarm : currentAlarms) {
        const auto known = std::ranges::find_if(m_alarms, [&alarm](const ManagedAlarm& managed) {
            return managed.alarm.code == alarm.code;
        });
        if (known == m_alarms.end()) {
            m_alarms.push_back({alarm, AlarmState::Active});
            record(AuditEventType::AlarmRaised, alarm);
            newSeriousAlarm = newSeriousAlarm || alarm.priority >= AlarmPriority::Medium;
            continue;
        }
        if (known->state == AlarmState::Latched) {
            known->state = AlarmState::Active;
            record(AuditEventType::AlarmReactivated, alarm);
        }
        known->alarm = alarm;
    }

    if (newSeriousAlarm && audioPaused()) {
        m_pauseUntil.reset();
        record(AuditEventType::AudioPauseEnded, "New alarm ended the audio pause");
    }
    endPauseIfNothingLeft();
    sortAlarms();
}

void AlarmManager::tick(Milliseconds now)
{
    if (m_pauseUntil && now >= *m_pauseUntil) {
        m_pauseUntil.reset();
        record(AuditEventType::AudioPauseExpired, "Audio pause time is over");
    }
}

void AlarmManager::acknowledgeAll()
{
    for (auto it = m_alarms.begin(); it != m_alarms.end();) {
        if (it->state == AlarmState::Latched) {
            record(AuditEventType::AlarmReset, it->alarm);
            it = m_alarms.erase(it);
            continue;
        }
        if (it->state == AlarmState::Active) {
            it->state = AlarmState::Acknowledged;
            record(AuditEventType::AlarmAcknowledged, it->alarm);
        }
        ++it;
    }
    endPauseIfNothingLeft();
    sortAlarms();
}

void AlarmManager::pauseAudio(Milliseconds now)
{
    if (!hasUnacknowledged()) {
        return;
    }
    m_pauseUntil = now + m_policy.audioPauseDuration;
    record(AuditEventType::AudioPaused, "Audio paused by the operator");
}

void AlarmManager::resumeAudio()
{
    if (audioPaused()) {
        m_pauseUntil.reset();
        record(AuditEventType::AudioResumed, "Audio resumed by the operator");
    }
}

void AlarmManager::clear()
{
    if (!m_alarms.empty()) {
        record(AuditEventType::AlarmsCleared, "Session ended with " + std::to_string(m_alarms.size()) + " alarm(s)");
    }
    m_alarms.clear();
    m_pauseUntil.reset();
}

bool AlarmManager::hasUnacknowledged() const noexcept
{
    return std::ranges::any_of(m_alarms, [](const ManagedAlarm& managed) {
        return managed.state != AlarmState::Acknowledged;
    });
}

AlarmManager::Milliseconds AlarmManager::audioPauseRemaining(Milliseconds now) const noexcept
{
    return m_pauseUntil ? std::max(*m_pauseUntil - now, Milliseconds::zero()) : Milliseconds::zero();
}

std::optional<AlarmPriority> AlarmManager::soundingPriority() const noexcept
{
    if (audioPaused()) {
        return std::nullopt;
    }
    // The list is sorted by priority, so the first unacknowledged alarm decides.
    const auto sounding = std::ranges::find_if(m_alarms, [](const ManagedAlarm& managed) {
        return managed.state != AlarmState::Acknowledged;
    });
    if (sounding == m_alarms.end()) {
        return std::nullopt;
    }
    return sounding->alarm.priority;
}

void AlarmManager::record(AuditEventType type, const Alarm& alarm)
{
    m_audit.record({type, alarm.code, alarm.priority, alarm.message});
}

void AlarmManager::record(AuditEventType type, std::string detail)
{
    m_audit.record({type, {}, std::nullopt, std::move(detail)});
}

void AlarmManager::endPauseIfNothingLeft()
{
    if (audioPaused() && !hasUnacknowledged()) {
        m_pauseUntil.reset();
        record(AuditEventType::AudioPauseEnded, "Nothing left to silence");
    }
}

void AlarmManager::sortAlarms()
{
    const auto key = [](const ManagedAlarm& managed) {
        return std::tuple(static_cast<int>(managed.alarm.priority) * -1, stateRank(managed.state),
                          static_cast<int>(managed.alarm.source));
    };
    std::ranges::stable_sort(m_alarms, {}, key);
}

} // namespace vsm::core
