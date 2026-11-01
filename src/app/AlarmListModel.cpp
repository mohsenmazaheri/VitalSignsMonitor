#include "AlarmListModel.h"

namespace vsm::app {

AlarmListModel::AlarmListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int AlarmListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_alarms.size());
}

QVariant AlarmListModel::data(const QModelIndex& index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
        return {};
    }
    const core::ManagedAlarm& managed = m_alarms[static_cast<std::size_t>(index.row())];
    const core::Alarm& alarm = managed.alarm;

    switch (role) {
    case CodeRole:
        return QString::fromStdString(alarm.code);
    case Qt::DisplayRole:
    case MessageRole:
        return QString::fromStdString(alarm.message);
    case PriorityRole:
        return static_cast<int>(alarm.priority);
    case PriorityNameRole:
        return QString::fromUtf8(core::toString(alarm.priority));
    case TechnicalRole:
        return alarm.category == core::AlarmCategory::Technical;
    case StateRole:
        return QString::fromUtf8(core::toString(managed.state));
    default:
        return {};
    }
}

QHash<int, QByteArray> AlarmListModel::roleNames() const
{
    return {
        {CodeRole, "code"},
        {MessageRole, "message"},
        {PriorityRole, "priority"},
        {PriorityNameRole, "priorityName"},
        {TechnicalRole, "technical"},
        {StateRole, "alarmState"},
    };
}

void AlarmListModel::setAlarms(std::vector<core::ManagedAlarm> alarms)
{
    if (alarms == m_alarms) {
        return;
    }
    beginResetModel();
    m_alarms = std::move(alarms);
    endResetModel();
    emit countChanged();
}

int AlarmListModel::highestPriority() const
{
    // The manager sorts by priority, so the first entry is the most severe.
    return m_alarms.empty() ? 0 : static_cast<int>(m_alarms.front().alarm.priority);
}

QString AlarmListModel::topMessage() const
{
    return m_alarms.empty() ? QString() : QString::fromStdString(m_alarms.front().alarm.message);
}

QString AlarmListModel::topState() const
{
    return m_alarms.empty() ? QString() : QString::fromUtf8(core::toString(m_alarms.front().state));
}

} // namespace vsm::app
