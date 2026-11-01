# vsm_device - talks to the (simulated) bedside device.
# Owns the I/O thread, the byte transports (TCP / serial) and the session that
# turns frames into domain objects. Depends on Qt Core/Network/SerialPort.
qt_add_library(vsm_device STATIC
    include/vsm/device/ITransport.h
    include/vsm/device/TcpTransport.h
    include/vsm/device/SerialTransport.h
    include/vsm/device/ConnectionSettings.h
    include/vsm/device/TransportFactory.h
    include/vsm/device/LinkStatistics.h
    include/vsm/device/DeviceSession.h
    include/vsm/device/DeviceController.h
    TcpTransport.cpp
    SerialTransport.cpp
    TransportFactory.cpp
    DeviceSession.cpp
    DeviceController.cpp
)
add_library(vsm::device ALIAS vsm_device)

target_include_directories(vsm_device PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(vsm_device
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::SerialPort
        vsm::core
    PRIVATE
        vsm::protocol
)
vsm_set_warnings(vsm_device)
