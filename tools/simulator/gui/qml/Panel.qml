pragma Singleton
import QtQuick

// Colours, fonts and sizes of the simulated hardware front panel.
// Light-grey moulded case, black-bezel colour LCD, membrane keys and LEDs,
// as on bench-top patient simulators.
QtObject {
    // Desk and case
    readonly property color desk: "#1b1f24"
    readonly property color caseTop: "#e3e6ea"
    readonly property color caseBottom: "#c3c8ce"
    readonly property color caseEdge: "#8c939b"
    readonly property color groupFill: "#d3d7dc"
    readonly property color groupBorder: "#a7adb4"
    readonly property color label: "#333a42"
    readonly property color labelMuted: "#5d656e"
    readonly property color accent: "#0b6fb8" // brand blue

    // LCD
    readonly property color bezel: "#16191d"
    readonly property color lcd: "#06111a"
    readonly property color lcdGrid: "#12304a"
    readonly property color lcdText: "#d4e9ff"
    readonly property color lcdDim: "#6a8aa3"
    readonly property color ecg: "#3dff8f"
    readonly property color spo2: "#4fd8ff"
    readonly property color warning: "#ffb300"
    readonly property color fault: "#ff5252"

    // Keys
    readonly property color keyTop: "#fbfbfc"
    readonly property color keyBottom: "#d9dce0"
    readonly property color keyBorder: "#8f969e"
    readonly property color keyText: "#262b31"
    readonly property color keyShadow: "#7d848c"

    // LEDs
    readonly property color ledGreen: "#36ff6e"
    readonly property color ledAmber: "#ffb000"
    readonly property color ledRed: "#ff3434"
    readonly property color ledOff: "#4a4f55"

    readonly property string monoFont: "Consolas"

    // Design size of the front panel; the window scales it uniformly.
    readonly property int caseWidth: 1200
    readonly property int caseHeight: 770
}
