import QtQuick
import QtQuick.Layouts

// Controls for one simulated parameter: knob, set-point readout, step keys,
// preset keys and the "sensor off" key. Emits requests; the owner writes the
// value to the controller, so this component holds no device state.
GroupFrame {
    id: root

    property int value
    property int from
    property int to
    property string unit
    property list<int> presets
    property string sensorKeyText
    property bool sensorOff
    property color accent: Panel.ecg

    signal valueRequested(int value)
    signal sensorOffRequested(bool off)

    function request(v) {
        root.valueRequested(Math.max(root.from, Math.min(root.to, v)))
    }

    RowLayout {
        anchors.fill: parent
        spacing: 16

        RotaryKnob {
            Layout.alignment: Qt.AlignVCenter
            from: root.from
            to: root.to
            value: root.value
            onMoved: root.request(Math.round(value))
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            RowLayout {
                spacing: 10

                // Small set-point display, like the 7-segment window of a bench instrument.
                Rectangle {
                    Layout.preferredWidth: 122
                    Layout.preferredHeight: 52
                    radius: 4
                    color: Panel.lcd
                    border.color: Panel.bezel
                    border.width: 3

                    Text {
                        anchors.right: unitText.left
                        anchors.rightMargin: 6
                        anchors.baseline: unitText.baseline
                        text: root.value
                        color: root.accent
                        font.family: Panel.monoFont
                        font.pixelSize: 34
                        font.bold: true
                    }
                    Text {
                        id: unitText
                        anchors.right: parent.right
                        anchors.rightMargin: 9
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 10
                        text: root.unit
                        color: Panel.lcdDim
                        font.family: Panel.monoFont
                        font.pixelSize: 12
                    }
                    Text {
                        x: 7
                        y: 4
                        text: "SET"
                        color: Panel.lcdDim
                        font.family: Panel.monoFont
                        font.pixelSize: 10
                    }
                }

                ColumnLayout {
                    spacing: 4
                    PanelKey {
                        text: "▲"
                        implicitWidth: 46
                        implicitHeight: 24
                        autoRepeat: true
                        onClicked: root.request(root.value + 1)
                    }
                    PanelKey {
                        text: "▼"
                        implicitWidth: 46
                        implicitHeight: 24
                        autoRepeat: true
                        onClicked: root.request(root.value - 1)
                    }
                }

                Item { Layout.fillWidth: true }

                PanelKey {
                    text: root.sensorKeyText
                    checkable: true
                    checked: root.sensorOff
                    ledColor: Panel.ledAmber
                    implicitWidth: 104
                    implicitHeight: 52
                    fontSize: 11
                    onToggled: root.sensorOffRequested(checked)
                }
            }

            Text {
                text: "PRESETS"
                color: Panel.labelMuted
                font.pixelSize: 10
                font.bold: true
                font.letterSpacing: 1.2
            }

            RowLayout {
                spacing: 6
                Repeater {
                    model: root.presets
                    delegate: PanelKey {
                        required property int modelData
                        text: modelData
                        implicitWidth: 48
                        implicitHeight: 34
                        checkable: true
                        showLed: false
                        checked: root.value === modelData
                        faceColor: checked ? "#cfe3f5" : Panel.keyTop
                        onClicked: root.request(modelData)
                    }
                }
            }
        }
    }
}
