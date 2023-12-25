// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef MAINCONTROLPANEL_H
#define MAINCONTROLPANEL_H

#include "updatecontrolpanel.h"
#include "safeupdateitem.h"
#include "systemupdateitem.h"
#include "unknownupdateitem.h"
#include "updatemodel.h"
#include "widgets/contentwidget.h"

#include <QWidget>

namespace dcc {
namespace update {

class MainControlPanel : public QWidget {
    Q_OBJECT

public:
    explicit MainControlPanel(UpdateModel* model, QWidget* parent = nullptr);
    ~MainControlPanel();

    void onUpdatesAvailableStatus();

Q_SIGNALS:
    void requestUpdateCtrl(int ctrlType);

public Q_SLOTS:
    void onUpdateStatusChanged(ControlPanelType, UpdatesStatus);
    void onUpdateInfoChanged(UpdateType);
    void onCheckUpdateModeChanged(int);
    void removeControlPanel(ControlPanelType type);
    void onControlTypeChanged();

private:
    void initUi();
    void initConnect();
    void initData();
    void onUpdateModeChanged(quint64 mode);
    void updateContent(UpdateType type, UpdatesStatus status);
    QList<UpdateSettingItem*> getUpdateSettingItems(dcc::widgets::SettingsGroup* group);
    ControlPanelType getControlPanelTypeOfSettingGroup(dcc::widgets::SettingsGroup* group);
    void insertGroutInOrder(ControlPanelType type, UpdateControlPanel* controlPanel, widgets::SettingsGroup* settingGroup);
    void createControlPanel(UpdateType updateType, ControlPanelType contorlPanelType);
    void addItem2Group(UpdateSettingItem* item, widgets::SettingsGroup* group);

private:
    UpdateModel* m_model;
    QMap<ControlPanelType, QPair<UpdateControlPanel*, dcc::widgets::SettingsGroup*>> m_controlPanels;
    QMap<ControlPanelType, QWidget*> m_contentWidgets;
    dcc::ContentWidget* m_updateInfoWidget;
    QMap<UpdateType, QPointer<UpdateSettingItem>> m_updateItems;
    QVBoxLayout* m_contentLayout;
};

}
}

#endif // MAINCONTROLPANEL_H