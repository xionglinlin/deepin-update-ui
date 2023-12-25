// SPDX-FileCopyrightText: 2016 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef UPDATEDETAILINFOITEM_H
#define UPDATEDETAILINFOITEM_H

#include "widgets/labels/normallabel.h"
#include "widgets/labels/largelabel.h"
#include "widgets/settingsitem.h"
#include "widgets/contentwidget.h"
#include "common.h"

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>

#include <DFloatingButton>
#include <DCommandLinkButton>
#include <DLabel>
#include <DLineEdit>
#include <DTextEdit>
#include <DTipLabel>

namespace dcc {
namespace update {

const QString titleColor = "#0082fa";
const QString grayColor = "#526a7f";

const int titleFontSize = 14;
const int dateFontSize = 12;
const int detailFontSize = 8;
const int linkFontSize = 10;

class UpdateDetailInfoItem: public dcc::widgets::SettingsItem
{
    Q_OBJECT
public:
    explicit UpdateDetailInfoItem(QWidget *parent = 0);

    void initUi();

    void setDate(QString date);
    void setTitle(QString title);
    void setLinkData(QString data);
    void setDetailData(QString data);
    void setIsLastItem(bool isLastItem);
    void setUpdateType(UpdateType type);

Q_SIGNALS:
    void requestHideDetails();

private:
    DLabel *m_dateLabel;
    DLabel *m_explainTitle;
    DLabel *m_linkDataLabel;
    DLabel *m_dataLabel;
    DLabel *m_linkLabel;
    DLabel *m_title;
    QHBoxLayout *m_titleLayout;
    QVBoxLayout *m_mainLayout;
    DCommandLinkButton *m_hideButton;
};

}
}

#endif //UPDATEDETAILINFOITEM_H
