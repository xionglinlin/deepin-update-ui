// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef SYSTEMUPDATEITEM_H
#define SYSTEMUPDATEITEM_H

#include "updatesettingitem.h"
#include "common.h"
#include "detailinfoitem.h"

#include <qdbusreply.h>
#include <QDBusInterface>
#include <QDBusPendingCall>

namespace dcc {
namespace update {

class SystemUpdateItem: public UpdateSettingItem
{
    Q_OBJECT
public:
    explicit SystemUpdateItem(QWidget *parent = nullptr);
    void showMore() override;
    void setData(UpdateItemInfo *updateItemInfo) override;
    char getLastNumForString(const QString &value);
    double subVersion(const QString &firstVersion, const QString &secondVersion);
    void createDetailInfoItem(const DetailInfo &info);

private Q_SLOTS:
    void hideMore();

private:
    QList<UpdateDetailInfoItem *> m_updateDetailItemList;
    DHorizontalLine *m_line;
    QWidget *m_lineWidget;
};

}
}

#endif // SYSTEMUPDATEITEM_H
