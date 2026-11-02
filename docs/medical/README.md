# Medical-software documents (learning exercise)

> **VitalSigns Monitor is not a medical device.** It works with synthetic data only. These documents
> practise the *method* of ISO 14971 and IEC 62304. They claim **no compliance** with any standard, and
> no notified body, clinician or quality system has reviewed them.

| Document | In the style of | Answers |
|---|---|---|
| [SOFTWARE-REQUIREMENTS.md](SOFTWARE-REQUIREMENTS.md) | IEC 62304, 5.2 | What must the software do, testably? |
| [RISK-ANALYSIS.md](RISK-ANALYSIS.md) | ISO 14971 | What can go wrong, how bad is it, what reduces it, what is left? |
| [TRACEABILITY.md](TRACEABILITY.md) | IEC 62304, 5.1.1 / 5.7 | Which code and which test stand behind each requirement and risk? |

## The check that keeps them honest

`tools/check-traceability.ps1` runs as the CTest test `traceability_check`, so it also runs in CI. It
fails when:

- a test, source file or manual test named in the matrix does not exist,
- a requirement has no entry in the matrix, or an entry has no requirement,
- a risk names a requirement that does not exist, or the risk table and the matrix disagree,
- a score is not severity x probability, or a risk stays unacceptable after its controls,
- an `FR-` requirement of the RFP is neither traced nor listed as not implemented.

It checks that the links exist. It does not check that a test really proves its requirement; that
still takes a review.

## What the analysis changed in the code

The analysis was done against the running code, and it found real problems. Fixed in version 0.3.0:

| Found | Before | Now | Risk |
|---|---|---|---|
| A heartbeat kept the link "alive" | Device stops measuring but sends heartbeats: the last HR stays on screen, no alarm | Numerics and ECG each have a 3 s watchdog; heartbeats do not count | R-02 |
| Impossible values were displayed | SpO2 150 shown as a reading, no alarm | Values above 300 bpm or 100 % become "---" and are counted | R-05 |
| The ECG froze on link loss | Numbers cleared, old trace stayed and could look live | The trace is cleared too | R-03 |
| No test for the view model | Clearing stale data was only checked by hand | `vsm_app_tests` runs the real view model and alarm chain | R-01, R-03 |

Risks that are still **open** are listed in [RISK-ANALYSIS.md](RISK-ANALYSIS.md). They stay visible on purpose.

## Software safety class

If this were a real monitor, IEC 62304 class **C** would apply: a missed alarm could contribute to death
or serious injury. Class C is the assumption behind these exercises. The software was **not** developed
under a class C process.

## What a real project would still need

- A quality management system (ISO 13485) and a software development plan.
- Formal reviews with records, independent verification, and a problem resolution process.
- Clinical input for severity, probability and alarm limits. The values here are a developer's estimates.
- Usability engineering (IEC 62366) and measurement of the alarm signals against IEC 60601-1-8.
- A cybersecurity risk analysis, and hazard analysis of the hardware and system around the software.
- Evaluation of known anomalies in each third-party component (below).
- Segregation of safety-relevant parts from the user interface.

## Third-party software (SOUP, IEC 62304 8.1.2)

"Software of unknown provenance": components not developed under this process.

| Component | Version | Used for | If it fails |
|---|---|---|---|
| Qt Core, Network, Serial Port | 6.8.3 in CI, 6.12.0 in development | Event loop, TCP, serial I/O | Link loss is detected by the watchdog; a hang is not (R-11) |
| Qt Quick, Quick Controls | as above | Display | Wrong or frozen display, no independent check (R-11) |
| Qt Multimedia, with its media plugins | as above | Alarm sound | Silence; the visual signals remain (R-10) |
| GoogleTest | 1.15.2 | Unit tests only, not shipped | A false test result |
| MinGW GCC | 13.1 | Compiler and runtime | Wrong code; reduced by running the tests on two compilers |

Known anomalies of these versions have not been reviewed.
