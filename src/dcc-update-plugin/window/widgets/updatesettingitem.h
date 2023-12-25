// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef UpdateSettingItem_H
#define UpdateSettingItem_H

#include "widgets/settingsgroup.h"
#include "widgets/contentwidget.h"
#include "widgets/settingsitem.h"
#include "widgets/labels/tipslabel.h"
#include "widgets/detailinfoitem.h"
#include "updateiteminfo.h"
#include "indicatorcheckbox.h"

#include <QWidget>
#include <QAction>
#include <QLabel>
#include <QPushButton>
#include <QPointer>

#include <DFloatingButton>
#include <DCommandLinkButton>
#include <DLabel>
#include <DLineEdit>
#include <DTextEdit>
#include <DTipLabel>
#include <DShadowLine>
#include <DCheckBox>

namespace dcc {
namespace update {

struct Error_Info {
    UpdateErrorType ErrorType;
    QString errorMessage;
    QString errorTips;
};

class UpdateSettingItem: public dcc::widgets::SettingsItem
{
    Q_OBJECT
public:
    explicit UpdateSettingItem(UpdateType updateType, QWidget *parent = nullptr);
    void setIconVisible(bool show);
    void setIcon(QString path);
    virtual void setData(UpdateItemInfo *updateItemInfo);
    UpdateType updateType() const;
    void setChecked(bool isChecked);
    void setVersion(QString version);
    void setCheckBoxVisible(bool visible) { m_checkBox->setVisible(visible); }
    void setCheckBoxEnabled(bool enabled);
    UpdateItemInfo *updateItemInfo() const {  return m_updateItemInfo; }

Q_SIGNALS:
    void requestCheckUpdateModeChanged(UpdateType, bool);

public Q_SLOTS:
    virtual void showMore();

private:
    void initUi();
    void initConnect();
    void setCheckBoxState(UpdatesStatus status);
    void mousePressEvent(QMouseEvent *event) override;

protected:
    UpdateType m_updateType;
    dcc::widgets::SmallLabel *m_icon;
    DLabel *m_titleLabel;
    IndicatorCheckBox *m_checkBox;
    DLabel *m_versionLabel;
    DLabel *m_detailLabel;
    DLabel *m_dateLabel;
    DCommandLinkButton *m_showMoreButton;
    QWidget *m_infoWidget;
    dcc::widgets::SettingsGroup *m_settingsGroup;
    QPointer<UpdateItemInfo> m_updateItemInfo;
    bool m_allowSettingEnabled;
    QString m_hoveredLink;
};

}
}


#endif //UpdateSettingItem_H
