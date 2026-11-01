# CLAUDE.md - VitalSigns Monitor

Portfolio/learning project (track 1 of the 12-week learning plan). C++20, Qt 6.8+ (built with
Qt 6.12.0 mingw_64 since 31 Oct 2026; the MSVC kit is no longer installed), CMake, GoogleTest.
Synthetic data only - never add real patient data.

## Build & test (this machine, Git Bash)

```bash
export PATH="/c/Qt/6.12.0/mingw_64/bin:/c/Qt/Tools/mingw1310_64/bin:/c/Qt/Tools/Ninja:/c/Qt/Tools/CMake_64/bin:$PATH"
cmake -S . -B build/mingw-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64
cmake --build build/mingw-debug --parallel
ctest --test-dir build/mingw-debug --output-on-failure
```

Qt Creator uses its own folder `build/Desktop_Qt_6_12_0_MinGW_64_bit_Debug` - do not delete it.

Run (Qt `bin` on PATH): `build/mingw-debug/bin/VitalSignsPatientSimulator.exe` (GUI, TCP 5555 at
start-up) or `VitalSignsSimulator.exe` (console), then `VitalSignsMonitor.exe --connect 127.0.0.1:5555`.

## Layout

- `src/protocol`, `src/core` - pure C++20; `src/audit` - audit CSV file (C++20, no Qt); `src/device` -
  Qt I/O; `src/ui` - shared QML module `VitalSigns.Ui` (waveform); `src/app` - monitor.
- `.github/workflows/ci.yml` - Windows (MinGW) and Linux with Qt 6.8.3, warnings as errors.
- `tools/simulator/engine` (`vsm_simulation` library), `cli` (console), `gui` (front-panel QML).
- `Documents/` - Word documents (RFP, Developer, User, Installation guides) in `EN/` and `FA/`,
  generated from `Documents/source/{en,fa}/*.md` by `Documents/source/build-docs.ps1` (Word COM).
  Edit the sources, never the .docx; keep EN and FA in step and the facts equal to the code.

## Rules

- Keep the layering: `protocol` and `core` stay Qt-free; `core` never includes `protocol`.
- Alarm logic takes "now" as a parameter and reports through `IAuditLog`; it never reads a clock or a file.
- Every new alarm or operator event must reach the audit log; audit entries never hold patient data.
- New alarm -> new `IAlarmRule`; new transport -> new `ITransport`; new simulator output -> new
  `IByteSink`; wire concrete types only in the `main.cpp` files.
- Classes registered with `QML_ELEMENT` must not be `final`; `Q_INVOKABLE` functions must not be
  `static` or `[[nodiscard]]`.
- Apps using `VitalSigns.Ui` link `vsm_uiplugin` and call `Q_IMPORT_QML_PLUGIN(VitalSigns_UiPlugin)`.
- Every new logic class gets GoogleTest tests; build must stay warning-free (GCC `-Wall -Wextra
  -Wconversion ...`, MSVC `/W4`).
- Doxygen-style `///` comments on public types; comments explain *why*, not *what*.
- Medical standards (IEC 62304, ISO 14971, IEC 60601-1-8) are referenced only as "in the style of" /
  learning exercises - never claim compliance. The simulator GUI must keep its
  "SYNTHETIC SIGNALS ONLY - NOT A MEDICAL DEVICE" label.
