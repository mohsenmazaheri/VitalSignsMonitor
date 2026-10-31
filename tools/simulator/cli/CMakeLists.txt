# VitalSignsSimulator - console program that pretends to be a bedside device.
# It produces SYNTHETIC ECG / HR / SpO2 data and sends it with the vsm protocol
# over TCP (server) or a serial port. Handy for scripts and CI.
qt_add_executable(VitalSignsSimulator
    main.cpp
)
target_link_libraries(VitalSignsSimulator PRIVATE vsm::simulation)
target_compile_definitions(VitalSignsSimulator PRIVATE VSM_VERSION="${PROJECT_VERSION}")
vsm_set_warnings(VitalSignsSimulator)
