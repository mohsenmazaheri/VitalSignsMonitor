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

# --- Monitor view model tests (alarm chain, stale data, audit; no QML, no window) -
# The view model holds safety behaviour (clearing stale data), so it is tested too.
# Its sources are compiled in directly: they belong to the executable VitalSignsMonitor.
set(VSM_APP_DIR ${CMAKE_SOURCE_DIR}/src/app)
add_executable(vsm_app_tests
    device/main.cpp
    device/FakeTransport.h
    app/MonitorViewModelTest.cpp
    ${VSM_APP_DIR}/AlarmListModel.h ${VSM_APP_DIR}/AlarmListModel.cpp
    ${VSM_APP_DIR}/AuditLogModel.h ${VSM_APP_DIR}/AuditLogModel.cpp
    ${VSM_APP_DIR}/IAlarmSound.h
    ${VSM_APP_DIR}/MonitorViewModel.h ${VSM_APP_DIR}/MonitorViewModel.cpp
)
target_include_directories(vsm_app_tests PRIVATE ${VSM_APP_DIR} device)
target_compile_definitions(vsm_app_tests PRIVATE VSM_VERSION="${PROJECT_VERSION}")
target_link_libraries(vsm_app_tests PRIVATE vsm::device vsm::core vsm::ui Qt6::Qml Qt6::SerialPort GTest::gtest)
vsm_set_warnings(vsm_app_tests)
if(WIN32)
    add_custom_command(TARGET vsm_app_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:vsm_app_tests> $<TARGET_FILE_DIR:vsm_app_tests>
        COMMAND_EXPAND_LISTS
    )
endif()
gtest_discover_tests(vsm_app_tests DISCOVERY_MODE PRE_TEST)

# --- Medical-style documents match the code and the tests -------------------------
# Skipped when no PowerShell is available; CI has it on Windows and Linux.
find_program(VSM_POWERSHELL NAMES pwsh powershell)
if(VSM_POWERSHELL)
    add_test(NAME traceability_check
        COMMAND ${VSM_POWERSHELL} -NoProfile -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/tools/check-traceability.ps1
    )
endif()
