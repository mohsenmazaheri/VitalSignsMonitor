import QtQuick
import QtQuick.Controls

// Membrane key of the front panel. A checkable key shows a status LED, like
// the mode keys of bench-top instruments.
AbstractButton {
    id: key

    property bool showLed: checkable
    property bool ledOn: checked
    property color ledColor: Panel.ledGreen
    property color faceColor: Panel.keyTop
    property color textColor: Panel.keyText
    property int fontSize: 12

    implicitWidth: 92
    implicitHeight: 40
    focusPolicy: Qt.TabFocus
    hoverEnabled: true

    background: Item {
        // Drop shadow under the key.
        Rectangle {
            anchors.fill: face
            anchors.topMargin: 3
            anchors.bottomMargin: -3
            radius: face.radius
            color: Panel.keyShadow
            opacity: key.down ? 0.25 : 0.6
        }

        Rectangle {
            id: face
            width: parent.width
            height: parent.height
            y: key.down ? 2 : 0
            radius: 6
            border.color: key.visualFocus ? Panel.accent : Panel.keyBorder
            border.width: key.visualFocus ? 2 : 1
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.lighter(key.faceColor, key.hovered ? 1.04 : 1.0) }
                GradientStop { position: 1.0; color: Qt.darker(key.faceColor, key.down ? 1.22 : 1.12) }
            }
        }
    }

    contentItem: Item {
        y: key.down ? 2 : 0
        opacity: key.enabled ? 1.0 : 0.4

        Row {
            anchors.centerIn: parent
            spacing: 7

            Led {
                anchors.verticalCenter: parent.verticalCenter
                visible: key.showLed
                size: 9
                on: key.ledOn
                color: key.ledColor
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: key.text
                color: key.textColor
                font.pixelSize: key.fontSize
                font.bold: true
                font.letterSpacing: 0.5
            }
        }
    }
}
