# Unit tests (GoogleTest, fetched at configure time).
include(FetchContent)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # match the MSVC runtime used by Qt
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# Tests get their own output folder: the Qt DLLs copied next to them below
# must not end up beside the application, where they would break Qt's
# plugin lookup (platform plugin, QML) when the app is started from the IDE.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin")

# --- Pure C++ tests: protocol + core (no Qt at runtime) -----------------------
add_executable(vsm_unit_tests
    protocol/Crc16Test.cpp
    protocol/FrameCodecTest.cpp
    protocol/FrameDecoderFuzzTest.cpp
    protocol/FrameDecoderTest.cpp
    protocol/MessagesTest.cpp
    audit/FileAuditLogTest.cpp
    core/AlarmEvaluatorTest.cpp
    core/AlarmLimitsTest.cpp
    core/AlarmManagerTest.cpp
    core/AlarmToneTest.cpp
    core/SweepBufferTest.cpp
)
target_link_libraries(vsm_unit_tests PRIVATE vsm::protocol vsm::core vsm::audit GTest::gtest_main)
vsm_set_warnings(vsm_unit_tests)
gtest_discover_tests(vsm_unit_tests)

# --- Device layer tests (Qt Core event loop, fake transport) ------------------
add_executable(vsm_device_tests
    device/main.cpp
    device/FakeTransport.h
    device/DeviceSessionTest.cpp
)
target_link_libraries(vsm_device_tests PRIVATE vsm::device vsm::protocol GTest::gtest)
vsm_set_warnings(vsm_device_tests)

# On Windows the test executable needs the Qt DLLs next to it, both for ctest
# and for test discovery (which runs the executable at build time).
if(WIN32)
    add_custom_command(TARGET vsm_device_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:vsm_device_tests> $<TARGET_FILE_DIR:vsm_device_tests>
        COMMAND_EXPAND_LISTS
    )
endif()
gtest_discover_tests(vsm_device_tests DISCOVERY_MODE PRE_TEST)

# --- Simulator engine tests (fake clock, in-memory sink) ----------------------
if(TARGET vsm_simulation)
    add_executable(vsm_simulation_tests
        device/main.cpp # same QCoreApplication + GoogleTest entry point
        simulation/DeviceSimulatorTest.cpp
    )
    target_link_libraries(vsm_simulation_tests PRIVATE vsm::simulation vsm::protocol GTest::gtest)
    vsm_set_warnings(vsm_simulation_tests)
    if(WIN32)
        add_custom_command(TARGET vsm_simulation_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_RUNTIME_DLLS:vsm_simulation_tests> $<TARGET_FILE_DIR:vsm_simulation_tests>
            COMMAND_EXPAND_LISTS
        )
    endif()
    gtest_discover_tests(vsm_simulation_tests DISCOVERY_MODE PRE_TEST)
endif()
