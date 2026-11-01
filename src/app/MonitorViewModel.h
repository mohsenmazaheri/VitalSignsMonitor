#pragma once

#include "AlarmListModel.h"
#include "vsm/ui/WaveformSeries.h"

#include "vsm/core/AlarmEvaluator.h"
#include "vsm/core/VitalSigns.h"
#include "vsm/device/DeviceController.h"

#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

namespace vsm::app {

/// View model for the main monitor screen (MVVM).
///
/// Translates device events into QML-friendly properties and runs the alarm
/// evaluation. It holds no I/O and no alarm rules itself: both are injected
/// (DeviceController, AlarmEvaluator), which keeps this class small and lets
/// tests replace them.
class MonitorViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("MonitorViewModel is created in main.cpp")

    // Numerics: -1 means "no valid value" (QML shows "---").
    Q_PROPERTY(int heartRate READ heartRate NOTIFY vitalsChanged)
    Q_PROPERTY(int spo2 READ spo2 NOTIFY vitalsChanged)
    Q_PROPERTY(bool ecgLeadsOff READ ecgLeadsOff NOTIFY vitalsChanged)
    Q_PROPERTY(bool spo2ProbeOff READ spo2ProbeOff NOTIFY vitalsChanged)

    // Highest active alarm priority per parameter (0 = none), for tile colouring.
    Q_PROPERTY(int heartRateAlarmPriority READ heartRateAlarmPriority NOTIFY alarmsChanged)
    Q_PROPERTY(int spo2AlarmPriority READ spo2AlarmPriority NOTIFY alarmsChanged)

    Q_PROPERTY(bool sessionActive READ sessionActive NOTIFY connectionChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectionChanged)
    Q_PROPERTY(QString connectionStatus READ connectionStatus NOTIFY connectionChanged)
    Q_PROPERTY(QString linkStatistics READ linkStatistics NOTIFY linkStatisticsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

    Q_PROPERTY(vsm::app::AlarmListModel* alarms READ alarms CONSTANT)
    Q_PROPERTY(vsm::ui::WaveformSeries* ecg READ ecg CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    MonitorViewModel(device::DeviceController& device, core::AlarmEvaluator evaluator, QObject* parent = nullptr);

    Q_INVOKABLE void connectTcp(const QString& host, int port);
    Q_INVOKABLE void connectSerial(const QString& portName, int baudRate);
    Q_INVOKABLE void disconnectDevice();
    [[nodiscard]] Q_INVOKABLE QStringList availableSerialPorts() const;

    [[nodiscard]] int heartRate() const { return m_vitals.heartRateBpm.value_or(-1); }
    [[nodiscard]] int spo2() const { return m_vitals.spo2Percent.value_or(-1); }
    [[nodiscard]] bool ecgLeadsOff() const { return m_vitals.ecgLeadsOff; }
    [[nodiscard]] bool spo2ProbeOff() const { return m_vitals.spo2ProbeOff; }

    [[nodiscard]] int heartRateAlarmPriority() const;
    [[nodiscard]] int spo2AlarmPriority() const;

    [[nodiscard]] bool sessionActive() const { return m_device.hasSession(); }
    [[nodiscard]] bool connected() const { return m_connectionState == device::ITransport::State::Connected; }
    [[nodiscard]] QString connectionStatus() const;
    [[nodiscard]] QString linkStatistics() const { return m_linkStatistics; }
    [[nodiscard]] QString lastError() const { return m_lastError; }

    [[nodiscard]] AlarmListModel* alarms() { return &m_alarms; }
    [[nodiscard]] ui::WaveformSeries* ecg() { return &m_ecg; }
    [[nodiscard]] static QString version();

signals:
    void vitalsChanged();
    void alarmsChanged();
    void connectionChanged();
    void linkStatisticsChanged();
    void lastErrorChanged();

private:
    void connectWith(const device::ConnectionSettings& settings);
    void onVitals(const core::VitalSigns& vitals);
    void onConnectionState(device::ITransport::State state);
    void onLinkLost(bool lost);
    void onStatistics(const device::LinkStatistics& statistics);
    void setLastError(const QString& message);
    void resetPatientData();
    void reevaluateAlarms();
    [[nodiscard]] int priorityFor(core::AlarmSource source) const;

    device::DeviceController& m_device;
    core::AlarmEvaluator m_evaluator;
    AlarmListModel m_alarms;
    ui::WaveformSeries m_ecg;

    core::VitalSigns m_vitals;
    bool m_linkLost{false};
    device::ITransport::State m_connectionState{device::ITransport::State::Disconnected};
    QString m_endpoint;
    QString m_linkStatistics;
    QString m_lastError;
};

} // namespace vsm::app
