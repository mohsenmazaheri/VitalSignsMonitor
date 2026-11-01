#pragma once

#include "DeviceSimulator.h"
#include "SinkFactory.h"

#include "vsm/ui/WaveformSeries.h"

#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

#include <memory>

namespace vsm::simulator::gui {

/// View model of the patient-simulator front panel (MVVM).
///
/// Translates panel actions (knobs, buttons, output selection) into calls on
/// DeviceSimulator and exposes the device state as QML properties. It owns
/// the active output sink, which it creates through an injected SinkFactory,
/// so it never needs to know TcpBroadcastSink or SerialSink.
class PatientSimulatorController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("PatientSimulatorController is created in main.cpp")

    // Power
    Q_PROPERTY(bool powered READ powered WRITE setPowered NOTIFY poweredChanged)

    // Mode / scenario
    Q_PROPERTY(QStringList scenarioNames READ scenarioNames CONSTANT)
    Q_PROPERTY(QString scenario READ scenario NOTIFY modeChanged)
    Q_PROPERTY(QString phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(int scenarioSeconds READ scenarioSeconds NOTIFY liveChanged)

    // Patient settings (writing any of them switches to manual mode)
    Q_PROPERTY(int targetHeartRate READ targetHeartRate WRITE setTargetHeartRate NOTIFY targetsChanged)
    Q_PROPERTY(int targetSpo2 READ targetSpo2 WRITE setTargetSpo2 NOTIFY targetsChanged)
    Q_PROPERTY(bool leadsOff READ leadsOff WRITE setLeadsOff NOTIFY targetsChanged)
    Q_PROPERTY(bool probeOff READ probeOff WRITE setProbeOff NOTIFY targetsChanged)
    Q_PROPERTY(int minimumHeartRate READ minimumHeartRate CONSTANT)
    Q_PROPERTY(int maximumHeartRate READ maximumHeartRate CONSTANT)
    Q_PROPERTY(int minimumSpo2 READ minimumSpo2 CONSTANT)
    Q_PROPERTY(int maximumSpo2 READ maximumSpo2 CONSTANT)

    // What the device reports right now (-1 = not reported, sensor off)
    Q_PROPERTY(int heartRate READ heartRate NOTIFY liveChanged)
    Q_PROPERTY(int spo2 READ spo2 NOTIFY liveChanged)
    Q_PROPERTY(vsm::ui::WaveformSeries* ecg READ ecg CONSTANT)
    Q_PROPERTY(int sampleRateHz READ sampleRateHz CONSTANT)

    // Output
    Q_PROPERTY(bool outputOpen READ outputOpen NOTIFY outputChanged)
    Q_PROPERTY(bool serialOutput READ serialOutput NOTIFY outputChanged)
    Q_PROPERTY(bool receiverConnected READ receiverConnected NOTIFY outputChanged)
    Q_PROPERTY(QString outputDescription READ outputDescription NOTIFY outputChanged)
    Q_PROPERTY(QString outputStatus READ outputStatus NOTIFY outputChanged)
    Q_PROPERTY(int baudRate READ baudRate NOTIFY outputChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

    // Fault injection
    Q_PROPERTY(bool cableConnected READ cableConnected WRITE setCableConnected NOTIFY cableConnectedChanged)
    Q_PROPERTY(double corruptionPercent READ corruptionPercent WRITE setCorruptionPercent NOTIFY
                   corruptionPercentChanged)

    // Statistics (double: QML numbers, counters can exceed 2^31)
    Q_PROPERTY(double framesSent READ framesSent NOTIFY statisticsChanged)
    Q_PROPERTY(double framesCorrupted READ framesCorrupted NOTIFY statisticsChanged)
    Q_PROPERTY(double framesDropped READ framesDropped NOTIFY statisticsChanged)
    Q_PROPERTY(int bytesPerSecond READ bytesPerSecond NOTIFY liveChanged)
    /// Serial line usage in percent of the baud rate (-1 for TCP).
    Q_PROPERTY(int lineLoadPercent READ lineLoadPercent NOTIFY liveChanged)

    Q_PROPERTY(QString version READ version CONSTANT)

public:
    static constexpr int kMinimumHeartRate = 20;
    static constexpr int kMaximumHeartRate = 250;
    static constexpr int kMinimumSpo2 = 50;
    static constexpr int kMaximumSpo2 = 100;

    PatientSimulatorController(DeviceSimulator& simulator, SinkFactory sinkFactory, QObject* parent = nullptr);
    ~PatientSimulatorController() override;

    PatientSimulatorController(const PatientSimulatorController&) = delete;
    PatientSimulatorController& operator=(const PatientSimulatorController&) = delete;
    PatientSimulatorController(PatientSimulatorController&&) = delete;
    PatientSimulatorController& operator=(PatientSimulatorController&&) = delete;

    // --- Commands (called from QML) -------------------------------------------
    Q_INVOKABLE bool runScenario(const QString& name);
    /// Leaves the scenario and holds its current targets.
    Q_INVOKABLE void stopScenario();
    Q_INVOKABLE QStringList availableSerialPorts() const;
    Q_INVOKABLE QList<int> standardBaudRates() const;
    Q_INVOKABLE bool openTcp(int port);
    Q_INVOKABLE bool openSerial(const QString& portName, int baudRate);
    /// Opens the output described by @p settings (also used by main.cpp).
    bool openOutput(const OutputSettings& settings);
    Q_INVOKABLE void closeOutput();

    // --- Property accessors -----------------------------------------------------
    [[nodiscard]] bool powered() const { return m_simulator.isRunning(); }
    void setPowered(bool on);

    [[nodiscard]] QStringList scenarioNames() const;
    [[nodiscard]] QString scenario() const { return m_simulator.scenarioName(); }
    [[nodiscard]] QString phase() const { return m_simulator.phaseLabel(); }
    [[nodiscard]] int scenarioSeconds() const;

    [[nodiscard]] int targetHeartRate() const { return m_simulator.targets().heartRateBpm; }
    void setTargetHeartRate(int bpm);
    [[nodiscard]] int targetSpo2() const { return m_simulator.targets().spo2Percent; }
    void setTargetSpo2(int percent);
    [[nodiscard]] bool leadsOff() const { return m_simulator.targets().ecgLeadsOff; }
    void setLeadsOff(bool off);
    [[nodiscard]] bool probeOff() const { return m_simulator.targets().spo2ProbeOff; }
    void setProbeOff(bool off);
    [[nodiscard]] int minimumHeartRate() const { return kMinimumHeartRate; }
    [[nodiscard]] int maximumHeartRate() const { return kMaximumHeartRate; }
    [[nodiscard]] int minimumSpo2() const { return kMinimumSpo2; }
    [[nodiscard]] int maximumSpo2() const { return kMaximumSpo2; }

    [[nodiscard]] int heartRate() const;
    [[nodiscard]] int spo2() const;
    [[nodiscard]] ui::WaveformSeries* ecg() { return &m_ecg; }
    [[nodiscard]] int sampleRateHz() const { return m_simulator.sampleRateHz(); }

    [[nodiscard]] bool outputOpen() const;
    [[nodiscard]] bool serialOutput() const { return m_settings.kind == OutputSettings::Kind::Serial; }
    [[nodiscard]] bool receiverConnected() const;
    [[nodiscard]] QString outputDescription() const;
    [[nodiscard]] QString outputStatus() const;
    [[nodiscard]] int baudRate() const { return m_settings.baudRate; }
    [[nodiscard]] QString lastError() const { return m_lastError; }

    [[nodiscard]] bool cableConnected() const { return m_simulator.isTransmissionEnabled(); }
    void setCableConnected(bool connected) { m_simulator.setTransmissionEnabled(connected); }
    [[nodiscard]] double corruptionPercent() const { return m_simulator.corruptionProbability() * 100.0; }
    void setCorruptionPercent(double percent);

    [[nodiscard]] double framesSent() const { return static_cast<double>(m_simulator.statistics().framesSent); }
    [[nodiscard]] double framesCorrupted() const
    {
        return static_cast<double>(m_simulator.statistics().framesCorrupted);
    }
    [[nodiscard]] double framesDropped() const { return static_cast<double>(m_simulator.statistics().framesDropped); }
    [[nodiscard]] int bytesPerSecond() const { return m_bytesPerSecond; }
    [[nodiscard]] int lineLoadPercent() const;

    [[nodiscard]] QString version() const;

signals:
    void poweredChanged();
    void modeChanged();
    void phaseChanged();
    void targetsChanged();
    void liveChanged();
    void outputChanged();
    void lastErrorChanged();
    void cableConnectedChanged();
    void corruptionPercentChanged();
    void statisticsChanged();

private:
    void changeTargets(const std::function<void(PatientTargets&)>& change);
    void onNumericsGenerated();
    void setLastError(const QString& error);

    DeviceSimulator& m_simulator;
    SinkFactory m_sinkFactory;
    std::unique_ptr<IByteSink> m_sink;
    OutputSettings m_settings;
    ui::WaveformSeries m_ecg;
    QString m_lastError;

    quint64 m_bytesAtLastSecond{0};
    int m_bytesPerSecond{0};
};

} // namespace vsm::simulator::gui
