import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window

    // Injected from main.cpp via setInitialProperties().
    required property MonitorViewModel monitor

    width: 1280
    height: 760
    minimumWidth: 960
    minimumHeight: 600
    visible: true
    title: qsTr("VitalSigns Monitor %1").arg(monitor.version)
    color: Theme.background

    palette {
        window: Theme.panel
        windowText: Theme.text
        base: Theme.background
        text: Theme.text
        button: Theme.panelBorder
        buttonText: Theme.text
        highlight: Theme.spo2
        highlightedText: Theme.background
        placeholderText: Theme.textMuted
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacing
        spacing: Theme.spacing

        ConnectionBar {
            Layout.fillWidth: true
            monitor: window.monitor
        }

        AlarmBanner {
            Layout.fillWidth: true
            alarms: window.monitor.alarms
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacing

            WaveformPanel {
                Layout.fillWidth: true
                Layout.minimumWidth: 320
                Layout.fillHeight: true
                series: window.monitor.ecg
                title: qsTr("ECG II")
                traceColor: Theme.ecg
                statusText: window.monitor.ecgLeadsOff ? qsTr("LEADS OFF") : ""
            }

            ColumnLayout {
                Layout.fillWidth: false // layouts fill by default; keep the numerics column fixed
                Layout.preferredWidth: 280
                Layout.fillHeight: true
                spacing: Theme.spacing

                NumericTile {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    label: qsTr("HR")
                    unit: qsTr("bpm")
                    value: window.monitor.heartRate
                    accent: Theme.ecg
                    alarmPriority: window.monitor.heartRateAlarmPriority
                }

                NumericTile {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    label: qsTr("SpO₂")
                    unit: qsTr("%")
                    value: window.monitor.spo2
                    accent: Theme.spo2
                    alarmPriority: window.monitor.spo2AlarmPriority
                    statusText: window.monitor.spo2ProbeOff ? qsTr("PROBE OFF") : ""
                }
            }
        }

        // Footer: link health + mandatory disclaimer.
        RowLayout {
            Layout.fillWidth: true

            Label {
                text: window.monitor.linkStatistics
                color: Theme.textMuted
                font.pixelSize: 12
            }
            Item { Layout.fillWidth: true }
            Label {
                text: qsTr("SYNTHETIC DATA - NOT A MEDICAL DEVICE - NOT FOR CLINICAL USE")
                color: Theme.alarmMedium
                font.pixelSize: 12
                font.bold: true
            }
        }
    }
}
