import QtQuick

// Panel indicator lamp with an optional caption below it.
Column {
    id: root

    property color color: Panel.ledGreen
    property bool on: false
    property string label
    property int size: 12

    spacing: 4

    Item {
        width: Math.max(root.size, caption.implicitWidth)
        height: root.size

        // Soft halo of a lit lamp.
        Rectangle {
            anchors.centerIn: lamp
            width: root.size * 2
            height: width
            radius: width / 2
            color: root.color
            opacity: root.on ? 0.22 : 0
        }

        Rectangle {
            id: lamp
            anchors.centerIn: parent
            width: root.size
            height: root.size
            radius: root.size / 2
            color: root.on ? root.color : Panel.ledOff
            border.color: "#202326"
            border.width: 1

            // Specular highlight of the lens.
            Rectangle {
                x: parent.width * 0.22
                y: parent.height * 0.18
                width: parent.width * 0.32
                height: width
                radius: width / 2
                color: "white"
                opacity: root.on ? 0.7 : 0.25
            }
        }
    }

    Text {
        id: caption
        anchors.horizontalCenter: parent.horizontalCenter
        visible: root.label.length > 0
        text: root.label
        color: Panel.label
        font.pixelSize: 10
        font.bold: true
        font.letterSpacing: 0.8
    }
}
