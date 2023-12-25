// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef UPGRADEHISTORYDIALOG_H
#define UPGRADEHISTORYDIALOG_H

#include "systemupdateitem.h"
#include "safeupdateitem.h"
#include "historyupdateitem.h"
#include "updateloghelper.h"

#include <QObject>
#include <QVBoxLayout>

#include <DDialog>

using namespace dcc::update;

class UpgradeHistoryDialog : public Dtk::Widget::DDialog
{
    Q_OBJECT
public:
    explicit UpgradeHistoryDialog(QWidget *parent = nullptr);
    ~UpgradeHistoryDialog();

private:
    void initUI();
    void initConnections();
    void updateLayout(const QList<HistoryItemInfo>& items);
    DLabel *createLable();

private:
    QMap<UpdateType, QPointer<HistoryUpdateItem>> m_updateItems;

    QVBoxLayout* m_contentLayout;
    QWidget *m_normalHistoryWidget;
    QWidget *m_noHistoryWidget;
};

#endif