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
