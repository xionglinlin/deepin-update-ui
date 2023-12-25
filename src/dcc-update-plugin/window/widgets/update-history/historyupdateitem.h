// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HISTORYUPDATEITEM_H
#define HISTORYUPDATEITEM_H

#include "updatesettingitem.h"
#include "updateloghelper.h"

#include <QObject>

namespace dcc {
namespace update {

class HistoryUpdateItem : public dcc::widgets::SettingsItem
{
    Q_OBJECT
public:
    explicit HistoryUpdateItem(const HistoryItemInfo &info, QWidget *parent = nullptr);
    ~HistoryUpdateItem();

private:
    void initUi();
    void addItem(const HistoryItemInfo &info);
    void addSecurityItem(const HistoryItemInfo &info);
    void addSystemItem(const HistoryItemInfo &info);
    DLabel* createDetailLabel();
    DLabel* createNameLabel();

private:
    UpdateType m_updateType;
    dcc::widgets::SmallLabel *m_icon;
    DLabel *m_titleLabel;
    DLabel *m_summaryLabel;
    DLabel *m_dateLabel;
    QVBoxLayout* m_contentLayout;
    QWidget *m_infoWidget;
};

}
}

#endif