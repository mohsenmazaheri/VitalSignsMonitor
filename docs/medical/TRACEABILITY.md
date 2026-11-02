# Traceability matrix

In the style of IEC 62304. A learning exercise, not a compliance claim (see [README.md](README.md)).
Software version 0.3.0. `tools/check-traceability.ps1` verifies this file against the code and the tests.

Reading a row: the requirement comes from an RFP item or a risk, is implemented in the listed code, and is
verified by the listed tests. `MT-nn` is a manual test from
[SOFTWARE-REQUIREMENTS.md](SOFTWARE-REQUIREMENTS.md), which means no automated test covers that part.
`-` means the column does not apply.

| Requirement | Source | Risks | Code | Verification |
|---|---|---|---|---|
| SRS-001 | FR-COM-01 | - | src/device/TcpTransport.cpp | MT-01 |
| SRS-002 | FR-COM-02 | - | src/device/SerialTransport.cpp | MT-02 |
| SRS-003 | FR-COM-03 | R-11 | src/device/DeviceController.cpp | MT-01 |
| SRS-004 | FR-COM-04 | R-14 | src/device/DeviceSession.cpp | DeviceSessionTest.ReconnectsAfterUnexpectedDisconnect, DeviceSessionTest.StopDoesNotReconnect |
| SRS-005 | FR-COM-05 | R-01, R-02 | src/device/DeviceSession.cpp | DeviceSessionTest.WatchdogReportsLostLinkAndRecoveryClearsIt, DeviceSessionTest.LinkIsLostWhenNumericsStopButEcgContinues, DeviceSessionTest.LinkIsLostWhenEcgStopsButNumericsContinue, DeviceSessionTest.HealthyStreamKeepsTheLinkAlive |
| SRS-006 | FR-COM-05 | R-02 | src/device/DeviceSession.cpp | DeviceSessionTest.HeartbeatAloneDoesNotKeepTheLinkAlive |
| SRS-007 | FR-COM-05 | R-02 | src/device/DeviceSession.cpp | DeviceSessionTest.NothingIsDeliveredUntilNumericsAndEcgAreBothBack |
| SRS-008 | FR-COM-06 | R-14, R-22 | src/device/DeviceSession.cpp | DeviceSessionTest.CountsSequenceGapsAndCrcErrors, DeviceSessionTest.SequenceWrapAroundIsNotAGap |
| SRS-010 | FR-PRO-01, FR-PRO-05 | R-04 | src/protocol/Frame.cpp, src/protocol/Crc16.cpp | Crc16.MatchesStandardCheckValue, Crc16.EmptyInputReturnsInitialValue, Crc16.DetectsSingleBitFlip, FrameCodec.EncodesHeaderPayloadAndCrcLittleEndian, FrameCodec.AcceptsEmptyPayload, FrameCodec.RejectsOversizedPayload |
| SRS-011 | FR-PRO-02 | R-04, R-15 | src/protocol/FrameDecoder.cpp | FrameDecoder.DropsFrameWithBadCrcAndRecoversOnNextFrame, FrameDecoder.RejectsInvalidVersionAndResynchronises, FrameDecoder.RejectsOversizedLengthWithoutWaitingForIt, FrameDecoder.ReassemblesFrameDeliveredByteByByte, FrameDecoderFuzz.ChunkingDoesNotChangeTheResult, FrameDecoderFuzz.CorruptedStreamNeverDeliversAWrongFrame |
| SRS-012 | FR-PRO-03 | R-15 | src/protocol/Messages.cpp | Messages.HeartbeatRoundTrip, Messages.NumericsRoundTripWithValidValues, Messages.EcgRoundTripKeepsNegativeSamples, Messages.RejectsNumericsWithWrongPayloadSize, Messages.RejectsEcgWhoseSampleCountDoesNotMatchPayload, FrameDecoderFuzz.RandomFramesNeverCrashTheMessageParser |
| SRS-013 | FR-PRO-04 | - | src/protocol/Messages.cpp | Messages.IgnoresUnknownMessageType |
| SRS-014 | - | R-15 | src/protocol/FrameDecoder.cpp | FrameDecoder.BoundsMemoryOnEndlessGarbage, FrameDecoderFuzz.RandomBytesNeverCrashAndStayBounded |
| SRS-015 | FR-COM-07 | R-05 | src/device/DeviceSession.cpp | DeviceSessionTest.ImpossibleHeartRateBecomesEmptyAndIsCounted, DeviceSessionTest.ImpossibleSpo2BecomesEmptyAndIsCounted, DeviceSessionTest.ExtremeButPossibleValuesAreKept |
| SRS-016 | - | R-21 | src/device/DeviceSession.cpp | DeviceSessionTest.ConvertsEcgMicrovoltsToMillivolts |
| SRS-020 | FR-DSP-01 | - | src/ui/WaveformItem.cpp, src/ui/WaveformSeries.cpp | SweepBuffer.WritesLeftToRightAndWraps, MT-01 |
| SRS-021 | FR-DSP-02 | R-01, R-06 | src/app/MonitorViewModel.h, src/app/qml/NumericTile.qml | DeviceSessionTest.InvalidNumericsBecomeEmptyOptionals, MonitorViewModelTest.LinkLossClearsNumbersAndWaveform |
| SRS-022 | FR-DSP-03 | - | src/app/qml/Main.qml | MT-03 |
| SRS-023 | FR-DSP-04 | - | src/app/qml/ConnectionBar.qml, src/app/MonitorViewModel.cpp | MonitorViewModelTest.InvalidPortIsRefusedWithAMessage, MT-01 |
| SRS-024 | FR-DSP-05 | R-20 | src/app/qml/Main.qml, tools/simulator/gui/qml/DeviceCase.qml | MT-04 |
| SRS-025 | FR-COM-05 | R-01, R-03 | src/app/MonitorViewModel.cpp | MonitorViewModelTest.LinkLossClearsNumbersAndWaveform |
| SRS-030 | FR-ALM-01 | R-12 | src/core/AlarmRules.cpp, src/core/include/vsm/core/AlarmLimits.h | DefaultAlarmsTest.NormalVitalsRaiseNoAlarm, DefaultAlarmsTest.LimitsAreExclusive, AlarmEvaluator.CustomLimitsAreUsed, MonitorViewModelTest.VitalsReachTheTilesAndRaiseTheAlarm |
| SRS-031 | FR-ALM-02 | - | src/core/AlarmRules.cpp | DefaultAlarmsTest.SensorOffRaisesLowPriorityTechnicalAlarms, MonitorViewModelTest.LinkLossRaisesAndLogsTheAlarm |
| SRS-032 | FR-ALM-03 | R-10, R-23 | src/app/qml/AlarmBanner.qml, src/app/qml/Theme.qml | MonitorViewModelTest.SoundFollowsTheAlarmAndStopsOnAcknowledge, MT-05 |
| SRS-033 | FR-ALM-04 | - | src/core/AlarmEvaluator.cpp | DefaultAlarmsTest.CriticalAlarmReplacesMediumAlarmOfSameSource, DefaultAlarmsTest.AlarmsAreSortedByPriorityDescending, AlarmManagerTest.ListIsSortedByPriorityThenState |
| SRS-034 | FR-ALM-05 | - | src/core/AlarmRules.cpp | DefaultAlarmsTest.LinkLostSuppressesSensorAlarms, DefaultAlarmsTest.MissingValuesNeverRaiseLimitAlarms |
| SRS-035 | FR-ALM-06 | - | src/core/AlarmEvaluator.cpp | AlarmEvaluator.AcceptsCustomRuleThroughInterface, AlarmEvaluator.RejectsNullRule |
| SRS-036 | FR-ALM-07 | R-07 | src/core/AlarmManager.cpp | AlarmManagerTest.HighAlarmLatchesAfterItsConditionEnds, AlarmManagerTest.MediumAlarmEndsWithItsCondition, AlarmManagerTest.LatchedAlarmStaysUntilAcknowledged, AlarmManagerTest.LatchedAlarmBecomesActiveWhenTheConditionReturns, AlarmManagerTest.LatchedHighStaysAboveNewMediumOfTheSameSource, AlarmManagerTest.ShortCriticalEpisodeIsNotLost |
| SRS-037 | FR-ALM-09 | R-09 | src/core/AlarmManager.cpp | AlarmManagerTest.AcknowledgeSilencesButKeepsTheAlarmVisible, AlarmManagerTest.AcknowledgedHighAlarmDoesNotLatch, AlarmManagerTest.AlarmThatComesBackAfterAcknowledgeSoundsAgain, AlarmManagerTest.EscalationAfterAcknowledgeSoundsAgain, MonitorViewModelTest.SoundFollowsTheAlarmAndStopsOnAcknowledge |
| SRS-038 | FR-ALM-10 | R-08 | src/core/AlarmManager.cpp, src/app/MonitorViewModel.cpp | AlarmManagerTest.AudioPauseSilencesForTwoMinutesThenSoundReturns, AlarmManagerTest.AudioPauseDurationComesFromThePolicy, AlarmManagerTest.OperatorCanEndTheAudioPause, AlarmManagerTest.NewMediumAlarmEndsTheAudioPause, AlarmManagerTest.NewLowAlarmKeepsTheAudioPause, MonitorViewModelTest.AudioPauseCountsDownAndCanBeEnded |
| SRS-039 | FR-ALM-10 | R-08 | src/core/AlarmManager.cpp | AlarmManagerTest.AudioPauseWithoutAlarmsIsRefused, AlarmManagerTest.AudioPauseEndsWhenNothingIsLeftToSilence, AlarmManagerTest.AcknowledgingEverythingEndsTheAudioPause |
| SRS-040 | FR-ALM-11 | R-10 | src/core/AlarmTone.cpp, src/app/QtAlarmSound.cpp | AlarmToneByPriority.HasTheDocumentedNumberOfPulses, AlarmToneByPriority.LengthEqualsTheRepeatPeriod, AlarmToneByPriority.EndsWithSilenceSoThatItCanLoop, AlarmToneByPriority.IsLoudButDoesNotClip, AlarmTone.MoreUrgentMeansMorePulses, Wav.HeaderDescribesMono16BitPcm, AlarmManagerTest.SoundFollowsTheMostUrgentUnacknowledgedAlarm, MT-06 |
| SRS-041 | FR-ALM-07 | - | src/app/MonitorViewModel.cpp | MonitorViewModelTest.LatchedAlarmStaysOnTheBannerButNotOnTheTile |
| SRS-042 | - | R-18 | src/core/AlarmManager.cpp, src/app/MonitorViewModel.cpp | AlarmManagerTest.ClearForgetsEverythingIncludingLatchedAlarms, MonitorViewModelTest.DisconnectForgetsLatchedAlarms |
| SRS-043 | - | R-12 | src/core/AlarmLimits.cpp | AlarmLimits.DefaultsAreValid, AlarmLimits.HeartRateLimitsMustBeOrdered, AlarmLimits.Spo2LimitsMustBeOrderedAndAtMostHundred, AlarmEvaluator.RejectsInvalidLimits |
| SRS-050 | FR-AUD-01 | R-09 | src/core/AlarmManager.cpp, src/app/MonitorViewModel.cpp, src/audit/FileAuditLog.cpp | AlarmManagerTest.NewAlarmIsActiveAudibleAndLogged, AuditFormat.TimestampIsUtcWithMilliseconds, AuditFormat.AlarmLineHasAllColumns, MonitorViewModelTest.LinkLossRaisesAndLogsTheAlarm |
| SRS-051 | FR-AUD-02 | - | src/core/AuditLog.cpp, src/audit/FileAuditLog.cpp | AuditEvents.OnlyOperatorKeysCountAsOperatorActions, AuditFormat.OperatorEventWithoutAlarmLeavesColumnsEmpty |
| SRS-052 | FR-AUD-03 | - | src/app/AuditLogModel.cpp, src/app/qml/EventLogPopup.qml | MonitorViewModelTest.EverythingLoggedAlsoAppearsOnScreen, FileAuditLogTest.CreatesFolderAndWritesHeaderAndEvents, MT-10 |
| SRS-053 | FR-AUD-04 | R-16 | src/audit/FileAuditLog.cpp, src/app/main.cpp | FileAuditLogTest.UnwritableFileIsReportedAndDoesNotThrow |
| SRS-054 | - | R-17 | src/audit/FileAuditLog.cpp | FileAuditLogTest.EachEventIsOnDiskImmediately, FileAuditLogTest.SecondRunAppendsWithoutSecondHeader |
| SRS-055 | FR-AUD-01 | - | src/app/MonitorViewModel.cpp | MonitorViewModelTest.ReconnectLoopDoesNotFloodTheLog |
| SRS-060 | FR-SIM-01 | - | tools/simulator/engine/SyntheticPatient.cpp | DeviceSimulatorTest.GeneratesExactlyTheSampleRate, DeviceSimulatorTest.HeartRateMovesGraduallyTowardsTheTarget |
| SRS-061 | FR-SIM-02 | - | tools/simulator/engine/Scenario.cpp | DeviceSimulatorTest.ScenarioDrivesTargetsAndPhase |
| SRS-062 | FR-SIM-03 | - | tools/simulator/engine/DeviceSimulator.cpp | DeviceSimulatorTest.SendsNumericsOncePerSecondWithManualTargets, DeviceSimulatorTest.SensorsOffReportInvalidValuesAndFlatEcg, DeviceSimulatorTest.ManualTargetsEndTheScenario |
| SRS-063 | FR-SIM-04 | R-24 | tools/simulator/engine/TcpBroadcastSink.cpp, tools/simulator/engine/SerialSink.cpp | MT-01, MT-02, MT-07 |
| SRS-064 | FR-SIM-05 | - | tools/simulator/engine/DeviceSimulator.cpp | DeviceSimulatorTest.UnpluggedCableSendsNothingButKeepsNumbering, DeviceSimulatorTest.FullCorruptionMakesEveryFrameFailTheCrc |
| SRS-065 | FR-SIM-06 | - | tools/simulator/gui/qml/DeviceCase.qml | MT-08 |
| SRS-066 | FR-SIM-07 | - | tools/simulator/cli/main.cpp | MT-09 |
| SRS-067 | FR-SIM-08 | - | tools/simulator/engine/DeviceSimulator.cpp, tools/simulator/gui/qml/LcdScreen.qml | DeviceSimulatorTest.NoSinkDropsFramesWithoutCrashing, DeviceSimulatorTest.PowerOffStopsGeneration, MT-08 |

## RFP items that are not implemented

| RFP item | Reason |
|---|---|
| FR-ALM-08 | Alarm limits cannot be changed in the user interface yet. |

## Known gaps in this matrix

- **MT-02 and MT-06 have not been done**, and MT-08 was only partly repeated. The serial path and the sound of the melodies are unverified.
- Manual results were recorded by the developer, with no independent witness or signed record, which a real process would require.
- Tests that run the view model do not exercise the QML files that bind to it.
