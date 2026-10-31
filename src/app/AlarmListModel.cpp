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
    const core::Alarm& alarm = m_alarms[static_cast<std::size_t>(index.row())];

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
    };
}

void AlarmListModel::setAlarms(std::vector<core::Alarm> alarms)
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
    // The evaluator sorts by priority, so the first entry is the most severe.
    return m_alarms.empty() ? 0 : static_cast<int>(m_alarms.front().priority);
}

QString AlarmListModel::topMessage() const
{
    return m_alarms.empty() ? QString() : QString::fromStdString(m_alarms.front().message);
}

} // namespace vsm::app
