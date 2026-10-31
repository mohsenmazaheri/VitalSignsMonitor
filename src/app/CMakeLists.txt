# VitalSignsMonitor - Qt Quick desktop application (presentation layer).
qt_add_executable(VitalSignsMonitor
    main.cpp
)

# Theme.qml is a singleton (pragma Singleton). Must be set before qt_add_qml_module().
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(VitalSignsMonitor
    URI VitalSigns.App
    DEPENDENCIES VitalSigns.Ui
    VERSION 1.0
    SOURCES
        MonitorViewModel.h MonitorViewModel.cpp
        AlarmListModel.h AlarmListModel.cpp
    QML_FILES
        qml/Main.qml
        qml/ConnectionBar.qml
        qml/AlarmBanner.qml
        qml/WaveformPanel.qml
        qml/NumericTile.qml
        qml/Theme.qml
)

target_link_libraries(VitalSignsMonitor PRIVATE
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::SerialPort
    vsm::core
    vsm::device
    vsm::ui
    vsm_uiplugin # static QML plugin of VitalSigns.Ui, imported in main.cpp
)

target_compile_definitions(VitalSignsMonitor PRIVATE
    VSM_VERSION="${PROJECT_VERSION}"
)

set_target_properties(VitalSignsMonitor PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
vsm_set_warnings(VitalSignsMonitor)
