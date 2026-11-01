import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Connection settings (TCP or serial), connect/disconnect and status.
Rectangle {
    id: root

    required property MonitorViewModel monitor

    implicitHeight: layout.implicitHeight + 2 * Theme.spacing
    color: Theme.panel
    border.color: Theme.panelBorder
    radius: Theme.radius

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: Theme.spacing
        spacing: Theme.spacing

        ComboBox {
            id: transportBox
            model: [qsTr("TCP"), qsTr("Serial")]
            enabled: !root.monitor.sessionActive
        }

        // --- TCP fields ---
        TextField {
            id: hostField
            visible: transportBox.currentIndex === 0
            enabled: !root.monitor.sessionActive
            text: "127.0.0.1"
            placeholderText: qsTr("Host")
            Layout.preferredWidth: 140
        }
        SpinBox {
            id: portBox
            visible: transportBox.currentIndex === 0
            enabled: !root.monitor.sessionActive
            from: 1
            to: 65535
            value: 5555
            editable: true
            textFromValue: (value) => value.toString() // no thousands separator
        }

        // --- Serial fields ---
        ComboBox {
            id: serialPortBox
            visible: transportBox.currentIndex === 1
            enabled: !root.monitor.sessionActive
            Layout.preferredWidth: 140
            model: root.monitor.availableSerialPorts()
        }
        ToolButton {
            visible: transportBox.currentIndex === 1
            enabled: !root.monitor.sessionActive
            text: "↻"
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Refresh serial ports")
            onClicked: serialPortBox.model = root.monitor.availableSerialPorts()
        }
        ComboBox {
            id: baudBox
            visible: transportBox.currentIndex === 1
            enabled: !root.monitor.sessionActive
            model: [9600, 57600, 115200, 230400]
            currentIndex: 2
        }

        Button {
            text: root.monitor.sessionActive ? qsTr("Disconnect") : qsTr("Connect")
            highlighted: !root.monitor.sessionActive
            onClicked: {
                if (root.monitor.sessionActive)
                    root.monitor.disconnectDevice()
                else if (transportBox.currentIndex === 0)
                    root.monitor.connectTcp(hostField.text, portBox.value)
                else
                    root.monitor.connectSerial(serialPortBox.currentText, Number(baudBox.currentText))
            }
        }

        // Status LED + text
        Rectangle {
            width: 12
            height: 12
            radius: 6
            color: !root.monitor.sessionActive ? Theme.textMuted
                 : root.monitor.connected ? Theme.ecg : Theme.alarmMedium
        }
        Label {
            text: root.monitor.connectionStatus
            color: Theme.text
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        Label {
            visible: text.length > 0
            text: root.monitor.lastError
            color: Theme.alarmHigh
            elide: Text.ElideRight
            Layout.maximumWidth: 360
        }
    }
}
