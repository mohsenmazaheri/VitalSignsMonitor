import QtQuick
import QtQuick.Controls

// Machined rotary knob with a scale. Drag up/down or use the mouse wheel;
// arrow keys work when it has focus.
Dial {
    id: dial

    implicitWidth: 124
    implicitHeight: 124
    stepSize: 1
    snapMode: Dial.SnapAlways
    inputMode: Dial.Vertical
    wheelEnabled: true

    readonly property real bodySize: Math.min(width, height) - 30

    background: Item {
        width: dial.width
        height: dial.height

        // Scale: 21 ticks over the knob's travel (-140 deg .. +140 deg).
        Repeater {
            model: 21
            delegate: Rectangle {
                required property int index
                x: dial.width / 2 - width / 2
                y: 0
                width: 2
                height: index % 5 === 0 ? 9 : 5
                color: Panel.label
                antialiasing: true
                transform: Rotation {
                    origin.x: 1
                    origin.y: dial.height / 2
                    angle: -140 + index * 14
                }
            }
        }

        // Knob body
        Rectangle {
            id: body
            anchors.centerIn: parent
            width: dial.bodySize
            height: width
            radius: width / 2
            border.color: "#5f656c"
            border.width: 1
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#fafbfc" }
                GradientStop { position: 1.0; color: "#8f969d" }
            }
        }

        // Cap
        Rectangle {
            anchors.centerIn: parent
            width: dial.bodySize * 0.72
            height: width
            radius: width / 2
            border.color: dial.visualFocus ? Panel.accent : "#7b8188"
            border.width: dial.visualFocus ? 2 : 1
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#a9afb6" }
                GradientStop { position: 1.0; color: "#eef0f2" }
            }
        }
    }

    // Pointer line, rotated with the knob position.
    handle: Rectangle {
        x: dial.width / 2 - width / 2
        y: dial.height / 2 - height / 2
        width: 4
        height: dial.bodySize * 0.24
        radius: 2
        color: dial.enabled ? Panel.accent : Panel.labelMuted
        antialiasing: true
        transform: [
            Translate { y: -dial.bodySize * 0.22 },
            Rotation {
                angle: dial.angle
                origin.x: 2
                origin.y: dial.bodySize * 0.12
            }
        ]
    }
}
