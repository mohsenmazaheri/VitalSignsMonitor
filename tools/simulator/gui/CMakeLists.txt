# VitalSignsPatientSimulator - Qt Quick front panel that looks and behaves like
# a hardware patient simulator (LCD, knobs, keys, LEDs). Sends SYNTHETIC data
# over a serial port or TCP using the same engine as the console simulator.
qt_add_executable(VitalSignsPatientSimulator
    main.cpp
)

# Panel.qml is a singleton (pragma Singleton). Must be set before qt_add_qml_module().
set_source_files_properties(qml/Panel.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(VitalSignsPatientSimulator
    URI VitalSigns.PatientSimulator
    DEPENDENCIES VitalSigns.Ui
    VERSION 1.0
    SOURCES
        PatientSimulatorController.h PatientSimulatorController.cpp
    QML_FILES
        qml/Main.qml
        qml/Panel.qml
        qml/DeviceCase.qml
        qml/LcdScreen.qml
        qml/Led.qml
        qml/PanelKey.qml
        qml/RotaryKnob.qml
        qml/ParameterGroup.qml
        qml/GroupFrame.qml
        qml/OutputPanel.qml
)

target_link_libraries(VitalSignsPatientSimulator PRIVATE
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::SerialPort
    vsm::simulation
    vsm::ui
    vsm_uiplugin # static QML plugin of VitalSigns.Ui, imported in main.cpp
)

target_compile_definitions(VitalSignsPatientSimulator PRIVATE VSM_VERSION="${PROJECT_VERSION}")

set_target_properties(VitalSignsPatientSimulator PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
vsm_set_warnings(VitalSignsPatientSimulator)
