import QtQuick
import QtQuick.Controls
import VitalSigns.Ui

// Framed sweep waveform with title, scale grid and status overlay.
Rectangle {
    id: root

    required property WaveformSeries series
    property string title
    property color traceColor: Theme.ecg
    property string statusText
    property real minimumValue: -1.0 // mV
    property real maximumValue: 2.0  // mV

    color: Theme.panel
    border.color: Theme.panelBorder
    radius: Theme.radius

    // 0.5 mV horizontal grid lines, like ECG paper (coarse).
    Repeater {
        model: Math.floor((root.maximumValue - root.minimumValue) / 0.5) + 1
        delegate: Rectangle {
            required property int index
            x: plot.x
            width: plot.width
            height: 1
            y: plot.y + plot.height - index * 0.5 / (root.maximumValue - root.minimumValue) * plot.height
            color: Theme.panelBorder
            opacity: 0.6
        }
    }

    WaveformItem {
        id: plot
        anchors.fill: parent
        anchors.margins: 2 * Theme.spacing
        anchors.topMargin: 4 * Theme.spacing
        series: root.series
        color: root.traceColor
        minimumValue: root.minimumValue
        maximumValue: root.maximumValue
    }

    Label {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: Theme.spacing
        text: root.title + "   " + qsTr("0.5 mV/div   %1 s sweep").arg(root.series.sweepSeconds)
        color: root.traceColor
        font.pixelSize: 14
        font.bold: true
    }

    Label {
        anchors.centerIn: parent
        visible: root.statusText.length > 0
        text: root.statusText
        color: Theme.alarmLow
        font.pixelSize: 28
        font.bold: true
    }
}
