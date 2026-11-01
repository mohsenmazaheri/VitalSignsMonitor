import QtQuick

// Recessed area of the case with a printed caption, grouping related controls.
Item {
    id: root

    property string title
    default property alias content: contentArea.data

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: Panel.groupFill
        border.color: Panel.groupBorder
        border.width: 1

        // Inner shadow at the top edge: the area looks pressed into the case.
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 1
            height: 6
            radius: 9
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#22000000" }
                GradientStop { position: 1.0; color: "#00000000" }
            }
        }
    }

    Text {
        id: caption
        x: 14
        y: 8
        text: root.title
        color: Panel.accent
        font.pixelSize: 11
        font.bold: true
        font.letterSpacing: 1.4
    }

    Item {
        id: contentArea
        anchors.fill: parent
        anchors.margins: 12
        anchors.topMargin: 28
    }
}
