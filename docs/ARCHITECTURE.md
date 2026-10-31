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
| app        | `src/app`         | ui, device, core, Qt Quick | `main.cpp` (composition root), `MonitorViewModel`, `AlarmListModel`, QML |
| ui         | `src/ui`          | core, Qt Quick             | QML module `VitalSigns.Ui`: `WaveformSeries`, `WaveformItem` (shared by monitor and simulator GUI) |
| device     | `src/device`      | protocol, core, Qt Network/SerialPort | `ITransport`, `TcpTransport`, `SerialTransport`, `DeviceSession`, `DeviceController` |
| core       | `src/core`        | C++ standard library only  | `VitalSigns`, `Alarm`, `AlarmLimits`, `IAlarmRule`, rules, `AlarmEvaluator`, `SweepBuffer` |
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
     -> linkLostChanged / statistics      ==queued==>         -> AlarmEvaluator -> AlarmListModel
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
| **D**ependency inversion | `DeviceSession` depends on `ITransport`; `DeviceController` gets a `TransportFactory`; `MonitorViewModel` gets its `AlarmEvaluator` injected; `DeviceSimulator` writes to `IByteSink` and reads an injected clock; `PatientSimulatorController` gets a `SinkFactory`; only the `main.cpp` files know the concrete types. |

## Patterns used

- **Strategy** - `IAlarmRule` (`LimitAlarmRule`, `ConditionAlarmRule`).
- **Facade** - `DeviceController` hides the thread and the session from the UI.
- **Factory** - `TransportFactory` / `createDefaultTransport()`, `SinkFactory` / `createDefaultSink()`,
  `makeDefaultAlarmRules()`, `makeScenario()`.
- **MVVM** - QML views bind to `MonitorViewModel`, `AlarmListModel`, `WaveformSeries`,
  `PatientSimulatorController`.
- **Composition root** - `src/app/main.cpp` and `tools/simulator/gui/main.cpp` build the object graphs.

## Alarm model (v0.1)

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
certified implementation. Latching, acknowledging/silencing and audio signals are planned for week 3.

## Known limitations

- Line width above 1 px only on the OpenGL backend (Direct3D/Vulkan draw 1 px). A triangle-strip
  renderer would fix this.
- One device per monitor window.
- Settings (limits, last connection) are not persisted yet.
