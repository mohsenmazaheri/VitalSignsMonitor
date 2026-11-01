import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Shows the most important alarm and holds the operator keys.
// An unacknowledged high-priority alarm flashes; acknowledged alarms are steady.
Rectangle {
    id: root

    required property MonitorViewModel monitor

    signal eventLogRequested()

    readonly property AlarmListModel alarms: monitor.alarms
    readonly property int priority: alarms.highestPriority
    readonly property bool active: priority > 0

    implicitHeight: 56
    radius: Theme.radius
    color: Theme.panel
    border.color: Theme.panelBorder

    // The colour lives on its own layer, so that only the colour flashes
    // and the keys stay readable and easy to hit.
    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        visible: root.active
        color: Theme.alarmColor(root.priority)

        SequentialAnimation on opacity {
            running: root.monitor.alarmFlashing
            loops: Animation.Infinite
            alwaysRunToEnd: true
            NumberAnimation { to: 0.35; duration: 250 }
            NumberAnimation { to: 1.0; duration: 250 }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 2 * Theme.spacing
        anchors.rightMargin: Theme.spacing
        spacing: Theme.spacing

        Label {
            Layout.fillWidth: true
            text: root.active ? root.alarms.topMessage : qsTr("No active alarms")
            color: root.active ? Theme.background : Theme.textMuted
            font.pixelSize: 20
            font.bold: root.active
            elide: Text.ElideRight
        }
        Label {
            // A latched alarm describes the past: say so instead of looking current.
            visible: root.alarms.topState === "LATCHED" || root.alarms.topState === "ACKNOWLEDGED"
            text: root.alarms.topState === "LATCHED" ? qsTr("ENDED - ACKNOWLEDGE") : qsTr("ACKNOWLEDGED")
            color: Theme.background
            font.pixelSize: 14
            font.bold: true
        }
        Label {
            visible: root.alarms.count > 1
            text: qsTr("+%1 more").arg(root.alarms.count - 1)
            color: Theme.background
            font.pixelSize: 16
        }
        BannerKey {
            objectName: "acknowledgeKey"
            text: qsTr("ACKNOWLEDGE")
            enabled: root.monitor.canAcknowledge
            onClicked: root.monitor.acknowledgeAlarms()
        }
        BannerKey {
            objectName: "audioPauseKey"
            Layout.preferredWidth: 190
            text: root.monitor.audioPaused ? qsTr("AUDIO PAUSED %1").arg(root.monitor.audioPauseRemaining)
                                           : qsTr("AUDIO PAUSE")
            enabled: root.monitor.audioPaused || root.monitor.canAcknowledge
            highlighted: root.monitor.audioPaused
            onClicked: root.monitor.toggleAudioPause()
        }
        BannerKey {
            objectName: "eventLogKey"
            text: qsTr("EVENTS")
            onClicked: root.eventLogRequested()
        }
    }

    component BannerKey: Button {
        Layout.preferredHeight: 40
        font.bold: true
        palette.button: Theme.panel
        palette.buttonText: enabled ? Theme.text : Theme.textMuted
        palette.highlight: Theme.alarmMedium
        palette.highlightedText: Theme.background
    }
}
