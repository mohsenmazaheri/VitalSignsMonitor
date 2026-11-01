# Architecture

## Goals

1. **Never block the UI.** Device I/O and parsing run in their own thread.
2. **Testable safety logic.** The alarm logic and the protocol are plain C++20 without Qt, so they
   can be unit-tested in milliseconds.
3. **Replaceable edges.** TCP, serial or a test fake behind one interface; alarm rules pluggable.
4. **Honest data.** A missing value is shown as `---`, never as an old or made-up number.

## Layers

| Layer      | Folder            | Depends on                 | Contains |
|------------|-------------------|----------------------------|----------|
| app        | `src/app`         | ui, device, audit, core, Qt Quick, Qt Multimedia | `main.cpp` (composition root), `MonitorViewModel`, `AlarmListModel`, `AuditLogModel`, `IAlarmSound`, `QtAlarmSound`, QML |
| audit      | `src/audit`       | core, C++ standard library | `FileAuditLog`: the audit trail as a CSV file |
| ui         | `src/ui`          | core, Qt Quick             | QML module `VitalSigns.Ui`: `WaveformSeries`, `WaveformItem` (shared by monitor and simulator GUI) |
| device     | `src/device`      | protocol, core, Qt Network/SerialPort | `ITransport`, `TcpTransport`, `SerialTransport`, `DeviceSession`, `DeviceController` |
| core       | `src/core`        | C++ standard library only  | `VitalSigns`, `Alarm`, `AlarmLimits`, `IAlarmRule`, rules, `AlarmEvaluator`, `AlarmManager`, `AlarmTone`, `IAuditLog`, `SweepBuffer` |
| protocol   | `src/protocol`    | C++ standard library only  | `Frame`, `FrameDecoder`, `Messages`, CRC-16 |
| simulation | `tools/simulator/engine` | protocol, Qt Network/SerialPort | `SyntheticPatient`, `Scenario`, `DeviceSimulator`, `IByteSink`, `TcpBroadcastSink`, `SerialSink`, `SinkFactory` |
| simulator apps | `tools/simulator/cli`, `tools/simulator/gui` | simulation (+ ui for the GUI) | console `main.cpp`; GUI `PatientSimulatorController` + front-panel QML |

`core` does not know `protocol`: the device layer maps wire messages (`NumericsMessage`) to domain
objects (`VitalSigns`). The wire format can change without touching the alarm logic.

## Data flow and threads

```
I/O thread                                             GUI thread
----------                                             ----------
QTcpSocket / QSerialPort
  -> ITransport::dataReceived(bytes)
  -> DeviceSession: FrameDecoder -> parseMessage
     -> vitalsReceived(VitalSigns)        ==queued==>  DeviceController (relay)
     -> ecgSamplesReceived(mV, rate)      ==queued==>    -> MonitorViewModel
     -> linkLostChanged / statistics      ==queued==>         -> AlarmEvaluator -> AlarmManager -> AlarmListModel, IAlarmSound, IAuditLog
                                                              -> WaveformSeries -> WaveformItem (scene graph)
                                                              -> QML properties
```

- `DeviceController` lives in the GUI thread and owns a `QThread`. For each connection it creates a
  `DeviceSession`, moves it into the I/O thread and starts it with a queued call.
- The transport is a child of the session, so the socket moves to the I/O thread with it.
- Disconnect: the session is stopped **inside** the I/O thread (sockets and timers must be used from
  their own thread), moved back and deleted in the GUI thread - with a blocking queued call, which is
  safe because the I/O thread never waits for the GUI.
- Cross-thread communication is only through queued signals carrying value types
  (`VitalSigns`, `QList<float>`, `LinkStatistics`); there is no shared mutable state and no mutex.
- `WaveformItem::updatePaintNode()` runs on the render thread while the GUI thread is blocked, so
  reading `WaveformSeries` there is safe without locking.

## Simulator

```
PatientSimulatorController (QML view model)       VitalSignsSimulator main()
          |  knobs, keys, output choice                     |  command line
          v                                                 v
   DeviceSimulator  --- every 40 ms: tick() ---------------------------------+
     - Scenario (scripted phases) or manual targets                         |
     - SyntheticPatient: ECG samples + gradual HR/SpO2                      |
     - fault switches: cable unplugged, corruption probability             |
     - protocol::encodeFrame(...)                                           |
          |                                                                 |
          v                                                                 |
   IByteSink  <- SinkFactory creates TcpBroadcastSink or SerialSink          |
```

- The engine (`vsm_simulation`) is a library: the console program, the GUI and the tests use the
  same code, so what the tests prove is what the GUI sends.
- `DeviceSimulator` takes an injectable clock. Tests drive `tick()` with a fake clock and an
  in-memory sink, so a 21-second scenario test runs in milliseconds.
- **Unplugged cable:** frames are still numbered, but not sent - exactly what happens when a real
  cable is pulled. The monitor reports "No data from device", then a sequence gap.
- Changing any patient control while a scenario runs switches to manual mode and holds the new
  targets, as bench-top patient simulators do.
- The front panel is laid out at a fixed design size and scaled as a whole (`DeviceCase` with
  `scale`), so it keeps the proportions of an instrument at any window size.

## SOLID in this code base

| Principle | Where |
|-----------|-------|
| **S**ingle responsibility | `FrameDecoder` only frames bytes; `parseMessage` only decodes payloads; `DeviceSession` only runs one link; `AlarmEvaluator` only evaluates and ranks; `WaveformItem` only draws. |
| **O**pen/closed | New alarm = new `IAlarmRule`; new transport = new `ITransport`; new simulator scenario = new data in `makeScenario()`. Nothing else changes. |
| **L**iskov substitution | `FakeTransport`, `TcpTransport` and `SerialTransport` are interchangeable for `DeviceSession` (the unit tests prove it). |
| **I**nterface segregation | `ITransport` has four methods and three signals - only what a session needs. `IByteSink` (simulator) is separate from `ITransport` (monitor). |
| **D**ependency inversion | `DeviceSession` depends on `ITransport`; `DeviceController` gets a `TransportFactory`; `MonitorViewModel` gets `AlarmEvaluator`, `AlarmManager` and `IAlarmSound` injected; `AlarmManager` writes to `IAuditLog` and never sees a file or a clock; `DeviceSimulator` writes to `IByteSink` and reads an injected clock; `PatientSimulatorController` gets a `SinkFactory`; only the `main.cpp` files know the concrete types. |

## Patterns used

- **Strategy** - `IAlarmRule` (`LimitAlarmRule`, `ConditionAlarmRule`).
- **Facade** - `DeviceController` hides the thread and the session from the UI.
- **Decorator** - `AuditLogModel` shows each event on screen and passes it on to the real `IAuditLog`.
- **State machine** - `AlarmManager`: Active, Acknowledged, Latched.
- **Factory** - `TransportFactory` / `createDefaultTransport()`, `SinkFactory` / `createDefaultSink()`,
  `makeDefaultAlarmRules()`, `makeScenario()`.
- **MVVM** - QML views bind to `MonitorViewModel`, `AlarmListModel`, `WaveformSeries`,
  `PatientSimulatorController`.
- **Composition root** - `src/app/main.cpp` and `tools/simulator/gui/main.cpp` build the object graphs.

## Alarm model (v0.2)

| Code | Condition (default limits) | Priority | Category |
|------|----------------------------|----------|----------|
| `HR_CRITICAL_HIGH` | HR > 150 bpm | High | Physiological |
| `HR_HIGH` | HR > 120 bpm | Medium | Physiological |
| `HR_LOW` | HR < 50 bpm | Medium | Physiological |
| `HR_CRITICAL_LOW` | HR < 40 bpm | High | Physiological |
| `SPO2_LOW` | SpO2 < 90 % | Medium | Physiological |
| `SPO2_CRITICAL_LOW` | SpO2 < 85 % | High | Physiological |
| `ECG_LEADS_OFF` | leads-off flag | Low | Technical |
| `SPO2_PROBE_OFF` | probe-off flag | Low | Technical |
| `DEVICE_LINK_LOST` | no valid frame for 3 s | Medium | Technical |

Only the highest-priority alarm per source is reported. When the link is lost the numerics are
cleared (shown as `---`) and the sensor alarms are suppressed, because their flags are stale.

The three priority levels follow the idea of IEC 60601-1-8; this is a learning model, not a
certified implementation.

### Alarm life cycle (`AlarmManager`)

```
              condition starts
                     |
                     v
   +------------- ACTIVE (visible, audible) ------------------+
   | condition ends                                           | ACKNOWLEDGE
   | high priority: latch                                     v
   |  medium/low: gone                               ACKNOWLEDGED (visible, silent)
   v                                                          | condition ends: gone
LATCHED (visible, audible)  -- ACKNOWLEDGE: gone
   |
   +-- condition returns: ACTIVE
```

- **Why latch only high priority:** a one-second critical event must not vanish unseen, but technical
  and medium alarms would otherwise pile up.
- **AUDIO PAUSE** silences for 2 minutes and is logged. A new medium or high alarm ends it at once, a
  new low alarm does not. It is refused when nothing is unacknowledged, because a pause "in advance"
  would hide the next alarm. The visual signals keep running during the pause.
- The tiles show the present: a latched alarm colours the banner, not the number below it.
- Disconnecting clears all alarms, also latched ones, because the next session may be another patient.
- `AlarmManager` receives "now" as a parameter, so tests run two minutes of alarm time in microseconds.

### Sound

`core::makeAlarmTone()` generates one repetition per priority (high 10 pulses every 6 s, medium 3
every 8 s, low 2 every 20 s; C5 with harmonics). `QtAlarmSound` writes them as WAV files to a temporary
folder and loops them with `QSoundEffect`. The pulse pattern follows the idea of IEC 60601-1-8 but was
not measured against it. Without an audio device the monitor stays silent and logs a warning.

## Audit log

`AlarmManager` and `MonitorViewModel` report to `core::IAuditLog`. `AuditLogModel` keeps the latest 500
entries for the **EVENTS** window and passes each entry to `FileAuditLog`, which appends one flushed CSV
line per event: `time_utc,event,actor,code,priority,detail`. `actor` tells `operator` from `system`.
The file holds no patient data. The log is a learning model: it is not protected against editing.

## Continuous integration

`.github/workflows/ci.yml` builds everything and runs all tests with warnings as errors on Windows
(MinGW) and Linux (GCC), with Qt 6.8.3, the oldest supported version.

## Known limitations

- Line width above 1 px only on the OpenGL backend (Direct3D/Vulkan draw 1 px). A triangle-strip
  renderer would fix this.
- One device per monitor window.
- Settings (limits, last connection) are not persisted yet; alarm limits cannot be changed in the UI.
- No hysteresis: a value moving around a limit raises and ends the alarm repeatedly.
- A write error after the audit file was opened is not shown on screen; only a failed open is.
- MSVC is not covered by CI, and the view models have no automated tests (checked by hand with the
  simulator).
