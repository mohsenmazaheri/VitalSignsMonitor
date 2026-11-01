import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// All current alarms and the latest audit events. The monitor keeps running
// behind it, and the alarm banner stays visible above it.
Popup {
    id: root

    required property MonitorViewModel monitor

    modal: false
    closePolicy: Popup.CloseOnEscape
    padding: 2 * Theme.spacing

    background: Rectangle {
        color: Theme.panel
        border.color: Theme.textMuted
        radius: Theme.radius
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacing

        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: qsTr("Current alarms (%1)").arg(root.monitor.alarms.count)
                color: Theme.text
                font.pixelSize: 16
                font.bold: true
            }
            Button {
                objectName: "closeEventLogKey"
                text: qsTr("Close")
                onClicked: root.close()
            }
        }

        ListView {
            objectName: "alarmList"
            Layout.fillWidth: true
            Layout.preferredHeight: 112
            clip: true
            model: root.monitor.alarms
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}

            delegate: RowLayout {
                required property string message
                required property string priorityName
                required property string alarmState
                required property int priority

                width: ListView.view.width
                height: 28
                spacing: Theme.spacing

                Rectangle {
                    Layout.preferredWidth: 12
                    Layout.preferredHeight: 12
                    radius: 6
                    color: Theme.alarmColor(parent.priority)
                }
                Label {
                    Layout.fillWidth: true
                    text: parent.message
                    color: Theme.text
                    elide: Text.ElideRight
                }
                Label {
                    Layout.preferredWidth: 80
                    text: parent.priorityName
                    color: Theme.textMuted
                }
                Label {
                    Layout.preferredWidth: 120
                    text: parent.alarmState
                    color: Theme.textMuted
                }
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacing
            text: qsTr("Event log (newest first)")
            color: Theme.text
            font.pixelSize: 16
            font.bold: true
        }

        ListView {
            objectName: "eventList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.monitor.auditLog
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}

            delegate: RowLayout {
                required property string time
                required property string event
                required property string detail
                required property bool operatorAction

                width: ListView.view.width
                height: 24
                spacing: Theme.spacing

                Label {
                    Layout.preferredWidth: 70
                    text: parent.time
                    color: Theme.textMuted
                    font.family: "Consolas"
                }
                Label {
                    Layout.preferredWidth: 210
                    text: parent.event
                    color: parent.operatorAction ? Theme.spo2 : Theme.text
                    font.family: "Consolas"
                    elide: Text.ElideRight
                }
                Label {
                    Layout.fillWidth: true
                    text: parent.detail
                    color: Theme.text
                    elide: Text.ElideRight
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Complete log: %1").arg(root.monitor.auditLog.location)
            color: Theme.textMuted
            font.pixelSize: 12
            elide: Text.ElideMiddle
        }
    }
}
