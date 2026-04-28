// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MIRRORSOURCEMODEL_H
#define MIRRORSOURCEMODEL_H

#include "common/dbus/mirrorinfolist.h"
#include "common.h"

#include <QAbstractListModel>

using namespace dcc::update::common;
class MirrorSourceModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum MirrorRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        UrlRole,
        SpeedRole
    };

    explicit MirrorSourceModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[IdRole] = "id";
        roles[NameRole] = "name";
        roles[UrlRole] = "url";
        roles[SpeedRole] = "speed";
        return roles;
    }

    void setMirrorList(const MirrorInfoList &list);
    void updateMirrorSpeed(const QString &mirrorId, int speed);
    void resetSpeedInfo();

private:
    struct MirrorData {
        QString id;
        QString name;
        QString url;
        int speed = SpeedStatus::Untested;
    };

    QList<MirrorData> m_mirrors;
};

#endif // MIRRORSOURCEMODEL_H
