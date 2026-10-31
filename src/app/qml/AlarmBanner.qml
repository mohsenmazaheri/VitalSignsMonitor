import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Shows the most severe active alarm; high-priority alarms flash.
Rectangle {
    id: root

    required property AlarmListModel alarms

    readonly property int priority: alarms.highestPriority
    readonly property bool active: priority > 0

    implicitHeight: 48
    radius: Theme.radius
    color: active ? Theme.alarmColor(priority) : Theme.panel
    border.color: Theme.panelBorder

    // Flash high-priority alarms (~2 Hz), steady for medium/low.
    SequentialAnimation on opacity {
        running: root.priority === 3
        loops: Animation.Infinite
        alwaysRunToEnd: true
        NumberAnimation { to: 0.35; duration: 250 }
        NumberAnimation { to: 1.0; duration: 250 }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 2 * Theme.spacing
        anchors.rightMargin: 2 * Theme.spacing

        Label {
            Layout.fillWidth: true
            text: root.active ? root.alarms.topMessage : qsTr("No active alarms")
            color: root.active ? Theme.background : Theme.textMuted
            font.pixelSize: 20
            font.bold: root.active
            elide: Text.ElideRight
        }
        Label {
            visible: root.alarms.count > 1
            text: qsTr("+%1 more").arg(root.alarms.count - 1)
            color: Theme.background
            font.pixelSize: 16
        }
    }
}
