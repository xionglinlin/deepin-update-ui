// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "interface/namespace.h"
#include "common.h"
#include "widgets/utils.h"
#include "updatesettingitem.h"
#include "updateiteminfo.h"
#include "updatestatusitem.h"
#include "maincontrolpanel.h"

#include <QWidget>
#include <DSpinner>

class AppUpdateInfo;
class QPushButton;

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace dcc {
namespace update {
class UpdateModel;
class SummaryItem;
class TipsItem;
class SystemUpdateItem;
class SecurityUpdateItem;
class UnknownUpdateItem;
class UpdateSettingItem;
}

namespace widgets {
class SettingsGroup;
class TipsLabel;
}
}

using namespace dcc::update;


namespace DCC_NAMESPACE {
namespace update {

class UpdateCtrlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UpdateCtrlWidget(UpdateModel *model, QWidget *parent = 0);
    ~UpdateCtrlWidget();

    const MainControlPanel* mainControlPanel() const { return m_mainControlPanel; }

Q_SIGNALS:
    void notifyUpdateState(int);

private Q_SLOTS:
    void onUpdateModeChanged(quint64 mode);

private:
    void initUI();
    void initConnect();
    void setModel(UpdateModel *model);
    void setStatus(const UpdatesStatus &status);
    void setUpdateProgress(const double value);
    void updateWidgetsVisible();
    QString getPrivacyPolicyLink() const;

private:
    UpdateModel *m_model;
    UpdatesStatus m_updateStatus;
    UpdateStatusItem *m_checkUpdateItem;
    QWidget *m_checkUpdateWidget;
    TipsItem *m_resultItem;
    QString m_systemVersion;
    DLabel *m_versionTip;
    qlonglong m_updateSize;
    DLabel *m_privacyPolicyLabel;
    MainControlPanel *m_mainControlPanel;
};

}// namespace datetime
}// namespace DCC_NAMESPACE
