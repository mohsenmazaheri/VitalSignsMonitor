# vsm_simulation - the simulated bedside device (synthetic patient, scenarios,
# frame generation, TCP/serial output). Shared by the console simulator and
# the patient-simulator GUI, and unit-tested on its own.
qt_add_library(vsm_simulation STATIC
    Scenario.h Scenario.cpp
    SyntheticPatient.h SyntheticPatient.cpp
    IByteSink.h
    TcpBroadcastSink.h TcpBroadcastSink.cpp
    SerialSink.h SerialSink.cpp
    SinkFactory.h SinkFactory.cpp
    DeviceSimulator.h DeviceSimulator.cpp
)
add_library(vsm::simulation ALIAS vsm_simulation)

target_include_directories(vsm_simulation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vsm_simulation
    PUBLIC  Qt6::Core Qt6::Network Qt6::SerialPort vsm::protocol
)
vsm_set_warnings(vsm_simulation)
