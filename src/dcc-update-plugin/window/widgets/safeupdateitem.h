// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef SAFEUPDATEITEM_H
#define SAFEUPDATEITEM_H

#include "updatesettingitem.h"
#include "detailinfoitem.h"

namespace dcc {
namespace update {


class SecurityUpdateItem: public UpdateSettingItem
{
    Q_OBJECT
public:
    explicit SecurityUpdateItem(QWidget *parent = nullptr);

    void init();
    void showMore() override;
    void setData(UpdateItemInfo *updateItemInfo) override;

public slots:
    void hideMore();

private:
    QList<UpdateDetailInfoItem *> m_updateDetailItemList;
};

}
}

#endif // SAFEUPDATEITEM_H
