#pragma once

#include "vsm/core/Alarm.h"

#include <optional>
#include <string>
#include <string_view>

namespace vsm::core {

/// Everything that is recorded in the audit trail. The names returned by
/// toString() are stable, because audit files are read later by other tools.
enum class AuditEventType {
    ApplicationStarted,
    ApplicationStopped,
    ConnectRequested,
    DisconnectRequested,
    Connected,
    ConnectionDropped,
    LinkLost,
    LinkRestored,
    AlarmRaised,
    AlarmEnded,
    AlarmLatched,
    AlarmReactivated,
    AlarmAcknowledged,
    AlarmReset,
    AlarmsCleared,
    AudioPaused,
    AudioResumed,      ///< The operator ended the pause.
    AudioPauseEnded,   ///< The system ended the pause early (new alarm, nothing left to silence).
    AudioPauseExpired,
};

/// One entry of the audit trail. It carries no time: the log stamps each entry
/// when it is written, so all sources share one clock. Synthetic data only -
/// an entry never holds patient identity.
struct AuditEvent {
    AuditEventType type{AuditEventType::ApplicationStarted};
    std::string code;                      ///< Alarm code, empty for other events.
    std::optional<AlarmPriority> priority; ///< Alarm priority, empty for other events.
    std::string detail;                    ///< Human-readable text, e.g. the alarm message.

    friend bool operator==(const AuditEvent&, const AuditEvent&) = default;
};

[[nodiscard]] std::string_view toString(AuditEventType type) noexcept;

/// True if a person caused the event (a key press), false if the system did.
[[nodiscard]] bool isOperatorAction(AuditEventType type) noexcept;

/// Destination of audit events. core only knows this interface; the file
/// writer and the on-screen list are separate classes (dependency inversion).
class IAuditLog {
public:
    virtual ~IAuditLog() = default;

    /// Must not throw: a failing log must never take the alarm logic down.
    virtual void record(const AuditEvent& event) = 0;

protected:
    IAuditLog() = default;
    IAuditLog(const IAuditLog&) = default;
    IAuditLog& operator=(const IAuditLog&) = default;
    IAuditLog(IAuditLog&&) = default;
    IAuditLog& operator=(IAuditLog&&) = default;
};

} // namespace vsm::core
