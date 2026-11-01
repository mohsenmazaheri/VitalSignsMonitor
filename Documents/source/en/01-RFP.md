---
title: Request for Proposal (RFP)
subtitle: VitalSigns Monitor - desktop vital-signs monitor and patient simulator (C++20, Qt 6 / QML)
docid: VSM-RFP-001
version: 1.1
date: 1 November 2026
author: Mohsen Mazaheri
status: Draft for review
lang: en
output: EN/VitalSignsMonitor-01-RFP-EN.docx
header: VitalSigns Monitor · Request for Proposal
history: 1.0 | 1 Nov 2026 | Mohsen Mazaheri | First issue, based on software version 0.1.0; 1.1 | 1 Nov 2026 | Mohsen Mazaheri | Alarm manager, audit log, alarm sounds and CI (software version 0.2.0)
notice: **Synthetic data only - not a medical device.** VitalSigns Monitor is a learning and portfolio project. It works only with synthetic signals from its own simulator and must not be used for diagnosis, treatment or patient monitoring.
---

# 1. Introduction

## 1.1 Purpose of this document

This Request for Proposal (RFP) describes the VitalSigns Monitor system and asks for proposals to design, build, test and document it. It is also the requirements baseline of the project: every requirement has an identifier (for example `FR-ALM-03`) that the design, the tests and later documents can refer to.

Version 0.2.0 of the software already exists. Appendix A shows which requirements it covers and which are planned.

## 1.2 Background

A bedside patient monitor receives data from acquisition devices (ECG amplifier, pulse oximeter) over a serial or network link. The software must read a continuous binary stream, detect and survive transmission errors, show waveforms and numbers without delay, and raise alarms with clear priorities. If the link fails, the monitor must say so instead of showing old values.

VitalSigns Monitor is a realistic, non-clinical model of such a system. It consists of a monitor application and a patient simulator that plays the role of the bedside device.

## 1.3 Important notice

- The system processes **synthetic data only**. No real patient data may be stored, processed or transmitted.
- The system is **not a medical device** and makes no claim of compliance with any medical standard.
- Where standards are mentioned (IEC 60601-1-8, IEC 62304, ISO 14971), the work is done "in the style of" the standard as a learning exercise.

## 1.4 Definitions and abbreviations

| Term | Meaning |
|---|---|
| ECG | Electrocardiogram - the electrical activity of the heart, shown as a waveform |
| HR | Heart rate in beats per minute (bpm) |
| SpO2 | Peripheral oxygen saturation in percent (%) |
| Frame | One protocol unit: sync bytes, header, payload and CRC |
| CRC | Cyclic redundancy check; here CRC-16/CCITT-FALSE |
| Sequence gap | A jump in the frame sequence number, meaning that frames were lost |
| Watchdog | Timer that reports a lost link when no valid frame arrives for 3 s |
| Transport | The monitor's connection to a device (TCP or serial port) |
| Sink | The simulator's output (TCP server or serial port) |
| Scenario | A scripted sequence of patient states in the simulator |
| MVVM | Model-View-ViewModel, the pattern used between C++ and QML |

# 2. Project overview

## 2.1 Objectives

- Build a desktop monitor that shows a live ECG, heart rate and SpO2 from a device connected over TCP or a serial port.
- Detect transmission problems (corrupted bytes, lost frames, silent link) and report them honestly.
- Raise physiological and technical alarms with three priority levels.
- Provide a patient simulator that generates synthetic signals and can inject faults, so that every behaviour of the monitor can be reproduced without hardware.
- Show a professional software process: layered architecture, SOLID design, unit tests, warning-free builds and complete documentation.

## 2.2 Scope

In scope:

- Monitor application (Windows and Linux desktop).
- Patient simulator with a hardware-style front panel, and a console simulator.
- Binary device protocol with CRC and sequence numbers.
- Alarm engine, unit tests, build system and documentation in English and Persian.

Out of scope:

- Real acquisition hardware and real patient data.
- Clinical validation, certification or regulatory submission.
- Central station, patient database, network distribution to several monitors (one device per monitor window).

## 2.3 Stakeholders and users

| Role | Interest |
|---|---|
| Project owner | Defines the requirements and accepts the deliverables |
| Software developer | Builds and maintains the system (see the Developer Guide) |
| Tester | Reproduces scenarios and faults with the simulator |
| Demo user / reviewer | Runs the programs to evaluate the work (see the User Guide) |

# 3. System context

The patient simulator generates the signals of a synthetic patient and sends them as protocol frames. The monitor receives the frames, checks them and displays the result.

```
+----------------------------+   TCP 127.0.0.1:5555    +-----------------------------+
|  VitalSignsPatientSimulator | ---------------------> |  VitalSignsMonitor          |
|  (or VitalSignsSimulator)   |   or serial COMx       |  ECG sweep, HR, SpO2,       |
|  synthetic patient,         |   (8N1, 115200 baud)   |  alarms, link statistics    |
|  scenarios, fault injection |                        |                             |
+----------------------------+                         +-----------------------------+
```

# 4. Functional requirements

Priority: **Must** = required for acceptance, **Should** = expected, **Could** = desirable if time allows.

## 4.1 Communication

| ID | Requirement | Priority |
|---|---|---|
| FR-COM-01 | The monitor connects to a device over TCP (host and port). | Must |
| FR-COM-02 | The monitor connects to a device over a serial port (port name, baud rate 9600 to 230400, 8N1). | Must |
| FR-COM-03 | Device I/O and parsing run in a separate thread; the user interface never waits for I/O. | Must |
| FR-COM-04 | After a lost connection the monitor reconnects automatically (default delay 2 s). | Must |
| FR-COM-05 | If no valid frame arrives for 3 s, the monitor reports "No data from device". | Must |
| FR-COM-06 | The monitor shows link statistics: frames, CRC errors, lost frames, sequence gaps and discarded bytes. | Should |

## 4.2 Device protocol

| ID | Requirement | Priority |
|---|---|---|
| FR-PRO-01 | Frames contain sync bytes, version, message type, sequence number, payload length, payload and a CRC-16/CCITT-FALSE. | Must |
| FR-PRO-02 | The decoder re-synchronises after corrupted bytes and rejects an invalid header before it waits for the payload. | Must |
| FR-PRO-03 | Message types: heartbeat (1 Hz), numerics (1 Hz) and ECG waveform (every 40 ms). | Must |
| FR-PRO-04 | Unknown message types are ignored and counted, so newer devices do not break older monitors. | Should |
| FR-PRO-05 | The protocol is documented byte by byte. | Must |

## 4.3 Display

| ID | Requirement | Priority |
|---|---|---|
| FR-DSP-01 | An ECG sweep is drawn at the sample rate sent by the device (250 Hz by default). | Must |
| FR-DSP-02 | Heart rate (bpm) and SpO2 (%) are shown as large numbers; a missing or invalid value is shown as "---". | Must |
| FR-DSP-03 | Sensor states are shown on the display ("LEADS OFF", "PROBE OFF"). | Must |
| FR-DSP-04 | A connection bar offers TCP or serial settings, Connect/Disconnect, a status lamp, a status text and the last error. | Must |
| FR-DSP-05 | A permanent notice states that the data are synthetic and that the program is not a medical device. | Must |

## 4.4 Alarms

| ID | Requirement | Priority |
|---|---|---|
| FR-ALM-01 | Physiological alarms with the default limits HR > 150, HR > 120, HR < 50, HR < 40 bpm, SpO2 < 90 % and SpO2 < 85 %. | Must |
| FR-ALM-02 | Technical alarms for ECG leads off, SpO2 probe off and lost device link. | Must |
| FR-ALM-03 | Three priorities (high, medium, low), each with its own colour. | Must |
| FR-ALM-04 | Only the highest-priority alarm of each source (HR, SpO2, ECG leads, probe, link) is reported. | Must |
| FR-ALM-05 | When the link is lost, the numbers are cleared and the sensor alarms are suppressed, because their flags are stale. | Must |
| FR-ALM-06 | Alarm rules are pluggable: a new rule needs no change in the evaluator. | Should |
| FR-ALM-07 | High-priority alarms latch: they stay visible and audible after the condition ends, until acknowledged. | Should |
| FR-ALM-08 | Alarm limits can be changed in the user interface and are saved. | Could |
| FR-ALM-09 | An acknowledge key silences current alarms and removes latched ones; an alarm that comes back or gets worse sounds again. | Should |
| FR-ALM-10 | An audio pause key silences for 2 minutes with a visible countdown; a new medium or high alarm ends the pause. | Should |
| FR-ALM-11 | Each priority has its own alarm melody; the more urgent, the more pulses. | Should |

## 4.5 Patient simulator

| ID | Requirement | Priority |
|---|---|---|
| FR-SIM-01 | The simulator generates a synthetic ECG (P, QRS and T waves); HR and SpO2 change gradually (3 bpm/s and 0.5 %/s). | Must |
| FR-SIM-02 | Scripted scenarios: normal, tachycardia, bradycardia, desaturation, sensor-off and demo. | Must |
| FR-SIM-03 | Manual control of HR (20 to 250 bpm), SpO2 (50 to 100 %), ECG leads off and SpO2 probe off. | Must |
| FR-SIM-04 | Output over TCP (local computer only) or a serial port. | Must |
| FR-SIM-05 | Fault injection: unplugged cable and corrupted frames (0, 1, 5 or 20 %). | Must |
| FR-SIM-06 | A graphical front panel that looks like a bench-top instrument (LCD, knobs, keys, LEDs). | Should |
| FR-SIM-07 | A console version with the same engine, for scripts and automated tests. | Should |
| FR-SIM-08 | Transmission statistics: frames sent, corrupted and dropped, bytes per second and serial line load. | Should |

## 4.6 Audit log

| ID | Requirement | Priority |
|---|---|---|
| FR-AUD-01 | Every alarm change, operator key, connection change and program start or stop is recorded with UTC time. | Should |
| FR-AUD-02 | The log tells operator actions from system events and contains no patient data. | Should |
| FR-AUD-03 | The latest events can be read on screen; the complete log stays in a CSV file. | Should |
| FR-AUD-04 | A log file that cannot be written is reported on screen and never stops the monitor. | Should |

# 5. Non-functional requirements

| ID | Category | Requirement |
|---|---|---|
| NFR-01 | Technology | C++20, Qt 6.8 or newer (Qt Quick / QML, Multimedia, Network, Serial Port), CMake 3.25 or newer. |
| NFR-02 | Platforms | Windows 10/11 64-bit (MinGW or MSVC) and Linux (GCC or Clang). |
| NFR-03 | Architecture | Layered design; the protocol and core layers do not depend on Qt; SOLID principles; dependencies are injected. |
| NFR-04 | Responsiveness | No I/O in the GUI thread; the ECG is drawn with the Qt Quick scene graph. |
| NFR-05 | Robustness | Corrupted input never crashes or stalls the decoder; the payload is limited to 1024 bytes. |
| NFR-06 | Testability | Automated unit tests (GoogleTest) for the protocol, alarms, buffers, device session and simulator engine; hardware and time are replaced by fakes in tests. |
| NFR-07 | Code quality | Warning-free build with GCC (-Wall -Wextra -Wpedantic -Wconversion and more) and MSVC (/W4); public types documented. |
| NFR-08 | Security | The simulator's TCP server listens on 127.0.0.1 only. |
| NFR-09 | Data protection | No personal or patient data is stored or transmitted. |
| NFR-10 | Documentation | README, architecture and protocol documents, learning log, and this document set in English and Persian. |
| NFR-11 | Cost | Free and open-source tools and libraries only (Qt open-source edition, GoogleTest); no paid licences. |
| NFR-12 | Continuous integration | Every change is built and tested on Windows and Linux with warnings as errors (GitHub Actions). |

# 6. Deliverables

| No. | Deliverable | Form |
|---|---|---|
| D1 | Source code with build system | Git repository (github.com/mohsenmazaheri/VitalSignsMonitor) |
| D2 | Monitor application | VitalSignsMonitor executable |
| D3 | Patient simulator with front panel | VitalSignsPatientSimulator executable |
| D4 | Console simulator | VitalSignsSimulator executable |
| D5 | Automated unit tests | GoogleTest suites run by CTest |
| D6 | Technical documents | README, ARCHITECTURE, PROTOCOL, LEARNING (Markdown) |
| D7 | This document set | RFP, Developer Guide, User Guide, Installation Guide (Word, English and Persian) |

# 7. Milestones

| Milestone | Content | State |
|---|---|---|
| M1 - week 2 | CMake project, protocol, simulator, I/O thread, monitor user interface, patient simulator front panel, 63 unit tests | Done (v0.1.0) |
| M2 - week 3 | Alarm manager (latching, acknowledge, audio pause, sounds), audit log, 121 unit tests, continuous integration (GitHub Actions, Windows and Linux) | Done (v0.2.0); first CI run pending |
| M3 - week 4 | Risk table in the style of ISO 14971, software requirements and traceability in the style of IEC 62304, Linux build in WSL2 | Planned |
| M4 - later | Stream data to a web application (ASP.NET Core and SignalR) | Idea |

# 8. Proposal content

A proposal should contain:

1. Understanding of the requirements, including any questions or assumptions.
2. Technical approach: architecture, threading model, protocol handling and alarm design.
3. Test approach: what is tested automatically, what manually, and how faults are reproduced.
4. Work plan with milestones that match section 7.
5. Risks and how they are reduced.
6. Effort estimate in hours per milestone.
7. Relevant experience and examples of earlier work.

# 9. Evaluation criteria

| Criterion | Weight |
|---|---|
| Technical approach and architecture | 30 % |
| Coverage of the functional and non-functional requirements | 20 % |
| Quality and test approach | 20 % |
| Work plan and risk handling | 15 % |
| Clarity of the proposal and of the documentation | 15 % |

# 10. Constraints and assumptions

- Development and first tests take place on Windows 11 with Qt 6.12.0 (MinGW 64-bit).
- One device is connected to one monitor window.
- The serial path needs either real hardware or a virtual COM-port pair (for example com0com).
- Alarm limits use fixed default values in version 0.2.0.
- No real patient data is ever used (see section 1.3).

# 11. Contact

Project owner: Mohsen Mazaheri. Questions about this RFP can be raised as issues in the project repository (github.com/mohsenmazaheri/VitalSignsMonitor).

# Appendix A. Status of version 0.2.0

| Requirement | State in v0.2.0 |
|---|---|
| FR-COM-01, FR-COM-03 to FR-COM-06 | Implemented and tested (unit tests and end-to-end over TCP) |
| FR-COM-02 | Implemented; not yet tested end-to-end (no virtual COM-port pair installed) |
| FR-PRO-01 to FR-PRO-05 | Implemented and unit-tested; documented in docs/PROTOCOL.md |
| FR-DSP-01 to FR-DSP-05 | Implemented |
| FR-ALM-01 to FR-ALM-06 | Implemented and unit-tested |
| FR-ALM-07, FR-ALM-09, FR-ALM-10 | Implemented, unit-tested and tried end-to-end with the simulator |
| FR-ALM-08 | Not implemented; planned for a later version |
| FR-ALM-11 | Implemented; melody patterns unit-tested, playback started without error, but nobody has listened to it yet |
| FR-AUD-01 to FR-AUD-04 | Implemented; file writing unit-tested, checked end-to-end. A write error after a successful open is not shown on screen |
| FR-SIM-01 to FR-SIM-08 | Implemented; engine unit-tested; serial output not yet tested end-to-end |
| NFR-01, NFR-03 to NFR-11 | Met |
| NFR-02 | Windows verified; Linux is built by CI, first run pending |
| NFR-12 | Workflow written; first run on GitHub pending |
