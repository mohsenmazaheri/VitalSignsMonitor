import QtQuick
import QtQuick.Layouts
import VitalSigns.Ui
import VitalSigns.PatientSimulator

// Colour LCD of the simulator: what the device is generating and sending.
Rectangle {
    id: root

    required property PatientSimulatorController device

    readonly property bool powered: device.powered

    function scenarioLabel(name) {
        return name.length > 0 ? "SCENARIO " + name.toUpperCase() : "MANUAL"
    }
    function clock(seconds) {
        const m = Math.floor(seconds / 60)
        const s = seconds % 60
        return (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s
    }

    radius: 12
    color: Panel.bezel

    Rectangle {
        id: glass
        anchors.fill: parent
        anchors.margins: 14
        radius: 4
        color: root.powered ? Panel.lcd : "#030608"
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 6
            visible: root.powered

            // --- Status bar --------------------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                Text {
                    text: root.scenarioLabel(root.device.scenario)
                    color: root.device.scenario.length > 0 ? Panel.warning : Panel.lcdText
                    font.family: Panel.monoFont
                    font.pixelSize: 15
                    font.bold: true
                }
                Text {
                    Layout.fillWidth: true
                    text: root.device.phase
                    color: Panel.lcdText
                    font.family: Panel.monoFont
                    font.pixelSize: 15
                    elide: Text.ElideRight
                }
                Text {
                    visible: root.device.scenario.length > 0
                    text: root.clock(root.device.scenarioSeconds)
                    color: Panel.lcdDim
                    font.family: Panel.monoFont
                    font.pixelSize: 15
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Panel.lcdGrid }

            // --- ECG preview ---------------------------------------------------------
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                readonly property real minimumMv: -0.6
                readonly property real maximumMv: 1.4

                // 0.5 mV grid
                Repeater {
                    model: 5
                    delegate: Rectangle {
                        required property int index
                        width: parent.width
                        height: 1
                        y: ecgTrace.y + ecgTrace.height * (1 - (index * 0.5 + 0.1) / 2.0)
                        color: Panel.lcdGrid
                    }
                }

                WaveformItem {
                    id: ecgTrace
                    anchors.fill: parent
                    anchors.topMargin: 20
                    series: root.device.ecg
                    color: Panel.ecg
                    minimumValue: parent.minimumMv
                    maximumValue: parent.maximumMv
                }

                Text {
                    text: "ECG II   0.5 mV/div   %1 Hz".arg(root.device.sampleRateHz)
                    color: Panel.ecg
                    font.family: Panel.monoFont
                    font.pixelSize: 13
                }

                Text {
                    anchors.centerIn: parent
                    visible: root.device.leadsOff
                    text: "LEADS OFF"
                    color: Panel.warning
                    font.family: Panel.monoFont
                    font.pixelSize: 30
                    font.bold: true
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Panel.lcdGrid }

            // --- Numerics being reported -----------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                spacing: 24

                Repeater {
                    model: [
                        { name: "HR", unit: "bpm", value: root.device.heartRate, set: root.device.targetHeartRate,
                          colour: Panel.ecg, off: root.device.leadsOff },
                        { name: "SpO2", unit: "%", value: root.device.spo2, set: root.device.targetSpo2,
                          colour: Panel.spo2, off: root.device.probeOff }
                    ]
                    delegate: RowLayout {
                        required property var modelData
                        spacing: 8

                        Text {
                            Layout.alignment: Qt.AlignTop
                            text: modelData.name
                            color: modelData.colour
                            font.family: Panel.monoFont
                            font.pixelSize: 15
                            font.bold: true
                        }
                        Text {
                            Layout.preferredWidth: 96
                            horizontalAlignment: Text.AlignRight
                            text: modelData.value >= 0 ? modelData.value : "---"
                            color: modelData.colour
                            font.family: Panel.monoFont
                            font.pixelSize: 50
                            font.bold: true
                        }
                        ColumnLayout {
                            spacing: 2
                            Text {
                                text: modelData.unit
                                color: Panel.lcdDim
                                font.family: Panel.monoFont
                                font.pixelSize: 13
                            }
                            Text {
                                text: modelData.off ? "SENSOR OFF" : "SET " + modelData.set
                                color: modelData.off ? Panel.warning : Panel.lcdDim
                                font.family: Panel.monoFont
                                font.pixelSize: 13
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Panel.lcdGrid }

            // --- Output line -------------------------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                Text {
                    Layout.fillWidth: true
                    text: "OUT " + root.device.outputDescription + "  " + root.device.outputStatus
                    color: root.device.outputOpen ? Panel.lcdText : Panel.fault
                    font.family: Panel.monoFont
                    font.pixelSize: 13
                    elide: Text.ElideRight
                }
                Text {
                    text: root.device.bytesPerSecond + " B/s"
                          + (root.device.lineLoadPercent >= 0 ? "  LOAD " + root.device.lineLoadPercent + "%" : "")
                    color: root.device.lineLoadPercent > 80 ? Panel.warning : Panel.lcdDim
                    font.family: Panel.monoFont
                    font.pixelSize: 13
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                Text {
                    text: "FRAMES " + root.device.framesSent.toFixed(0)
                    color: Panel.lcdDim
                    font.family: Panel.monoFont
                    font.pixelSize: 13
                }
                Text {
                    text: "CORRUPTED " + root.device.framesCorrupted.toFixed(0)
                          + " (" + root.device.corruptionPercent.toFixed(1) + "%)"
                    color: root.device.corruptionPercent > 0 ? Panel.warning : Panel.lcdDim
                    font.family: Panel.monoFont
                    font.pixelSize: 13
                }
                Text {
                    text: root.device.cableConnected ? "CABLE OK" : "CABLE UNPLUGGED"
                    color: root.device.cableConnected ? Panel.lcdDim : Panel.fault
                    font.family: Panel.monoFont
                    font.pixelSize: 13
                    font.bold: !root.device.cableConnected
                }
                Item { Layout.fillWidth: true }
            }
            Text {
                Layout.fillWidth: true
                visible: root.device.lastError.length > 0
                text: "ERR " + root.device.lastError
                color: Panel.fault
                font.family: Panel.monoFont
                font.pixelSize: 13
                elide: Text.ElideRight
            }
        }

        // Glass reflection
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#14ffffff" }
                GradientStop { position: 0.35; color: "#00ffffff" }
            }
        }
    }
}
