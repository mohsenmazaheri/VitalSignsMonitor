#pragma once

#include "vsm/core/AuditLog.h"

#include <QAbstractListModel>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include <deque>

namespace vsm::app {

/// Shows the latest audit events on screen and passes every event on to the
/// real log (decorator around core::IAuditLog). The list is limited; the file
/// keeps everything. Newest entry first.
class AuditLogModel : public QAbstractListModel, public core::IAuditLog {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("AuditLogModel is provided by MonitorViewModel")

    /// Where the complete log is written.
    Q_PROPERTY(QString location READ location CONSTANT)

public:
    enum Role {
        TimeRole = Qt::UserRole + 1,
        EventRole,
        CodeRole,
        DetailRole,
        OperatorRole,
    };
    Q_ENUM(Role)

    AuditLogModel(core::IAuditLog& next, QString location, QObject* parent = nullptr);

    void record(const core::AuditEvent& event) override;

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString location() const { return m_location; }

private:
    struct Entry {
        QString time;
        core::AuditEvent event;
    };

    core::IAuditLog& m_next;
    QString m_location;
    std::deque<Entry> m_entries;
};

} // namespace vsm::app
