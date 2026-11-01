# vsm_ui - reusable Qt Quick building blocks (QML module "VitalSigns.Ui").
# Shared by the monitor and the patient-simulator GUI so that both draw the
# ECG sweep with the same scene-graph code.
qt_add_library(vsm_ui STATIC)
add_library(vsm::ui ALIAS vsm_ui)

qt_add_qml_module(vsm_ui
    URI VitalSigns.Ui
    VERSION 1.0
    SOURCES
        include/vsm/ui/WaveformSeries.h WaveformSeries.cpp
        include/vsm/ui/WaveformItem.h WaveformItem.cpp
)

target_include_directories(vsm_ui
    PUBLIC include
    # qmltyperegistrar includes the headers by file name.
    PRIVATE include/vsm/ui
)
target_link_libraries(vsm_ui PUBLIC Qt6::Quick vsm::core)
vsm_set_warnings(vsm_ui)
