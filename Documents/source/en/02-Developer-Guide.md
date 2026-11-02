---
title: Developer Guide
subtitle: VitalSigns Monitor - architecture, code structure, building, testing and extending
docid: VSM-DEV-001
version: 1.2
date: 2 November 2026
author: Mohsen Mazaheri
status: Draft for review
lang: en
output: EN/VitalSignsMonitor-02-Developer-Guide-EN.docx
header: VitalSigns Monitor · Developer Guide
history: 1.0 | 1 Nov 2026 | Mohsen Mazaheri | First issue, based on software version 0.1.0; 1.1 | 1 Nov 2026 | Mohsen Mazaheri | Alarm manager, audit log, alarm sounds and CI (software version 0.2.0); 1.2 | 2 Nov 2026 | Mohsen Mazaheri | Stricter data watchdog, impossible values, risk analysis and traceability (software version 0.3.0)
notice: **Synthetic data only - not a medical device.** Never add real patient data to the code, the tests or the repository.
---

# 1. Introduction

## 1.1 Purpose and audience

This guide is for developers who build, change or review VitalSigns Monitor. It explains the architecture, the role of every module, the build and test process, and the steps for the most common extensions. Installation of the tools is described in the Installation Guide; the use of the programs in the User Guide.

## 1.2 Related documents

| Document | Content |
|---|---|
| README.md | Overview, features, quick build and run |
| docs/ARCHITECTURE.md | Layers, threads, SOLID mapping, alarm model |
| docs/PROTOCOL.md | Byte-level description of the device protocol |
| LEARNING.md | Learning log with design notes and interview questions |
| CLAUDE.md | Short rules for AI-assisted work on the code base |
| VSM-RFP-001 | Requirements with identifiers (FR-..., NFR-...) |
| docs/medical/ | Software requirements, risk analysis and traceability, as a learning exercise |

# 2. Technology stack

| Area | Choice | Note |
|---|---|---|
| Language | C++20 | std::span, std::variant, std::optional, designated initializers, defaulted operator== |
| Framework | Qt 6.8 or newer | Built and tested with Qt 6.12.0 (MinGW 64-bit) |
| User interface | Qt Quick / QML, Qt Quick Controls (Fusion style) | ECG drawn with the Qt Quick scene graph |
| I/O | Qt Network (QTcpSocket, QTcpServer), Qt Serial Port | |
| Sound | Qt Multimedia (QSoundEffect) | Melodies generated in code |
| Build | CMake 3.25+, Ninja or MinGW Makefiles | CMakePresets.json included |
| Tests | GoogleTest 1.15 (fetched at configure time), CTest | 140 tests, including the traceability check |
| Compilers | GCC 13 (MinGW), MSVC 2022, Clang 14+ | Warnings on, warning-free |

# 3. Repository layout

```
VitalSignsMonitor/
  CMakeLists.txt          top-level project, options, Qt lookup
  CMakePresets.json       mingw-debug, windows-msvc, linux-debug
  cmake/                  CompilerWarnings.cmake (warning policy)
  src/
    protocol/             wire format: Frame, FrameDecoder, Messages, CRC-16   (C++20, no Qt)
    core/                 domain: VitalSigns, alarms, SweepBuffer             (C++20, no Qt)
    audit/                FileAuditLog: audit trail as a CSV file             (C++20, no Qt)
    device/               ITransport, TCP/serial transports, DeviceSession, DeviceController
    ui/                   shared QML module VitalSigns.Ui: WaveformSeries, WaveformItem
    app/                  monitor: main.cpp, view models, QML
  tools/simulator/
    engine/               vsm_simulation library: patient model, scenarios, sinks
    cli/                  VitalSignsSimulator (console)
    gui/                  VitalSignsPatientSimulator (front panel, QML)
  tests/                  GoogleTest suites (protocol, core, device, simulation)
  docs/                   ARCHITECTURE.md, PROTOCOL.md, medical/, images/
  tools/check-traceability.ps1   checks docs/medical against code and tests
  .github/workflows/      ci.yml: build and test on Windows and Linux
  Documents/              this document set (Word) and its sources
```

# 4. Architecture

## 4.1 Layers

| Layer | Folder | Depends on | Main types |
|---|---|---|---|
| app | src/app | ui, device, audit, core, Qt Quick, Qt Multimedia | main.cpp, MonitorViewModel, AlarmListModel, AuditLogModel, QtAlarmSound, QML |
| audit | src/audit | core, C++ standard library | FileAuditLog |
| ui | src/ui | core, Qt Quick | WaveformSeries, WaveformItem |
| device | src/device | protocol, core, Qt Network, Qt Serial Port | ITransport, TcpTransport, SerialTransport, DeviceSession, DeviceController |
| core | src/core | C++ standard library only | VitalSigns, Alarm, AlarmLimits, IAlarmRule, AlarmEvaluator, AlarmManager, AlarmTone, IAuditLog, SweepBuffer |
| protocol | src/protocol | C++ standard library only | Frame, FrameDecoder, Messages, crc16CcittFalse |
| simulation | tools/simulator/engine | protocol, Qt Network, Qt Serial Port | SyntheticPatient, Scenario, DeviceSimulator, IByteSink, sinks |
| simulator apps | tools/simulator/cli and gui | simulation (and ui for the GUI) | console main.cpp, PatientSimulatorController, front-panel QML |

Dependency rules:

- `protocol` and `core` stay free of Qt, so they compile and test in milliseconds and could move to other platforms.
- `core` never includes `protocol`. The device layer maps wire messages (NumericsMessage) to domain objects (VitalSigns), so the wire format can change without touching the alarm logic.
- Concrete classes are created only in the `main.cpp` files (composition roots). Everything else receives interfaces or factories.

## 4.2 CMake targets

| Target | Kind | Namespace | Content |
|---|---|---|---|
| vsm_protocol (vsm::protocol) | static library | vsm::protocol | protocol layer |
| vsm_core (vsm::core) | static library | vsm::core | domain and alarms |
| vsm_audit (vsm::audit) | static library | vsm::audit | audit trail file |
| vsm_device (vsm::device) | static library | vsm::device | transports, session, controller |
| vsm_ui (vsm::ui) + vsm_uiplugin | static QML module VitalSigns.Ui | vsm::ui | waveform types |
| vsm_simulation (vsm::simulation) | static library | vsm::simulator | simulator engine |
| VitalSignsMonitor | executable, QML module VitalSigns.App | vsm::app | monitor |
| VitalSignsPatientSimulator | executable, QML module VitalSigns.PatientSimulator | vsm::simulator | front-panel simulator |
| VitalSignsSimulator | console executable | - | console simulator |
| vsm_unit_tests, vsm_device_tests, vsm_simulation_tests, vsm_app_tests | test executables | - | GoogleTest suites |

## 4.3 Threads and data flow

```
I/O thread                                             GUI thread
----------                                             ----------
QTcpSocket / QSerialPort
  -> ITransport::dataReceived(bytes)
  -> DeviceSession: FrameDecoder -> parseMessage
     -> vitalsReceived(VitalSigns)        ==queued==>  DeviceController (relay)
     -> ecgSamplesReceived(mV, rate)      ==queued==>    -> MonitorViewModel
     -> linkLostChanged / statistics      ==queued==>         -> AlarmEvaluator -> AlarmManager -> AlarmListModel, IAlarmSound, IAuditLog
                                                              -> WaveformSeries -> WaveformItem
                                                              -> QML properties
```

- `DeviceController` lives in the GUI thread and owns a QThread. For each connection it creates a DeviceSession, moves it into the I/O thread and starts it with a queued call.
- The transport is a child of the session, so the socket moves to the I/O thread with it.
- On disconnect the session is stopped inside the I/O thread (sockets and timers must be used from their own thread), moved back and deleted in the GUI thread.
- Threads exchange only value types (VitalSigns, QList of float, LinkStatistics) through queued signals. There is no shared mutable state and no mutex.
- `WaveformItem::updatePaintNode()` runs on the render thread while the GUI thread is blocked, so it can read WaveformSeries without locking.

## 4.4 SOLID and design patterns

| Principle or pattern | Where it is applied |
|---|---|
| Single responsibility | FrameDecoder only frames bytes; parseMessage only decodes payloads; DeviceSession runs one link; AlarmEvaluator only evaluates and ranks; WaveformItem only draws |
| Open/closed | New alarm = new IAlarmRule; new transport = new ITransport; new simulator output = new IByteSink; new scenario = new entry in makeScenario() |
| Liskov substitution | FakeTransport, TcpTransport and SerialTransport are interchangeable for DeviceSession (proved by the tests) |
| Interface segregation | ITransport has four methods and three signals; IByteSink is separate from ITransport |
| Dependency inversion | DeviceSession depends on ITransport; DeviceController gets a TransportFactory; MonitorViewModel gets AlarmEvaluator, AlarmManager and IAlarmSound injected; AlarmManager writes to IAuditLog; DeviceSimulator gets an IByteSink and a clock |
| Strategy | IAlarmRule with LimitAlarmRule and ConditionAlarmRule |
| Facade | DeviceController hides the thread and the session from the user interface |
| Decorator | AuditLogModel shows each event on screen and passes it on to the real IAuditLog |
| State machine | AlarmManager: Active, Acknowledged, Latched |
| Factory | createDefaultTransport(), createDefaultSink(), makeDefaultAlarmRules(), makeScenario() |
| MVVM | QML views bind to MonitorViewModel, AlarmListModel, WaveformSeries and PatientSimulatorController |
| Composition root | src/app/main.cpp and tools/simulator/gui/main.cpp build the object graphs |

# 5. Modules

## 5.1 protocol (vsm::protocol)

- `Frame` and `encodeFrame()` build a frame: sync 0xAA 0x55, version 1, message type, sequence, payload length and CRC.
- `FrameDecoder::feed(bytes)` accepts any chunk of the byte stream and returns the complete, valid frames. It keeps DecoderStatistics (frames decoded, CRC errors, invalid headers, discarded bytes).
- `parseMessage(frame)` returns a `Message` (std::variant of HeartbeatMessage, NumericsMessage and EcgWaveformMessage) or std::nullopt for an unknown or malformed payload. `toFrame()` is the reverse.
- `crc16CcittFalse()` computes CRC-16/CCITT-FALSE (polynomial 0x1021, initial value 0xFFFF; check value of "123456789" is 0x29B1).

## 5.2 core (vsm::core)

- `VitalSigns` holds optional heart rate and SpO2 plus the two sensor flags. An empty optional means "not available"; the UI then shows "---".
- `AlarmContext` = VitalSigns + deviceLinkLost.
- `Alarm` has a stable code (for example HR_HIGH), a source, a category, a priority and a message.
- `IAlarmRule::evaluate(context)` returns an alarm or std::nullopt. `LimitAlarmRule` compares a value with a limit; `ConditionAlarmRule` uses a predicate.
- `makeDefaultAlarmRules(AlarmLimits)` creates the nine default rules and rejects inconsistent limits.
- `AlarmEvaluator` evaluates all rules and keeps only the highest priority per source, sorted by priority.
- `AlarmManager` adds memory to the stateless evaluator. An alarm is Active, Acknowledged or Latched. High-priority alarms latch until acknowledged; the audio pause lasts 2 minutes and a new medium or high alarm ends it. Every time-dependent call receives a monotonic "now", so tests never wait.
- `IAuditLog` and `AuditEvent` define the audit trail. The manager only knows this interface, never a file.
- `makeAlarmTone()` generates one repetition of the melody per priority (high 10 pulses every 6 s, medium 3 every 8 s, low 2 every 20 s); `encodeWav()` wraps it in a WAV file image.
- `SweepBuffer` is the ring buffer behind the ECG sweep display.

Alarm chain: AlarmEvaluator says what is true now, AlarmManager remembers what the operator has seen, and the view model passes the result to the alarm list and the sound.

## 5.3 device (vsm::device)

The transport interface:

```
class ITransport : public QObject {
    Q_OBJECT
public:
    enum class State { Disconnected, Connecting, Connected };
    virtual void open() = 0;
    virtual void close() = 0;
    virtual State state() const = 0;
    virtual QString description() const = 0;   // "tcp://127.0.0.1:5555", "serial://COM5@115200"
signals:
    void dataReceived(const QByteArray& data);
    void stateChanged(vsm::device::ITransport::State state);
    void errorOccurred(const QString& message);
};
```

- `TcpTransport` and `SerialTransport` implement it. `createDefaultTransport(ConnectionSettings)` chooses one.
- `DeviceSession` connects a transport to a FrameDecoder, maps messages to domain objects, runs one watchdog each for numerics and ECG (`kDefaultWatchdogTimeout` = 3000 ms; heartbeats do not count), treats impossible values as not available and the reconnect timer (`kDefaultReconnectDelay` = 2000 ms), and counts LinkStatistics (frames, CRC errors, lost frames, sequence gaps, discarded bytes).
- `DeviceController` owns the I/O thread and is the facade the view model talks to.
- Logging categories: `vsm.device.controller`, `vsm.device.session`, `vsm.device.tcp`, `vsm.device.serial`; in the app `vsm.app.sound`.

## 5.4 ui (vsm::ui, QML module VitalSigns.Ui)

- `WaveformSeries` stores the samples to draw (created in C++, passed to QML).
- `WaveformItem` is a QQuickItem that draws the trace with the scene graph (QSGGeometry in DrawLines mode; the gap in front of the sweep is left out).
- The module is a static library. Every application that uses it links `vsm_uiplugin` and calls `Q_IMPORT_QML_PLUGIN(VitalSigns_UiPlugin)`; its own QML module declares `DEPENDENCIES VitalSigns.Ui`.

## 5.5 app (monitor)

- `main.cpp` parses `--connect host:port`, builds DeviceController, AlarmEvaluator and MonitorViewModel, and loads the QML module VitalSigns.App.
- `MonitorViewModel` exposes heart rate, SpO2, sensor flags, alarm priorities, connection state, link statistics and the last error; it offers connectTcp(), connectSerial(), disconnectDevice() and availableSerialPorts().
- `AlarmListModel` is the list model behind the alarm banner and the EVENTS window.
- `AuditLogModel` keeps the latest 500 events for the EVENTS window and passes each one on to the real log (decorator).
- `IAlarmSound` is the sound interface. `QtAlarmSound` writes the generated melodies to a temporary folder and loops them with QSoundEffect, which plays WAV files only; `NullAlarmSound` is used with `--no-sound`.
- `FileAuditLog` (library vsm_audit, no Qt) appends one flushed CSV line per event: `time_utc,event,actor,code,priority,detail`. `isOk()` reports a file that cannot be opened.
- QML: Main.qml, ConnectionBar.qml, AlarmBanner.qml, EventLogPopup.qml, WaveformPanel.qml, NumericTile.qml, Theme.qml (colour singleton).

## 5.6 Simulator engine (vsm::simulator)

- `SyntheticPatient` builds the ECG from Gaussian P, Q, R, S and T waves with a small baseline wander, and moves HR and SpO2 towards their targets (3 bpm/s and 0.5 %/s).
- `Scenario` is a list of timed phases with PatientTargets; `makeScenario(name)` creates the six built-in scenarios. A scenario repeats after its last phase.
- `DeviceSimulator` calls `tick()` every 40 ms: it generates the ECG samples since the last tick, sends an ECG frame, and once per second sends numerics and a heartbeat. It supports scenario mode and manual mode, an unplugged cable (frames are numbered but not sent) and a corruption probability, and keeps TransmitStatistics. The clock is injected as `std::function<qint64()>`.
- `IByteSink` is the output interface; `TcpBroadcastSink` (QTcpServer on 127.0.0.1, sends to every connected monitor) and `SerialSink` (8N1, write-only, drops data when more than 64 KiB are pending) implement it. `createDefaultSink(OutputSettings)` chooses one.

## 5.7 Simulator applications

- Console: `tools/simulator/cli/main.cpp` - options --scenario, --port, --serial, --baud, --rate, --corrupt; prints phase changes and sink status.
- GUI: `PatientSimulatorController` is the view model of the front panel (power, scenario, targets, sensor keys, output, fault injection, statistics). The QML module VitalSigns.PatientSimulator contains Main.qml, DeviceCase.qml, LcdScreen.qml, ParameterGroup.qml, RotaryKnob.qml, PanelKey.qml, Led.qml, GroupFrame.qml, OutputPanel.qml and Panel.qml (colour singleton). The panel is laid out at 1200 x 770 and scaled as a whole.

# 6. Device protocol (summary)

All multi-byte fields are little-endian. Details: docs/PROTOCOL.md.

| Offset | Size | Field | Notes |
|---|---|---|---|
| 0 | 2 | Sync | 0xAA 0x55 |
| 2 | 1 | Version | 1 |
| 3 | 1 | Message type | 0x01 heartbeat, 0x10 numerics, 0x20 ECG waveform |
| 4 | 2 | Sequence | +1 per frame, wraps 65535 to 0 |
| 6 | 2 | Payload length | N, at most 1024 |
| 8 | N | Payload | depends on the message type |
| 8+N | 2 | CRC | CRC-16/CCITT-FALSE over bytes 2 to 8+N-1 |

Receiver rules: search for the sync bytes and count the bytes skipped; reject a bad version or a length above 1024 before waiting for the payload; on a CRC error drop one byte and search again; count sequence gaps and lost frames; ignore unknown message types; numerics and ECG each reset their own 3 s watchdog, a heartbeat does not.

# 7. Building

## 7.1 With Qt Creator

1. Open the top-level `CMakeLists.txt`.
2. Select the kit "Desktop Qt 6.12.0 MinGW 64-bit" (or another Qt 6.8+ kit). The presets that Qt Creator offers from CMakePresets.json are not needed.
3. Build with Ctrl+B. Executables go to `<build>/bin`.
4. Choose the program to run in the kit selector (Run column): VitalSignsPatientSimulator, VitalSignsMonitor or VitalSignsSimulator.

> To run the simulator and the monitor together from Qt Creator, set Preferences > Build & Run > General > "Stop applications before building" to "Same Application". With the default value Qt Creator stops the simulator when it builds the monitor.

## 7.2 From the command line (Git Bash, MinGW)

```
export QTDIR=C:/Qt/6.12.0/mingw_64
export PATH="$QTDIR/bin:/c/Qt/Tools/mingw1310_64/bin:/c/Qt/Tools/Ninja:/c/Qt/Tools/CMake_64/bin:$PATH"
cmake --preset mingw-debug
cmake --build --preset mingw-debug
ctest --preset mingw-debug
```

Presets: `mingw-debug` (Ninja, Windows), `windows-msvc` with build presets `windows-debug` / `windows-release` (Visual Studio 2022), `linux-debug` (Ninja, Linux). All use the QTDIR environment variable as CMAKE_PREFIX_PATH.

## 7.3 CMake options

| Option | Default | Effect |
|---|---|---|
| VSM_BUILD_TESTS | ON | Build the GoogleTest suites (downloads GoogleTest at configure time) |
| VSM_BUILD_SIMULATOR | ON | Build the simulator engine and both simulator programs |
| VSM_WARNINGS_AS_ERRORS | OFF | Add -Werror or /WX to the project's own targets |

The warning policy is in `cmake/CompilerWarnings.cmake`: GCC/Clang use -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Wconversion -Wsign-conversion -Wold-style-cast -Woverloaded-virtual; MSVC uses /W4 /permissive- /utf-8 /Zc:__cplusplus. Third-party code is never compiled with these flags.

# 8. Testing

## 8.1 Running the tests

Run `ctest --preset mingw-debug` (or "Run All Tests" in Qt Creator). All 140 tests must pass before a change is committed.

| Test executable | Suite | Tests | What it proves |
|---|---|---|---|
| vsm_unit_tests | Crc16 | 3 | Standard check value, empty input, single bit flip detected |
| vsm_unit_tests | FrameCodec | 3 | Little-endian header and CRC, empty payload, oversized payload rejected |
| vsm_unit_tests | FrameDecoder | 10 | Several frames per chunk, byte-by-byte input, garbage, bad CRC, bad version, oversized length, bounded memory, reset |
| vsm_unit_tests | Messages | 9 | Round trips of every message type, invalid values and flags, wrong payload sizes, unknown type ignored |
| vsm_unit_tests | DefaultAlarmsTest, AlarmEvaluator | 11 | Exclusive limits, critical replaces medium, sorting, missing values, sensor alarms, link-loss suppression, custom rules |
| vsm_unit_tests | AlarmLimits | 3 | Defaults valid, ordering of heart-rate and SpO2 limits |
| vsm_unit_tests | SweepBuffer | 4 | Empty start, wrap-around, clear, zero capacity rejected |
| vsm_unit_tests | FrameDecoderFuzz | 4 | Random bytes, random chunks and flipped bytes (fixed seeds): no crash, bounded memory, never a wrong frame |
| vsm_unit_tests | AlarmManagerTest, AuditEvents | 26 | Latching, acknowledge, audio pause and its early end, sorting, sound priority, audit entries, short critical episode |
| vsm_unit_tests | AlarmTone, Wav, AlarmToneByPriority | 16 | Pulse count, length, loopable end, no clipping, WAV header and byte order |
| vsm_unit_tests | AuditFormat, FileAuditLogTest | 12 | UTC time, CSV quoting, header once, immediate flush, append, unwritable file |
| vsm_device_tests | DeviceSessionTest | 18 | Session with FakeTransport: vitals mapping, ECG units, split frames, gaps and CRC errors, wrap-around, watchdogs for numerics and ECG, impossible values, reconnect |
| vsm_app_tests | MonitorViewModelTest | 10 | Real view model and alarm chain with a fake transport and sound: stale data cleared, sound, latching, audio pause, disconnect, audit |
| vsm_simulation_tests | DeviceSimulatorTest | 10 | Engine with FakeSink and a fake clock: sample rate, numerics, gradual HR, sensors off, cable, corruption, scenarios, power off |
| CTest | traceability_check | 1 | docs/medical matches the code and the tests |

## 8.2 Test doubles

- `FakeTransport` (tests/device/FakeTransport.h) replaces the socket: a test pushes bytes and state changes and checks what the session emits.
- `FakeSink` (in DeviceSimulatorTest.cpp) collects the bytes the simulator sends.
- A fake clock (a lambda returning a variable) drives `DeviceSimulator::tick()`, so a 21-second scenario test runs in milliseconds without sleeping.
- The device and simulation tests share `tests/device/main.cpp`, which creates the QCoreApplication that queued signals and timers need. Signals are counted with lambdas instead of QSignalSpy, so no QtTest dependency is needed.

## 8.3 Writing a new test

1. Add a TEST() or TEST_F() to the matching file in `tests/` (or a new file listed in tests/CMakeLists.txt).
2. Use fakes for hardware and time; never sleep in a test.
3. Name the test after the behaviour, for example `ReportsLinkLostAfterWatchdogTimeout`.
4. Build and run ctest; the new test is discovered automatically (gtest_discover_tests).

## 8.4 Continuous integration

`.github/workflows/ci.yml` builds everything and runs all tests on Windows (MinGW) and Linux (GCC) with warnings as errors. It uses Qt 6.8.3, the oldest supported version, so developing on a newer Qt cannot silently raise the minimum. It runs on every pull request and every push to main. MSVC is not covered yet.

The QML files have no automated tests; check them by hand with the simulator (User Guide, section 7).

## 8.5 Medical-style documents

`docs/medical/` holds software requirements, a risk analysis and a traceability matrix, in the style of IEC 62304 and ISO 14971. They are a learning exercise and claim no compliance. When you change behaviour, update them: every requirement names its code and tests, and `tools/check-traceability.ps1` (CTest test `traceability_check`) fails on a broken link. It checks that links exist, not that a test really proves its requirement.

# 9. Extending the system

## 9.1 Add an alarm rule

1. If a limit or a simple condition is enough, add a LimitAlarmRule or ConditionAlarmRule in `makeDefaultAlarmRules()` (src/core/AlarmRules.cpp) with a new stable code.
2. For other logic, write a class that implements IAlarmRule::evaluate().
3. If the rule has a new source, extend AlarmSource, so that "one alarm per source" still works.
4. Add tests to AlarmEvaluatorTest and update the alarm table in docs/ARCHITECTURE.md and the User Guide.

## 9.2 Add a transport (for example Bluetooth)

1. Implement ITransport (open, close, state, description and the three signals).
2. Extend ConnectionSettings and createDefaultTransport().
3. Add the settings to ConnectionBar.qml and a connect method to MonitorViewModel.
4. DeviceSession does not change - test the new transport against it like FakeTransport.

## 9.3 Add a simulator output

1. Implement IByteSink. write() must never block; drop data if the receiver cannot keep up.
2. Extend OutputSettings and createDefaultSink().
3. Add a key to OutputPanel.qml and an open method to PatientSimulatorController.

## 9.4 Add a scenario

Add a name to `scenarioNames()` and the phases to `makeScenario()` in tools/simulator/engine/Scenario.cpp. Add a key label in DeviceCase.qml (scenarioKeyLabels) if the default upper-case name is too long, and a test in DeviceSimulatorTest.

## 9.5 Add a message type

1. Choose a new type code and document the payload in docs/PROTOCOL.md.
2. Add a struct to Messages.h, add it to the Message variant, and extend toFrame() and parseMessage().
3. Send it from DeviceSimulator and handle it in DeviceSession.
4. Add MessagesTest cases. Older monitors ignore the new type (rule 5 of the receiver).

# 10. Coding rules

- Keep the layering: protocol and core stay Qt-free; core never includes protocol.
- Wire concrete types only in the main.cpp files.
- Classes registered with QML_ELEMENT must not be `final` (Qt derives from them).
- Do not mark Q_INVOKABLE functions `static`; avoid `[[nodiscard]]` on them as well.
- Every new logic class gets GoogleTest tests; the build must stay warning-free.
- Public types get Doxygen-style `///` comments; comments explain why, not what.
- Medical standards are referenced only "in the style of"; never claim compliance. The simulator must keep its "SYNTHETIC SIGNALS ONLY - NOT A MEDICAL DEVICE" label.

# 11. Debugging and troubleshooting

| Problem | Cause | Solution |
|---|---|---|
| CMake: Qt6 SerialPort not found | The Qt Serial Port module is not installed for the kit | Install it with the Qt MaintenanceTool (Qt 6.12.0 > Additional Libraries > Qt Serial Port) |
| The simulator stops when the monitor is started from Qt Creator | Qt Creator stops applications of the same project before building | Set "Stop applications before building" to "Same Application" |
| "QML debugging is enabled" in the output | Normal message of a debug build | No action |
| Monitor shows "Connecting ..." or "retrying" | Simulator not running, wrong port, or output closed | Start the simulator, check the port (default 5555), press OPEN on the simulator |
| Too much or too little log output | All vsm.* logging categories are on by default | Filter them with QT_LOGGING_RULES, for example "vsm.device.session.info=false" |
| A program does not start outside Qt Creator | Qt DLLs not found | Put the Qt bin folder on PATH or run windeployqt |

# 12. Version control

- Do not commit build folders, `CMakeLists.txt.user` or `.qtcreator/` (see .gitignore).
- One logical change per commit; the message says what changed and why.
- Before a commit: build without warnings and run all tests.

# 13. Regenerating these documents

The Word documents in `Documents/EN` and `Documents/FA` are generated from Markdown-like sources in `Documents/source/en` and `Documents/source/fa` with Microsoft Word:

```
powershell -ExecutionPolicy Bypass -File Documents\source\build-docs.ps1
powershell -ExecutionPolicy Bypass -File Documents\source\build-docs.ps1 -Only en\02-Developer-Guide.md
```

Edit the source, run the script and commit both the source and the .docx file.
