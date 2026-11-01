import QtQuick
import QtQuick.Controls
import VitalSigns.PatientSimulator

// Window: the device front panel on a dark desk, scaled uniformly so that the
// proportions of the instrument stay the same at any window size.
ApplicationWindow {
    id: window

    required property PatientSimulatorController device

    // Start at the panel's design size, but never larger than the screen.
    width: Math.min(Panel.caseWidth + 40, Screen.desktopAvailableWidth - 40)
    height: Math.min(Panel.caseHeight + 40, Screen.desktopAvailableHeight - 60)
    minimumWidth: 800
    minimumHeight: 540
    visible: true
    title: qsTr("VitalSigns Patient Simulator %1 - synthetic data").arg(device.version)
    color: Panel.desk

    DeviceCase {
        id: panel
        anchors.centerIn: parent
        device: window.device
        scale: Math.min((window.width - 40) / width, (window.height - 40) / height)
    }
}
