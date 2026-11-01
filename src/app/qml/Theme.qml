pragma Singleton
import QtQuick

// Central colours and sizes. Monitor conventions: ECG/HR green, SpO2 cyan,
// alarm colours by priority (high = red, medium = yellow, low = cyan).
QtObject {
    readonly property color background: "#05080c"
    readonly property color panel: "#0d141c"
    readonly property color panelBorder: "#1e2a36"
    readonly property color text: "#e6edf3"
    readonly property color textMuted: "#7d8a96"

    readonly property color ecg: "#00e676"
    readonly property color spo2: "#00e5ff"

    readonly property color alarmHigh: "#ff1744"
    readonly property color alarmMedium: "#ffd600"
    readonly property color alarmLow: "#40c4ff"

    readonly property int spacing: 8
    readonly property int radius: 6

    function alarmColor(priority) {
        switch (priority) {
        case 3: return alarmHigh
        case 2: return alarmMedium
        case 1: return alarmLow
        default: return panelBorder
        }
    }
}
