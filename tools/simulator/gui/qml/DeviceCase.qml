import QtQuick
import QtQuick.Layouts
import VitalSigns.PatientSimulator

// The whole front panel, laid out at a fixed design size (Panel.caseWidth x
// Panel.caseHeight) like a real instrument; Main.qml scales it to the window.
Rectangle {
    id: root

    required property PatientSimulatorController device

    // Labels of the scenario keys, by scenario name.
    readonly property var scenarioKeyLabels: ({
        "normal": "NORMAL", "tachycardia": "TACHY", "bradycardia": "BRADY",
        "desaturation": "DESAT", "sensor-off": "SENSOR", "demo": "DEMO"
    })

    width: Panel.caseWidth
    height: Panel.caseHeight
    radius: 26
    border.color: Panel.caseEdge
    border.width: 2
    gradient: Gradient {
        GradientStop { position: 0.0; color: Panel.caseTop }
        GradientStop { position: 1.0; color: Panel.caseBottom }
    }

    // TX lamp flickers while frames really leave the device, like a UART
    // activity LED (it toggles every few frames; frames come every 40 ms).
    readonly property bool transmitting: device.powered && device.cableConnected && device.receiverConnected
    readonly property bool txLampOn: transmitting && Math.floor(device.framesSent / 3) % 2 === 0

    // --- Header: brand plate and status lamps -----------------------------------------
    RowLayout {
        id: header
        x: 28
        y: 16
        width: parent.width - 56
        height: 46
        spacing: 18

        Text {
            text: "VSM"
            color: Panel.accent
            font.pixelSize: 32
            font.bold: true
            font.italic: true
        }
        ColumnLayout {
            spacing: 0
            Text {
                text: "PS-1  PATIENT SIMULATOR"
                color: Panel.label
                font.pixelSize: 16
                font.bold: true
                font.letterSpacing: 2
            }
            Text {
                text: "ECG  ·  HEART RATE  ·  SpO2      SERIAL / TCP"
                color: Panel.labelMuted
                font.pixelSize: 11
                font.letterSpacing: 1
            }
        }

        Item { Layout.fillWidth: true }

        Led { label: "POWER"; on: root.device.powered; color: Panel.ledGreen }
        Led { label: "TX"; on: root.txLampOn; color: Panel.ledAmber }
        Led { label: "LINK"; on: root.device.powered && root.device.receiverConnected; color: Panel.ledGreen }
        Led {
            label: "FAULT"
            on: root.device.powered && (!root.device.cableConnected || root.device.corruptionPercent > 0
                                        || root.device.lastError.length > 0)
            color: Panel.ledRed
        }
    }

    // --- Main row: LCD and parameter controls --------------------------------------------
    LcdScreen {
        id: lcd
        x: 24
        y: 76
        width: 640
        height: 440
        device: root.device
    }

    ColumnLayout {
        x: lcd.x + lcd.width + 18
        y: lcd.y
        width: root.width - x - 24
        height: lcd.height
        spacing: 12

        ParameterGroup {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "ECG  ·  HEART RATE"
            unit: "bpm"
            from: root.device.minimumHeartRate
            to: root.device.maximumHeartRate
            value: root.device.targetHeartRate
            presets: [30, 45, 60, 75, 120, 160]
            sensorKeyText: "LEADS OFF"
            sensorOff: root.device.leadsOff
            accent: Panel.ecg
            onValueRequested: (v) => root.device.targetHeartRate = v
            onSensorOffRequested: (off) => root.device.leadsOff = off
        }
        ParameterGroup {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "SpO2"
            unit: "%"
            from: root.device.minimumSpo2
            to: root.device.maximumSpo2
            value: root.device.targetSpo2
            presets: [80, 85, 88, 92, 95, 98]
            sensorKeyText: "PROBE OFF"
            sensorOff: root.device.probeOff
            accent: Panel.spo2
            onValueRequested: (v) => root.device.targetSpo2 = v
            onSensorOffRequested: (off) => root.device.probeOff = off
        }
    }

    // --- Lower row: scenarios, output, faults -------------------------------------------------
    RowLayout {
        x: 24
        y: lcd.y + lcd.height + 16
        width: root.width - 48
        height: 172
        spacing: 12

        GroupFrame {
            Layout.preferredWidth: 384
            Layout.fillHeight: true
            title: "SCENARIO"

            GridLayout {
                anchors.fill: parent
                columns: 4
                rowSpacing: 10
                columnSpacing: 6

                Repeater {
                    model: root.device.scenarioNames
                    delegate: PanelKey {
                        required property string modelData
                        Layout.fillWidth: true
                        text: root.scenarioKeyLabels[modelData] ?? modelData.toUpperCase()
                        showLed: true
                        ledOn: root.device.scenario === modelData
                        ledColor: Panel.ledAmber
                        fontSize: 11
                        onClicked: root.device.runScenario(modelData)
                    }
                }
                PanelKey {
                    Layout.fillWidth: true
                    text: "MANUAL"
                    showLed: true
                    ledOn: root.device.scenario.length === 0
                    fontSize: 11
                    onClicked: root.device.stopScenario()
                }
            }
        }

        OutputPanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            device: root.device
        }

        GroupFrame {
            Layout.preferredWidth: 300
            Layout.fillHeight: true
            title: "FAULT INJECTION"

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                PanelKey {
                    Layout.fillWidth: true
                    text: root.device.cableConnected ? "CABLE CONNECTED" : "CABLE UNPLUGGED"
                    showLed: true
                    ledOn: true
                    ledColor: root.device.cableConnected ? Panel.ledGreen : Panel.ledRed
                    fontSize: 11
                    onClicked: root.device.cableConnected = !root.device.cableConnected
                }

                Text {
                    text: "CORRUPT FRAMES"
                    color: Panel.labelMuted
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 1.2
                }
                RowLayout {
                    spacing: 6
                    Repeater {
                        model: [0, 1, 5, 20]
                        delegate: PanelKey {
                            required property int modelData
                            Layout.fillWidth: true
                            text: modelData + "%"
                            showLed: true
                            ledOn: Math.round(root.device.corruptionPercent) === modelData
                            ledColor: modelData === 0 ? Panel.ledGreen : Panel.ledRed
                            implicitWidth: 60
                            fontSize: 11
                            onClicked: root.device.corruptionPercent = modelData
                        }
                    }
                }
            }
        }
    }

    // --- Footer: power key and safety label ------------------------------------------------------
    RowLayout {
        x: 28
        y: root.height - 54
        width: root.width - 56
        height: 40
        spacing: 16

        PanelKey {
            text: "POWER"
            showLed: true
            ledOn: root.device.powered
            implicitWidth: 112
            onClicked: root.device.powered = !root.device.powered
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            radius: 4
            color: "#f6f0c9"
            border.color: "#b9ad6a"

            Text {
                anchors.centerIn: parent
                text: "SYNTHETIC SIGNALS ONLY  ·  NOT A MEDICAL DEVICE  ·  FOR SOFTWARE TESTING"
                color: "#4a4320"
                font.pixelSize: 12
                font.bold: true
                font.letterSpacing: 1
            }
        }

        Text {
            text: "v" + root.device.version
            color: Panel.labelMuted
            font.pixelSize: 11
        }
    }
}
