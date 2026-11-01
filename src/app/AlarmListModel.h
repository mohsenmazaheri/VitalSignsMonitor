#pragma once

#include "vsm/core/AlarmManager.h"

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

#include <vector>

namespace vsm::app {

/// Read-only list model of the alarms known to the AlarmManager, most important first.
/// Exposed to QML; the content is replaced by MonitorViewModel after each change.
class AlarmListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("AlarmListModel is provided by MonitorViewModel")

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    /// 0 = no alarm, otherwise the value of vsm::core::AlarmPriority (1..3).
    Q_PROPERTY(int highestPriority READ highestPriority NOTIFY countChanged)
    Q_PROPERTY(QString topMessage READ topMessage NOTIFY countChanged)
    /// The state of the first alarm: "ACTIVE", "LATCHED", "ACKNOWLEDGED" or empty.
    Q_PROPERTY(QString topState READ topState NOTIFY countChanged)

public:
    enum Role {
        CodeRole = Qt::UserRole + 1,
        MessageRole,
        PriorityRole,
        PriorityNameRole,
        TechnicalRole,
        StateRole,
    };
    Q_ENUM(Role)

    explicit AlarmListModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    /// Replaces the content; does nothing if the alarms did not change
    /// (avoids needless QML delegate re-creation).
    void setAlarms(std::vector<core::ManagedAlarm> alarms);

    [[nodiscard]] int highestPriority() const;
    [[nodiscard]] QString topMessage() const;
    [[nodiscard]] QString topState() const;
    [[nodiscard]] const std::vector<core::ManagedAlarm>& alarms() const noexcept { return m_alarms; }

signals:
    void countChanged();

private:
    std::vector<core::ManagedAlarm> m_alarms;
};

} // namespace vsm::app
