# Learning log

Short daily notes: what I built, what I learned, what I would answer in an interview.

## 2026-10-31 - Project start (week 2, day 1)

**Built**
- CMake project with four layers (protocol, core, device, app), a simulator and GoogleTest tests.
- Binary protocol with CRC-16 and a streaming decoder; device I/O in its own `QThread`.
- First Qt Quick UI: ECG sweep (scene graph), HR/SpO2 tiles, alarm banner.

**Learned / to review**
- Why `moveToThread` + queued signals instead of sharing data with a mutex.
- Why a QML element class must not be `final` (`QQmlElement<T>` derives from it).
- Qt Quick Layouts: a nested layout has `fillWidth: true` by default.
- `QSGGeometry::allocate()` discards old vertex data - count first, then write.

**Interview questions this answers**
- How do you keep a UI responsive while reading a serial port?
- How does a receiver recover from a corrupted byte stream?
- How do you unit-test code that talks to hardware? (interface + fake transport)

## 2026-10-31 - Patient simulator with a front panel (week 2, day 1, part 2)

**Built**
- Switched the build to Qt 6.12 MinGW (added the `mingw-debug` preset); clean with GCC warnings on.
- Split the simulator into a library (`vsm_simulation`) used by a console program and a new Qt Quick
  GUI that looks like a bench-top patient simulator: LCD, knobs, keys, LEDs, serial/TCP output,
  fault injection (unplugged cable, corrupted frames).
- Moved the waveform drawing into a shared static QML module (`VitalSigns.Ui`).
- 10 engine tests with a fake clock and an in-memory sink (63 tests in total).
- Found during the end-to-end test: the monitor's "Lost frames" counted *gaps*, not frames. It now
  shows both (`framesLost`, `sequenceGaps`).

**Learned / to review**
- A static QML module needs `Q_IMPORT_QML_PLUGIN(<Uri>Plugin)` and the `<target>plugin` link.
- `QT_QML_OUTPUT_DIRECTORY` must end in the URI path, or qmllint/Qt Creator cannot find modules.
- moc cannot handle `static` or `[[nodiscard]]` on `Q_INVOKABLE` functions - keep them plain members.
- Injecting a clock (`std::function<qint64()>`) makes timing code testable without sleeping.

**Interview questions this answers**
- How do you test a monitor against hardware faults you cannot easily reproduce? (simulator with fault injection)
- How do you detect lost data on a serial link? (sequence numbers + CRC + watchdog)

## 2026-11-01 - Alarm manager, audit log, CI (week 3)

**Built**
- `AlarmManager`: latching, acknowledge, audio pause. Pure C++, "now" comes in as a parameter.
- Audit log: `IAuditLog` in core, `FileAuditLog` (CSV), `AuditLogModel` as decorator for the EVENTS window.
- Alarm melodies generated in code (`makeAlarmTone`, `encodeWav`), played with `QSoundEffect`.
- Randomised decoder tests with fixed seeds, and a GitHub Actions workflow. 121 tests in total.
- The end-to-end run found a real bug: an operator's "resume audio" was logged as `system`.

**Learned / to review**
- Evaluator vs manager: "what is true now" is stateless, "what has the operator seen" is state.
- Why only high priority latches, and why an audio pause must not be possible with no alarm present.
- `QSoundEffect` plays WAV only, so generated tones go through a temporary file.
- A flashing banner must not flash its own keys: the colour needs its own layer.
- Windows exit code 0xc0000139 means a DLL lacks an entry point, usually the wrong one found via PATH.
  In CI, Git Bash put Git's libstdc++-6.dll ahead of the compiler's; the first CI run found this.

**Interview questions this answers**
- Latched vs non-latched alarm, and acknowledge vs audio pause?
- How do you test time-dependent logic without sleeping?
- What belongs in an audit trail, and what must never be in it?

## 2026-11-02 - Risk analysis, requirements, traceability (week 4)

**Built**
- `docs/medical`: requirements, risk table and traceability matrix, in the style of IEC 62304 and ISO 14971.
- `tools/check-traceability.ps1`, run by CTest and CI: broken links between documents, code and tests fail the build.
- `vsm_app_tests`: the real view model and alarm chain with a fake transport and sound. 140 tests in total.
- Fixes the analysis led to: heartbeats no longer keep the link alive, impossible values become "---",
  the ECG clears on link loss.

**Learned / to review**
- Doing the risk table against the running code found bugs that 121 passing tests had missed. The tests
  proved what the code did, not that it should do it: one even asserted the heartbeat behaviour.
- A CRC proves the bytes arrived unchanged, not that the value makes sense.
- A control can create a new risk: the stricter watchdog can raise a false "no data" on a bad link (R-22).
- Severity stays; software controls lower the probability. Open risks stay visible instead of being hidden.
- Linking a test to a requirement does not prove the test is good; the checker only sees that links exist.

**Interview questions this answers**
- Hazard, hazardous situation and harm: what is the difference? What is residual risk?
- What is SOUP, and what is software safety class C?
- What does a traceability matrix give you, and what can it not tell you?
