#include "vsm/core/AuditLog.h"

namespace vsm::core {

std::string_view toString(AuditEventType type) noexcept
{
    using enum AuditEventType;
    switch (type) {
    case ApplicationStarted:
        return "APPLICATION_STARTED";
    case ApplicationStopped:
        return "APPLICATION_STOPPED";
    case ConnectRequested:
        return "CONNECT_REQUESTED";
    case DisconnectRequested:
        return "DISCONNECT_REQUESTED";
    case Connected:
        return "CONNECTED";
    case ConnectionDropped:
        return "CONNECTION_DROPPED";
    case LinkLost:
        return "LINK_LOST";
    case LinkRestored:
        return "LINK_RESTORED";
    case AlarmRaised:
        return "ALARM_RAISED";
    case AlarmEnded:
        return "ALARM_ENDED";
    case AlarmLatched:
        return "ALARM_LATCHED";
    case AlarmReactivated:
        return "ALARM_REACTIVATED";
    case AlarmAcknowledged:
        return "ALARM_ACKNOWLEDGED";
    case AlarmReset:
        return "ALARM_RESET";
    case AlarmsCleared:
        return "ALARMS_CLEARED";
    case AudioPaused:
        return "AUDIO_PAUSED";
    case AudioResumed:
        return "AUDIO_RESUMED";
    case AudioPauseEnded:
        return "AUDIO_PAUSE_ENDED";
    case AudioPauseExpired:
        return "AUDIO_PAUSE_EXPIRED";
    }
    return "UNKNOWN";
}

bool isOperatorAction(AuditEventType type) noexcept
{
    using enum AuditEventType;
    switch (type) {
    case ConnectRequested:
    case DisconnectRequested:
    case AlarmAcknowledged:
    case AlarmReset:
    case AudioPaused:
    case AudioResumed:
        return true;
    default:
        return false;
    }
}

} // namespace vsm::core
