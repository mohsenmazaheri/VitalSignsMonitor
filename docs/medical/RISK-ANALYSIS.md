# Risk analysis

In the style of ISO 14971. A learning exercise, not a compliance claim (see [README.md](README.md)).
Software version 0.3.0.

**Assumption for this exercise:** the software is judged *as if* it monitored a real patient. It does
not. Severity and probability are a developer's estimates, with no clinical input and no field data, so
they show the method rather than measured risk.

## Scales

| Severity (S) | Meaning | | Probability (P) | Meaning |
|---|---|---|---|---|
| 1 | Negligible | | 1 | Improbable |
| 2 | Minor, temporary | | 2 | Remote |
| 3 | Serious | | 3 | Occasional |
| 4 | Critical or fatal | | 4 | Frequent |

**Score = S x P.** 1 to 4: acceptable. 5 to 8: review, reduce if practical, and justify what remains.
9 or more: not acceptable; a control is required. Controls in software lower the probability, not the
severity.

## Risk table

Before and after: the score without and with the controls. Controls are requirements from
[SOFTWARE-REQUIREMENTS.md](SOFTWARE-REQUIREMENTS.md). "Open" means work remains; see below.

| ID | Hazardous situation | Possible harm | S | P | Score | Controls | S after | P after | Residual | Status |
|---|---|---|---|---|---|---|---|---|---|---|
| R-01 | Old numbers stay on screen after the device stops sending | Treatment based on a value that is no longer true | 4 | 3 | 12 | SRS-005, SRS-021, SRS-025 | 4 | 1 | 4 | Controlled |
| R-02 | Device still sends heartbeats but no longer measures; numbers look current | Deterioration goes unnoticed | 4 | 2 | 8 | SRS-005, SRS-006, SRS-007 | 4 | 1 | 4 | Controlled |
| R-03 | A frozen ECG trace looks like a live one | Arrhythmia or asystole goes unnoticed | 3 | 3 | 9 | SRS-025 | 3 | 1 | 3 | Controlled |
| R-04 | Transmission error changes a value | Wrong number shown, false or missed alarm | 4 | 3 | 12 | SRS-010, SRS-011 | 4 | 1 | 4 | Controlled |
| R-05 | Device fault sends an impossible value with a correct CRC; SpO2 150 raises no alarm | Fault taken for a reading | 3 | 2 | 6 | SRS-015 | 3 | 1 | 3 | Controlled |
| R-06 | A value is missing with no sensor flag: "---" and no alarm | Loss of monitoring goes unnoticed | 3 | 2 | 6 | SRS-021 | 3 | 2 | 6 | Open |
| R-07 | A short critical episode ends before anyone looks | Serious event is never noticed | 4 | 2 | 8 | SRS-036 | 4 | 1 | 4 | Controlled |
| R-08 | Audio pause hides a new alarm | New critical condition is not heard | 4 | 2 | 8 | SRS-038, SRS-039 | 4 | 1 | 4 | Controlled |
| R-09 | Alarm acknowledged by mistake, then forgotten | Ongoing condition is not treated | 3 | 3 | 9 | SRS-037, SRS-050 | 3 | 2 | 6 | Controlled |
| R-10 | Sound does not play: no audio device, muted, or the sound fails | Alarm is not noticed by someone away from the screen | 3 | 2 | 6 | SRS-032, SRS-040 | 3 | 2 | 6 | Open |
| R-11 | The GUI thread hangs: display and sound stop, the frozen screen looks normal | All monitoring stops unnoticed | 4 | 1 | 4 | SRS-003 | 4 | 1 | 4 | Open |
| R-12 | Wrong or inconsistent alarm limits | Alarm too late or not at all | 4 | 2 | 8 | SRS-030, SRS-043 | 4 | 1 | 4 | Controlled |
| R-13 | A value moving around a limit raises and ends an alarm repeatedly | Alarm fatigue; real alarms get ignored | 2 | 3 | 6 | - | 2 | 3 | 6 | Open |
| R-14 | A reconnect mixes up old and new data or counts false losses | Misleading link information | 2 | 2 | 4 | SRS-004, SRS-008 | 2 | 1 | 2 | Controlled |
| R-15 | Endless or malformed data fills memory or stalls the decoder | Monitoring stops | 3 | 2 | 6 | SRS-011, SRS-012, SRS-014 | 3 | 1 | 3 | Controlled |
| R-16 | Audit file stops being written during a run, and only a failed open is reported | Events cannot be reconstructed later | 2 | 2 | 4 | SRS-053 | 2 | 2 | 4 | Open |
| R-17 | A crash loses recent audit entries | Events cannot be reconstructed later | 2 | 2 | 4 | SRS-054 | 2 | 1 | 2 | Controlled |
| R-18 | Latched alarm or old values carried into the next session | Alarm attributed to the wrong patient | 3 | 2 | 6 | SRS-042 | 3 | 1 | 3 | Controlled |
| R-19 | Data from the wrong source: TCP has no authentication or encryption, and there is no patient identity | Decisions based on another patient's or forged data | 4 | 2 | 8 | - | 4 | 2 | 8 | Open |
| R-20 | The program is mistaken for a real medical device | Used on a real patient | 4 | 1 | 4 | SRS-024 | 4 | 1 | 4 | Controlled |
| R-21 | ECG drawn with the wrong scale | Misleading trace | 3 | 2 | 6 | SRS-016 | 3 | 1 | 3 | Controlled |
| R-22 | A badly disturbed link raises a false "No data from device" | False alarm, and the numbers vanish briefly | 2 | 2 | 4 | SRS-008 | 2 | 2 | 4 | Controlled |
| R-23 | Alarm priority shown by colour alone | Colour-blind user misjudges the urgency | 3 | 2 | 6 | SRS-032 | 3 | 1 | 3 | Controlled |
| R-24 | The TCP server of the simulator is reachable from the network | Others could read the data stream | 2 | 2 | 4 | SRS-063 | 2 | 1 | 2 | Controlled |

## Open risks

These are known and not solved in version 0.3.0. None is acceptable for real use.

| Risk | What is missing | Proposed control |
|---|---|---|
| R-06 | A silent "---" gives no signal | A technical alarm when a value stays missing without a sensor flag |
| R-10 | A sound failure only reaches the log | Show it on screen; check the audio device at start-up |
| R-11 | Nothing watches the GUI thread; accepted for now only because the probability is low | A watchdog outside the GUI thread |
| R-13 | No hysteresis or alarm delay | Hysteresis per limit, with values from clinical input |
| R-16 | Write errors after the open are not checked | Check each write and raise a technical alarm |
| R-19 | No patient identity, no authentication, no encryption | Must be solved before any real use |

## Notes

- **R-22 is a side effect of the R-02 control.** With numerics once per second, 3 s without numerics
  needs three lost frames in a row. At the simulator's 20 % corruption setting that is about 0.8 % of
  all 3 s windows. This was accepted: a link that bad deserves attention. The value is calculated and
  was not measured.
- **Residual score 8 for R-19** is left at "review" only because of the learning context. In a real
  product it would block release.
- The CRC reduces but does not remove R-04: a 16-bit CRC can miss some multi-byte errors.
