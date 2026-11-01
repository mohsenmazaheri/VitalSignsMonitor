#include "AuditLogModel.h"

#include <QDateTime>

namespace vsm::app {

namespace {

constexpr std::size_t kMaxVisibleEntries = 500;

} // namespace

AuditLogModel::AuditLogModel(core::IAuditLog& next, QString location, QObject* parent)
    : QAbstractListModel(parent)
    , m_next(next)
    , m_location(std::move(location))
{
}

void AuditLogModel::record(const core::AuditEvent& event)
{
    m_next.record(event);

    beginInsertRows({}, 0, 0);
    m_entries.push_front({QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")), event});
    endInsertRows();

    if (m_entries.size() > kMaxVisibleEntries) {
        const int last = static_cast<int>(m_entries.size()) - 1;
        beginRemoveRows({}, last, last);
        m_entries.pop_back();
        endRemoveRows();
    }
}

int AuditLogModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_entries.size());
}

QVariant AuditLogModel::data(const QModelIndex& index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
        return {};
    }
    const Entry& entry = m_entries[static_cast<std::size_t>(index.row())];

    switch (role) {
    case TimeRole:
        return entry.time;
    case EventRole:
        return QString::fromUtf8(core::toString(entry.event.type));
    case CodeRole:
        return QString::fromStdString(entry.event.code);
    case Qt::DisplayRole:
    case DetailRole:
        return QString::fromStdString(entry.event.detail);
    case OperatorRole:
        return core::isOperatorAction(entry.event.type);
    default:
        return {};
    }
}

QHash<int, QByteArray> AuditLogModel::roleNames() const
{
    return {
        {TimeRole, "time"},
        {EventRole, "event"},
        {CodeRole, "code"},
        {DetailRole, "detail"},
        {OperatorRole, "operatorAction"},
    };
}

} // namespace vsm::app
