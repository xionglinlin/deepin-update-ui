// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "interface/moduleinterface.h"
#include "widgets/contentwidget.h"
#include "window/dconfigwatcher.h"
#include "updatemodel.h"
#include "updatemodesettingitem.h"

#include <QCheckBox>

#include <DTimeEdit>
#include <DSpinBox>

namespace dcc {
namespace widgets {
class SettingsGroup;
class SwitchWidget;
class TipsLabel;
}

namespace update {
class UpdateModel;
class NextPage;
}
}

QT_BEGIN_NAMESPACE
class QGSettings;
QT_END_NAMESPACE

DWIDGET_BEGIN_NAMESPACE
class DTipLabel;
DWIDGET_END_NAMESPACE

namespace DCC_NAMESPACE {
namespace update {

class UpdateSettings : public dcc::ContentWidget
{
    Q_OBJECT

public:
    explicit UpdateSettings(dcc::update::UpdateModel *model, QWidget *parent = 0);
    virtual ~UpdateSettings();
    void setWidgetsEnabled(bool enable);

Q_SIGNALS:
    void requestSetUpdateNotify(bool notify);
    void requestSetAutoDownloadUpdates(const bool &autoUpdate);
    void requestSetAutoCleanCache(const bool autoClean);
#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    void requestSetSourceCheck(const bool check);
#endif
    void requestEnableSmartMirror(bool enable);
    void requestShowMirrorsView();
    void requestSetTestingChannelEnable(const bool &enable);
    void requestCheckCanExitTestingChannel();
    void requestSetIdleDownloadConfig(const dcc::update::IdleDownloadConfig &config);
    void requestSetDownloadSpeedLimitConfig(const QString &config);
    void requestUpdateMode(quint64 updateMode);
    void requestSetP2PEnabled(bool enabled);

protected:
    bool eventFilter(QObject *, QEvent *) override;

private Q_SLOTS:
    void onTestingChannelCheckChanged(const bool checked);
    void onTestingChannelStatusChanged();
    void setIdleDownloadTimeWidgetsEnabled(bool enabled);
    void updateWidgetsEnabledStatus();

private:
    void initUi();
    void initConnection();
    void setModel(dcc::update::UpdateModel *model);

private:
    dcc::update::UpdateModel *m_model;
    UpdateModeSettingItem *m_updateModeSettingItem;
    dcc::widgets::SwitchWidget *m_updateNotify;              // 更新提醒开关
    dcc::widgets::SwitchWidget *m_autoDownloadUpdate;        // 下载更新开关
    dcc::widgets::SwitchWidget *m_downloadSpeedLimitSwitch;  // 更新限速开关
    DTK_WIDGET_NAMESPACE::DSpinBox *m_downloadSpeedLimitSpinBox;
    QWidget *m_downloadSpeedLimitWidget;                    // 用于SpinBox的显示和隐藏
    DTK_WIDGET_NAMESPACE::DTipLabel *m_autoDownloadUpdateTips;
    dcc::widgets::SwitchWidget *m_testingChannel;          // Testing Channel Switch Button
    DTK_WIDGET_NAMESPACE::DTipLabel *m_testingChannelTips; // Testing Channel Description Label
    QLabel *m_testingChannelHeadingLabel;                  // Testing Channel Title
    QLabel *m_testingChannelLinkLabel;                     // Testing Channel Join Link
    QCheckBox *m_idleDownloadCheckBox;                     // 闲时下载更新
    QLabel *m_startTimeLabel;
    QLabel *m_endTimeLabel;
    Dtk::Widget::DTimeEdit *m_startTimeEdit;
    Dtk::Widget::DTimeEdit *m_endTimeEdit;
    QWidget *m_idleTimeDownloadWidget;            // 用于闲时下载设置控件的整体显示和隐藏
    dcc::widgets::SwitchWidget *m_autoCleanCache; // 清理缓存开关
#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    dcc::widgets::SwitchWidget *m_sourceCheck;
#endif
    dcc::widgets::SwitchWidget *m_smartMirrorBtn;  // 智能镜像源
    dcc::update::NextPage *m_updateMirrors; // 设置镜像源
    dcc::widgets::SwitchWidget *m_p2pUpdateSwitch;  // p2p更新
    QWidget *m_p2pUpdateWidget;
};

}// namespace datetime
}// namespace DCC_NAMESPACE
