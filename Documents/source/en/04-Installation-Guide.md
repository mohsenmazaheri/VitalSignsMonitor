---
title: Installation and Setup Guide
subtitle: VitalSigns Monitor - tools, build, first start, deployment and virtual serial ports
docid: VSM-INS-001
version: 1.1
date: 1 November 2026
author: Mohsen Mazaheri
status: Draft for review
lang: en
output: EN/VitalSignsMonitor-04-Installation-Guide-EN.docx
header: VitalSigns Monitor · Installation and Setup Guide
history: 1.0 | 1 Nov 2026 | Mohsen Mazaheri | First issue, based on software version 0.1.0; 1.1 | 1 Nov 2026 | Mohsen Mazaheri | Alarm manager, audit log, alarm sounds and CI (software version 0.2.0)
notice: **Synthetic data only - not a medical device.** Install the programs only for learning, demonstration and software testing.
---

# 1. Introduction

This guide explains how to install the development tools, build VitalSigns Monitor from source, run it for the first time, and copy it to another Windows computer. It also covers virtual serial ports for testing without hardware.

| Section | For whom |
|---|---|
| 2 to 8 | Everybody who builds the programs from source (Windows) |
| 9 | Copying the finished programs to a computer without Qt |
| 10 | Testing the serial connection without hardware |
| 11 | Linux |
| 13 | Problems during installation |

Verified configuration: Windows 11 Pro 64-bit, Qt 6.12.0 MinGW 64-bit, MinGW 13.1, CMake from the Qt installation, Ninja, Qt Creator. Steps marked "not yet verified" follow the standard Qt procedure but have not been tested on a clean computer.

# 2. System requirements

| Item | Minimum / recommended |
|---|---|
| Operating system | Windows 10 or 11, 64-bit (verified on Windows 11). Linux 64-bit: see section 11 |
| Processor | x86-64, 2 cores or more (4+ recommended for building) |
| Memory | 8 GB recommended for building; the programs themselves need little memory |
| Disk space | Up to about 15 GB for the Qt 6.12.0 MinGW kit with all optional parts (14 GB measured with debug information files), about 0.7 GB for MinGW, and 250 to 400 MB for each debug build folder |
| Display | 1280 x 800 or more (the simulator panel is designed at 1200 x 770 and scales) |
| Graphics | OpenGL, Direct3D 11 or Vulkan capable driver (used by Qt Quick) |
| Network | Internet access for the Qt installer and for the first CMake run (GoogleTest download) |

# 3. Software prerequisites

| Software | Version | Purpose | Source |
|---|---|---|---|
| Qt (open-source) | 6.8 or newer; verified with 6.12.0 | Framework: Core, Gui, Multimedia, Network, Serial Port, Quick, Quick Controls 2 | Qt Online Installer (free Qt account) |
| MinGW | 13.1 (64-bit) | C++20 compiler (g++) | Qt installer, Developer and Designer Tools |
| CMake | 3.25 or newer | Build system | Qt installer or cmake.org |
| Ninja | any recent | Fast build tool | Qt installer |
| Qt Creator | any recent | IDE (optional but recommended) | Qt installer |
| Git | any recent | Get the source code | git-scm.com |
| GoogleTest | 1.15.2 | Unit tests | Downloaded automatically by CMake |
| com0com | optional | Virtual COM-port pair for serial tests | SourceForge project "com0com" |

Alternative: Visual Studio 2022 with an msvc2022_64 Qt kit (preset windows-msvc). Not verified on this machine.

# 4. Installing Qt and the tools

1. Download the Qt Online Installer from qt.io (open-source version) and sign in with a free Qt account.
2. Choose "Custom installation" and select the components below.
3. Finish the installation (default folder C:\Qt).

| Component (installer tree) | Needed |
|---|---|
| Qt > Qt 6.12.0 > MinGW 13.1.0 64-bit | Yes |
| Qt > Qt 6.12.0 > Additional Libraries > Qt Serial Port | Yes - without it CMake stops with "Qt6SerialPort not found" |
| Qt > Qt 6.12.0 > Additional Libraries > Qt Multimedia | Yes - needed for the alarm sounds; without it CMake stops with "Qt6Multimedia not found" |
| Qt > Developer and Designer Tools > MinGW 13.1.0 64-bit | Yes |
| Qt > Developer and Designer Tools > CMake, Ninja | Yes |
| Qt > Developer and Designer Tools > Qt Creator | Recommended |
| Qt > Qt 6.12.0 > Sources, Qt Debug Information Files | Optional (for debugging into Qt) |

> If Qt is already installed, add missing parts with `C:\Qt\MaintenanceTool.exe` > Add or remove components.

After installation the most important folders are:

| Folder | Content |
|---|---|
| C:\Qt\6.12.0\mingw_64 | Qt libraries, bin folder with DLLs and windeployqt.exe |
| C:\Qt\Tools\mingw1310_64\bin | g++, gdb and the MinGW runtime DLLs |
| C:\Qt\Tools\CMake_64\bin | cmake.exe, ctest.exe |
| C:\Qt\Tools\Ninja | ninja.exe |

# 5. Getting the source code

Clone the repository or copy the project folder:

```
git clone https://github.com/mohsenmazaheri/VitalSignsMonitor.git
```

Use a path you can write to. Paths with spaces work (the project is developed in "D:\Projects\Vital Signs Monitor").

# 6. Building with Qt Creator

1. Start Qt Creator and choose File > Open File or Project, then select the top-level `CMakeLists.txt`.
2. In "Configure Project" select the kit **Desktop Qt 6.12.0 MinGW 64-bit** and click **Configure Project**. Qt Creator may also offer kits from CMakePresets.json (mingw-debug, windows-msvc); they are not needed.
3. Wait until CMake has finished. The first run downloads GoogleTest.
4. Build with **Build > Build All Projects** (Ctrl+Shift+B).
5. Choose the program in the kit selector at the bottom left (Run column): **VitalSignsPatientSimulator** first, then run it with Ctrl+R.
6. Choose **VitalSignsMonitor** and run it as well.

> **Important setting:** by default Qt Creator stops all running programs of the same project before it builds. Then the simulator closes when you start the monitor. Change Edit > Preferences > Build & Run > General > **Stop applications before building** to **Same Application**. Alternatively start the second program with Build > Run Without Deployment.

The executables are written to `build\Desktop_Qt_6_12_0_MinGW_64_bit_Debug\bin`.

# 7. Building from the command line

Git Bash:

```
export QTDIR=C:/Qt/6.12.0/mingw_64
export PATH="$QTDIR/bin:/c/Qt/Tools/mingw1310_64/bin:/c/Qt/Tools/Ninja:/c/Qt/Tools/CMake_64/bin:$PATH"
cmake --preset mingw-debug
cmake --build --preset mingw-debug
```

PowerShell:

```
$env:QTDIR = "C:/Qt/6.12.0/mingw_64"
$env:PATH = "C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\CMake_64\bin;$env:PATH"
cmake --preset mingw-debug
cmake --build --preset mingw-debug
```

The programs are in `build\mingw-debug\bin`. Without presets:

```
cmake -S . -B build/mingw-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64
cmake --build build/mingw-debug --parallel
```

Build options: `-DVSM_BUILD_TESTS=OFF` skips the tests (and the GoogleTest download), `-DVSM_BUILD_SIMULATOR=OFF` skips the simulators, `-DVSM_WARNINGS_AS_ERRORS=ON` turns warnings into errors.

# 8. Checking the installation

## 8.1 Unit tests

```
ctest --preset mingw-debug
```

Expected result: "100% tests passed, 0 tests failed out of 121". In Qt Creator: Tools > Tests > Run All Tests.

## 8.2 First start

1. Start `VitalSignsPatientSimulator.exe`. The front panel appears, the POWER LED is on and the LCD shows an ECG.
2. Start `VitalSignsMonitor.exe` and click **Connect** (TCP, 127.0.0.1, 5555).
3. The ECG sweeps, HR and SpO2 appear, the simulator's LINK LED lights up.

> Outside Qt Creator the Qt folder `C:\Qt\6.12.0\mingw_64\bin` must be on PATH, otherwise Windows reports a missing Qt6Core.dll. For other computers see section 9.

Command-line shortcut: `VitalSignsMonitor --connect 127.0.0.1:5555`.

# 9. Deploying to another Windows computer

Not yet verified on a clean computer. The steps follow the standard Qt deployment with windeployqt.

1. Build a release version:

```
cmake -S . -B build/mingw-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DVSM_BUILD_TESTS=OFF
cmake --build build/mingw-release --parallel
```

2. Create a folder and copy the executables into it:

```
mkdir deploy
copy build\mingw-release\bin\VitalSignsMonitor.exe deploy\
copy build\mingw-release\bin\VitalSignsPatientSimulator.exe deploy\
copy build\mingw-release\bin\VitalSignsSimulator.exe deploy\
```

3. Let windeployqt copy the Qt libraries and QML modules (run it from a prompt where the Qt and MinGW bin folders are on PATH):

```
windeployqt --release --qmldir src\app\qml deploy\VitalSignsMonitor.exe
windeployqt --release --qmldir tools\simulator\gui\qml deploy\VitalSignsPatientSimulator.exe
windeployqt --release deploy\VitalSignsSimulator.exe
```

4. Check that the MinGW runtime DLLs are in the folder (libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll). If they are missing, copy them from `C:\Qt\Tools\mingw1310_64\bin`.
5. Copy the whole `deploy` folder to the other computer and start the programs there. Test both programs before handing the folder on.

The open-source Qt libraries are licensed under the LGPL v3: when the folder is passed on, the Qt licence terms apply (dynamic linking, licence texts, access to the Qt source).

# 10. Virtual serial ports (com0com)

The serial path needs two serial ports that are connected to each other. Without hardware a virtual pair does this job.

1. Download com0com from its SourceForge project page. Use the signed 64-bit package.
2. Run the installer as administrator. It creates a pair such as CNCA0 / CNCB0.
3. Open "Setup Command Prompt" from the com0com start-menu group and give the ports normal names:

```
change CNCA0 PortName=COM5
change CNCB0 PortName=COM6
```

4. In the simulator choose SERIAL, port COM5, 115200, OPEN. In the monitor choose Serial, COM6, 115200, Connect.

> Windows 11 with Secure Boot may refuse old unsigned drivers. If the driver cannot be installed, use two USB-serial adapters connected with a null-modem cable, or another virtual serial port tool. The serial path of version 0.1.0 has not yet been tested end-to-end on this machine.

# 11. Linux

Not yet verified; the Linux build is planned for milestone M3 (WSL2).

1. Install the Qt 6.8+ desktop kit (gcc_64) with the Qt Online Installer, including Qt Serial Port. Distribution packages are often older than Qt 6.8.
2. Install the build tools, for example on Debian/Ubuntu: `sudo apt install build-essential ninja-build libgl1-mesa-dev` and a CMake of version 3.25 or newer.
3. Build:

```
export QTDIR=~/Qt/6.12.0/gcc_64
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

4. Serial ports such as /dev/ttyUSB0 need the dialout group: `sudo usermod -aG dialout $USER`, then log in again. Virtual pairs can be created with socat.

# 12. Network and firewall

- The simulator's TCP server listens on 127.0.0.1 only, so other computers cannot connect.
- The monitor only opens outgoing connections.
- If Windows Firewall asks about the simulator, allowing private networks is enough; local connections work either way.
- Default port 5555. If another program uses it, start the simulator with `--port 5556` and use the same port in the monitor.

# 13. Troubleshooting

| Message or symptom | Cause | Solution |
|---|---|---|
| CMake: Failed to find required Qt component "SerialPort" | Qt Serial Port is not installed for the selected kit | MaintenanceTool > Add or remove components > Qt 6.12.0 > Additional Libraries > Qt Serial Port, then run CMake again |
| CMake: Failed to find required Qt component "Multimedia" | Qt Multimedia is not installed for the selected kit | MaintenanceTool > Add or remove components > Qt 6.12.0 > Additional Libraries > Qt Multimedia, then run CMake again |
| CMake: Could not find a package configuration file provided by "Qt6" | CMAKE_PREFIX_PATH or QTDIR not set | Set QTDIR (presets) or pass -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 |
| CMake cannot download GoogleTest | No internet at the first configure | Connect to the internet once, or configure with -DVSM_BUILD_TESTS=OFF |
| "Qt6Core.dll was not found" when starting an .exe | Qt bin folder not on PATH | Start from Qt Creator, add C:\Qt\6.12.0\mingw_64\bin to PATH, or deploy (section 9) |
| A QML module is missing at start | Incomplete deployment | Run windeployqt with the --qmldir option shown in section 9 |
| Simulator closes when the monitor is started from Qt Creator | Qt Creator stops applications of the same project before building | Set "Stop applications before building" to "Same Application" (section 6) |
| Simulator: TCP port cannot be opened | Port already in use, for example by a second simulator | Close the other program or use --port 5556 |
| Serial port list is empty | No real or virtual serial ports | Install com0com (section 10) or connect an adapter, then refresh with ↻ |
| Kit "windows-msvc" fails | Visual Studio 2022 or an MSVC Qt kit is not installed | Use the MinGW kit, or install Visual Studio 2022 and the msvc2022_64 Qt kit |

# 14. Uninstalling

- Delete the project folder (and the deploy folder, if any). Version 0.2.0 stores no settings (no QSettings, no registry entries). The only other files are the audit logs in `%LOCALAPPDATA%\Mohsen Mazaheri\VitalSigns Monitor\audit`; delete this folder too if you do not need them.
- Remove Qt with `C:\Qt\MaintenanceTool.exe` > Remove all components.
- Remove com0com with its uninstaller in the Windows apps list.
