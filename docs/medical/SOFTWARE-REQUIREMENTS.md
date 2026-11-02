# Software requirements

In the style of IEC 62304, 5.2. A learning exercise, not a compliance claim (see [README.md](README.md)).
Software version 0.3.0.

The RFP (`Documents/`) says what the system should offer; these requirements say what the software must
do, testably. Sources, code and tests are in [TRACEABILITY.md](TRACEABILITY.md).

## Communication

| ID | Requirement |
|---|---|
| SRS-001 | The monitor shall connect to a device over TCP, given a host and a port from 1 to 65535. |
| SRS-002 | The monitor shall connect to a device over a serial port at 9600, 57600, 115200 or 230400 baud, 8N1. |
| SRS-003 | Device I/O and decoding shall run outside the GUI thread. |
| SRS-004 | After an unexpected disconnect the monitor shall retry every 2 s until the operator disconnects. |
| SRS-005 | The link shall count as lost if numerics or ECG data have not arrived for 3 s. |
| SRS-006 | Heartbeat messages shall not count as data for SRS-005. |
| SRS-007 | While the link counts as lost, no vital signs and no ECG samples shall reach the display. The link recovers only when numerics and ECG have both arrived again. |
| SRS-008 | The monitor shall count and show valid frames, CRC errors, lost frames, sequence gaps and discarded bytes. A sequence wrap from 65535 to 0 is not a gap. |

## Device protocol and data checks

| ID | Requirement |
|---|---|
| SRS-010 | Frames shall have sync bytes, version, type, sequence, length, payload and CRC-16/CCITT-FALSE, little-endian. Payloads over 1024 bytes shall be refused. |
| SRS-011 | A frame with a wrong CRC, version or length shall never be delivered, and the decoder shall find the next valid frame in any chunking of the input. |
| SRS-012 | The monitor shall decode heartbeat, numerics and ECG waveform messages and refuse payloads of the wrong size. |
| SRS-013 | Unknown message types shall be ignored and counted. |
| SRS-014 | The memory used by the decoder shall stay bounded for any input. |
| SRS-015 | A heart rate above 300 bpm or an SpO2 above 100 % shall be treated as not available and counted. Other values in the same message remain valid. A value of 0 is valid. |
| SRS-016 | ECG samples shall be converted from microvolts to millivolts. |

## Display

| ID | Requirement |
|---|---|
| SRS-020 | The ECG shall be drawn as a sweep at the sample rate sent by the device. |
| SRS-021 | A value that is not available shall be shown as "---", never as a number. |
| SRS-022 | The display shall show "LEADS OFF" and "PROBE OFF" for the sensor flags. |
| SRS-023 | The connection bar shall offer the settings, connect/disconnect, status and the last error. Invalid settings shall be refused with a message. |
| SRS-024 | The monitor and the simulator shall permanently show that the data are synthetic and that this is not a medical device. |
| SRS-025 | When the link is lost, the numbers and the ECG trace shall be cleared. |

## Alarms

| ID | Requirement |
|---|---|
| SRS-030 | The monitor shall raise physiological alarms with these defaults: HR > 150 and HR < 40 high, HR > 120 and HR < 50 medium, SpO2 < 85 high, SpO2 < 90 medium. A limit itself raises no alarm. |
| SRS-031 | The monitor shall raise technical alarms: leads off (low), probe off (low), device link lost (medium). |
| SRS-032 | Each alarm shall be identified by text and priority colour, and an unacknowledged high-priority alarm shall flash, so that colour is never the only signal. |
| SRS-033 | Only the highest-priority alarm per source shall be reported, and alarms shall be listed by priority. |
| SRS-034 | While the link is lost, sensor alarms shall be suppressed, because their flags are stale. A missing value shall never raise a limit alarm. |
| SRS-035 | New alarm rules shall be possible without changing the evaluator. |
| SRS-036 | A high-priority alarm shall stay visible and audible after its condition ends, until acknowledged. Medium and low alarms end with their condition. |
| SRS-037 | Acknowledge shall silence current alarms, keep them visible while the condition lasts, and remove latched ones. An alarm that returns or escalates shall sound again. |
| SRS-038 | The audio pause shall last 2 minutes with a visible countdown and can be ended by the operator. A new medium or high alarm shall end it at once. Visual signals continue during the pause. |
| SRS-039 | The audio pause shall be refused when nothing is unacknowledged, and shall end when nothing is left to silence. |
| SRS-040 | Each priority shall have its own melody: high 10 pulses every 6 s, medium 3 every 8 s, low 2 every 20 s. The most urgent unacknowledged alarm decides. |
| SRS-041 | The tile of a parameter shall take an alarm colour only for a condition that exists now, not for a latched alarm. |
| SRS-042 | Disconnecting shall clear all alarms, including latched ones. |
| SRS-043 | Inconsistent alarm limits shall be refused. |

## Audit log

| ID | Requirement |
|---|---|
| SRS-050 | Every alarm change, operator key, connection change, link change and program start or stop shall be recorded with UTC time in milliseconds. |
| SRS-051 | Each entry shall say whether an operator or the system caused it. Entries shall hold no patient data. |
| SRS-052 | The latest events shall be readable on screen. The complete log shall stay in a CSV file. |
| SRS-053 | A log file that cannot be opened shall be reported on screen and shall not stop the monitor. |
| SRS-054 | Each entry shall be on disk before the next one is written. A new run shall append to the file of the same day. |
| SRS-055 | Repeated reconnect attempts shall not fill the log: only real connection changes are recorded. |

## Patient simulator

| ID | Requirement |
|---|---|
| SRS-060 | The simulator shall generate a synthetic ECG at the exact sample rate, with HR and SpO2 changing gradually. |
| SRS-061 | The simulator shall run scripted scenarios and show the current phase. |
| SRS-062 | The simulator shall offer manual control of HR, SpO2, leads off and probe off. Changing a value ends a running scenario. |
| SRS-063 | The simulator shall send over TCP (local computer only) or a serial port. |
| SRS-064 | The simulator shall inject faults: unplugged cable (frames numbered, not sent) and corrupted frames. |
| SRS-065 | The simulator shall have a front panel that looks like a bench-top instrument. |
| SRS-066 | A console simulator shall use the same engine. |
| SRS-067 | The simulator shall show frames sent, corrupted and dropped, bytes per second and serial line load. |

## Manual tests

For behaviour that automated tests do not reach: the QML views and real hardware. Run with the simulator.

| ID | Steps | Expected | Last result |
|---|---|---|---|
| MT-01 | Connect with TCP and let it run | ECG sweeps; the window stays responsive | Passed 2 Nov 2026, v0.3.0, judged from screenshots only |
| MT-02 | Connect over a virtual or real serial port pair | Same as MT-01 | **Not done: no COM-port pair on the development PC** |
| MT-03 | Simulator: LEADS OFF, then PROBE OFF | "LEADS OFF" on the ECG panel, "PROBE OFF" on the SpO2 tile, "---" for the values | Passed 2 Nov 2026, v0.3.0 |
| MT-04 | Read both windows | The synthetic-data notice is visible on the monitor and on the simulator | Passed 2 Nov 2026, v0.3.0 |
| MT-05 | Simulator: HR preset 160, SpO2 preset 85 | Red banner with text that flashes, yellow SpO2 tile | Passed 1 Nov 2026, v0.2.0 |
| MT-06 | Monitor with sound on: raise a high, a medium and a low alarm | Three different melodies | **Not done: playback starts without error, but nobody has listened** |
| MT-07 | Open TCP on the simulator and read its local address with `Get-NetTCPConnection -LocalPort <port>` | Bound to 127.0.0.1 only | Passed 2 Nov 2026, v0.3.0: 127.0.0.1 only, owned by the simulator. The state column was unreadable on this PC, and no connection was tried from another computer |
| MT-08 | Simulator: press keys, turn knobs, change the output | The panel works and looks like an instrument | Partly 2 Nov 2026, v0.3.0: keys and presets yes, knobs and output change not repeated |
| MT-09 | Run `VitalSignsSimulator --scenario sensor-off` and connect the monitor | The scenario's alarms appear in the monitor | Passed 2 Nov 2026, v0.3.0 |
| MT-10 | Monitor: press EVENTS during an alarm | Alarm list, newest events first, operator actions in cyan, path of the audit file | Passed 1 Nov 2026, v0.2.0 |
