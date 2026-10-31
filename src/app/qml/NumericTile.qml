import QtQuick
import QtQuick.Controls

// Large numeric parameter display (HR, SpO2, ...). The frame takes the alarm
// colour while an alarm for this parameter is active.
Rectangle {
    id: root

    property string label
    property string unit
    property int value: -1 // -1 = invalid
    property color accent: Theme.text
    property int alarmPriority: 0
    property string statusText

    color: Theme.panel
    radius: Theme.radius
    border.width: alarmPriority > 0 ? 3 : 1
    border.color: Theme.alarmColor(alarmPriority)

    Label {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: Theme.spacing
        text: root.label
        color: root.accent
        font.pixelSize: 20
        font.bold: true
    }

    Label {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.spacing
        text: root.unit
        color: root.accent
        font.pixelSize: 16
    }

    Label {
        anchors.centerIn: parent
        text: root.value >= 0 ? root.value : "---"
        color: root.alarmPriority > 0 ? Theme.alarmColor(root.alarmPriority) : root.accent
        font.pixelSize: Math.min(root.height * 0.5, 110)
        font.bold: true
    }

    Label {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.margins: Theme.spacing
        visible: root.statusText.length > 0
        text: root.statusText
        color: Theme.alarmLow
        font.pixelSize: 16
        font.bold: true
    }
}
