# vsm_protocol - wire format shared by the monitor and the simulator.
# Pure C++20: no Qt dependency, so it is trivial to unit-test and to reuse.
add_library(vsm_protocol STATIC
    include/vsm/protocol/Crc16.h
    include/vsm/protocol/Frame.h
    include/vsm/protocol/FrameDecoder.h
    include/vsm/protocol/Messages.h
    Crc16.cpp
    Frame.cpp
    FrameDecoder.cpp
    Messages.cpp
)
add_library(vsm::protocol ALIAS vsm_protocol)

target_include_directories(vsm_protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(vsm_protocol PUBLIC cxx_std_20)
vsm_set_warnings(vsm_protocol)
