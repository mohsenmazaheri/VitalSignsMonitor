import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VitalSigns.PatientSimulator

// Output interface selection: serial port (with baud rate) or TCP server.
GroupFrame {
    id: root

    required property PatientSimulatorController device

    // Interface chosen on the panel; becomes active with the OPEN key.
    property bool serialSelected: device.serialOutput

    title: "OUTPUT"

    function refreshPorts() {
        const ports = root.device.availableSerialPorts()
        portBox.model = ports
        if (ports.length > 0 && portBox.currentIndex < 0)
            portBox.currentIndex = 0
    }

    function openSelected() {
        if (root.serialSelected)
            root.device.openSerial(portBox.currentText, Number(baudBox.currentText))
        else
            root.device.openTcp(tcpPort.value)
    }

    Component.onCompleted: {
        refreshPorts()
        const baudIndex = baudBox.find(String(root.device.baudRate))
        if (baudIndex >= 0)
            baudBox.currentIndex = baudIndex
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            spacing: 8
            PanelKey {
                text: "SERIAL"
                checkable: true
                autoExclusive: true
                checked: root.serialSelected
                implicitWidth: 100
                onClicked: { root.serialSelected = true; root.refreshPorts() }
            }
            PanelKey {
                text: "TCP"
                checkable: true
                autoExclusive: true
                checked: !root.serialSelected
                implicitWidth: 100
                onClicked: root.serialSelected = false
            }
            Item { Layout.fillWidth: true }
            PanelKey {
                text: root.device.outputOpen ? "CLOSE" : "OPEN"
                showLed: true
                ledOn: root.device.outputOpen
                ledColor: root.device.receiverConnected ? Panel.ledGreen : Panel.ledAmber
                implicitWidth: 104
                onClicked: root.device.outputOpen ? root.device.closeOutput() : root.openSelected()
            }
        }

        // Serial settings
        RowLayout {
            visible: root.serialSelected
            spacing: 8
            Text { text: "PORT"; color: Panel.label; font.pixelSize: 11; font.bold: true }
            ComboBox {
                id: portBox
                Layout.preferredWidth: 120
                enabled: !root.device.outputOpen
            }
            PanelKey {
                text: "↻"
                implicitWidth: 40
                implicitHeight: 32
                fontSize: 15
                enabled: !root.device.outputOpen
                onClicked: root.refreshPorts()
            }
            Text { text: "BAUD"; color: Panel.label; font.pixelSize: 11; font.bold: true }
            ComboBox {
                id: baudBox
                Layout.preferredWidth: 100
                enabled: !root.device.outputOpen
                model: root.device.standardBaudRates()
            }
        }

        // TCP settings
        RowLayout {
            visible: !root.serialSelected
            spacing: 8
            Text { text: "PORT"; color: Panel.label; font.pixelSize: 11; font.bold: true }
            SpinBox {
                id: tcpPort
                from: 1024
                to: 65535
                value: 5555
                editable: true
                enabled: !root.device.outputOpen
                textFromValue: (value) => String(value) // no thousands separator
            }
            Text { text: "127.0.0.1 only"; color: Panel.labelMuted; font.pixelSize: 11 }
        }

        Text {
            Layout.fillWidth: true
            text: root.serialSelected && portBox.count === 0
                  ? "No serial port found - create a virtual pair (e.g. com0com)"
                  : root.device.outputStatus
            color: Panel.labelMuted
            font.pixelSize: 11
            elide: Text.ElideRight
        }
    }
}
