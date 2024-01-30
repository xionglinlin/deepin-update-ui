// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "maincontrolpanel.h"

#include <QVBoxLayout>

#include <DDBusSender>

using namespace dcc::widgets;

namespace dcc {
namespace update {

inline static void removeItemAndNotDestruct(SettingsGroup *group,SettingsItem *item)
{
    if (!item || !group)
        return;

    group->getLayout()->removeWidget(item);
    item->removeEventFilter(group);
    item->setParent(nullptr);
}

MainControlPanel::MainControlPanel(UpdateModel* model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
    , m_updateInfoWidget(new ContentWidget(parent))
    , m_contentLayout(new QVBoxLayout)
{
    initUi();
    initConnect();
    initData();
}

MainControlPanel::~MainControlPanel()
{
    for (auto item : m_updateItems.values()) {
        item->deleteLater();
        item = nullptr;
    }
}

void MainControlPanel::initUi()
{
    m_updateInfoWidget->setAccessibleName("UpdateInfoWidget");

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(m_updateInfoWidget, 1);
    setLayout(layout);

    m_contentLayout->addStretch(1);
    QWidget* contentWidget = new QWidget;
    contentWidget->setAccessibleName("UpdateCtrlWidget_contentWidget");
    contentWidget->setLayout(m_contentLayout);
    contentWidget->setContentsMargins(10, 0, 10, 0);

    m_updateInfoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_updateInfoWidget->setContent(contentWidget);
}

void MainControlPanel::initConnect()
{
    connect(m_model, &UpdateModel::updateInfoChanged, this, &MainControlPanel::onUpdateInfoChanged);
    connect(m_model, &UpdateModel::updateModeChanged, this, &MainControlPanel::onUpdateModeChanged);
    connect(m_model, &UpdateModel::updateStatusChanged, this, &MainControlPanel::onUpdateStatusChanged);
    connect(m_model, &UpdateModel::controlTypeChanged, this, &MainControlPanel::onControlTypeChanged);
    connect(m_model, &UpdateModel::checkUpdateModeChanged, this, &MainControlPanel::onCheckUpdateModeChanged);
}

void MainControlPanel::initData()
{
    for (auto type : m_model->allDownloadInfo().keys()) {
        onUpdateInfoChanged(type);
    }

    for (auto type : m_model->controlTypes()) {
        onUpdateStatusChanged(type, m_model->updateStatus(type));
    }

    onCheckUpdateModeChanged(m_model->checkUpdateMode());
}

void MainControlPanel::onUpdateInfoChanged(UpdateType type)
{
    if (!UpdateModel::isSupportedUpdateType(type))
        return;

    const auto updateItemInfo = m_model->updateItemInfo(type);
    if (!updateItemInfo || (updateItemInfo->packages().isEmpty() && updateItemInfo->updateStatus() != UpgradeSuccess)) {
        qCInfo(DCC_UPDATE) << "Update is not available, type:" << type;
        return;
    }

    if (!m_updateItems.contains(type)) {
        switch (type) {
        case UpdateType::SystemUpdate:
            m_updateItems.insert(SystemUpdate, new SystemUpdateItem);
            break;
        case UpdateType::SecurityUpdate:
            m_updateItems.insert(SecurityUpdate, new SecurityUpdateItem);
            break;
        case UpdateType::UnknownUpdate:
            m_updateItems.insert(UnknownUpdate, new UnknownUpdateItem);
            break;
        default:
            break;
        }
    }

    auto updateItem = m_updateItems.value(type);
    if (!updateItem)
        return;

    updateItem->setVisible(false);
    auto updateInfo = m_model->updateItemInfo(type);
    if (!updateInfo) {
        return;
    }

    updateItem->setIconVisible(true);
    updateItem->setData(updateInfo);
    updateItem->setChecked(m_model->checkUpdateMode() & updateItem->updateType());
    updateContent(type, m_model->updateStatus(type));
}

void MainControlPanel::onUpdateModeChanged(quint64 mode)
{
    // 设置更新类型的显示状态
    for (auto item : m_updateItems.values()) {
        if (!item) {
            continue;
        }

        auto info = m_model->updateItemInfo(item->updateType());
        item->setVisible(item->parent() && info && info->isUpdateModeEnabled());
    }

    // 设置控制窗口的显示状态
    for (auto controlType : m_controlPanels.keys()) {
        auto widget = m_contentWidgets.value(controlType, nullptr);
        if (widget) {
            widget->setVisible((m_model->updateTypes(controlType) & mode) > 0);
        }
    }
}

void MainControlPanel::onUpdatesAvailableStatus()
{
    onUpdateModeChanged(m_model->updateMode());
    m_model->updateCheckUpdateTime();
}

void MainControlPanel::onUpdateStatusChanged(ControlPanelType type, UpdatesStatus status)
{
    for (auto updateType : m_model->updateTypesList(type)) {
        updateContent(updateType, status);
    }
}

// 按照重启、安装、下载、可用的顺序插入
void MainControlPanel::insertGroutInOrder(ControlPanelType type, UpdateControlPanel* controlPanel, SettingsGroup* settingGroup)
{
    auto contentWidget = new QWidget(this);
    auto controlLayout = new QVBoxLayout(contentWidget);
    controlLayout->setSpacing(0);
    controlLayout->setMargin(0);
    controlLayout->addWidget(controlPanel);
    controlLayout->addSpacing(6);
    controlLayout->addWidget(settingGroup);
    controlLayout->addSpacing(10);
    m_contentWidgets.insert(type, contentWidget);
    int index = 0;
    for (auto key : m_controlPanels.keys()) {
        if (type <= key) {
            break;
        }
        index++;
    }
    m_contentLayout->insertWidget(index, contentWidget, 0);
}

void MainControlPanel::createControlPanel(UpdateType updateType, ControlPanelType contorlPanelType)
{
    qCInfo(DCC_UPDATE) << "Init control panel, update type:" << updateType << ", control panel type:" << contorlPanelType;
    auto updateControlPanel = new UpdateControlPanel(this);
    auto settingsGroup = new SettingsGroup;
    connect(updateControlPanel, &UpdateControlPanel::PauseDownload, this, [this] {
        Q_EMIT requestUpdateCtrl(UpdateCtrlType::Pause);
    });
    connect(updateControlPanel, &UpdateControlPanel::StartDownload, this, [this] {
        Q_EMIT requestUpdateCtrl(UpdateCtrlType::Start);
    });
    connect(updateControlPanel, &UpdateControlPanel::requestSetUpdateItemCheckBoxEnabled, this, [this, contorlPanelType](bool enabled) {
        for (auto updateType : m_model->updateTypesList(contorlPanelType)) {
            auto item = m_updateItems.value(updateType, nullptr);
            if (item) {
                item->setCheckBoxEnabled(enabled);
            }
        }
    });

    m_controlPanels.insert(contorlPanelType, qMakePair(updateControlPanel, settingsGroup));
    insertGroutInOrder(contorlPanelType, updateControlPanel, settingsGroup);
    updateControlPanel->setControlPanelType(contorlPanelType);
    updateControlPanel->setModel(m_model);
}

void MainControlPanel::addItem2Group(UpdateSettingItem* item, SettingsGroup* group)
{
    if (!item || !group)
        return;

    item->setVisible(true);
    if (getUpdateSettingItems(group).contains(item)) {
        return;
    }

    // 按照系统更新、安全更新、第三方更新的顺序插入
    qCInfo(DCC_UPDATE) << "Add item to control panel, update type:" << item->updateType()
            << ", control panel:" << getControlPanelTypeOfSettingGroup(group);
    if (item->updateType() == SystemUpdate) {
        group->insertItem(0, item);
    } else if (item->updateType() == UnknownUpdate) {
        group->appendItem(item);
    } else {
        group->insertItem(1, item);
    }

    auto updateControlPanel = m_controlPanels.value(getControlPanelTypeOfSettingGroup(group)).first;
    if (updateControlPanel) {
        item->setCheckBoxEnabled(updateControlPanel->getCheckBoxEnabledState());
    }

    // 从其他的group中移除
    // 如果group中没有item或者item是不可更新的状态么，那么移除整个模块
    for (const auto key : m_controlPanels.keys()) {
        auto controlPanel = m_controlPanels.value(key).first;
        auto settingGroup = m_controlPanels.value(key).second;
        if (settingGroup != group) {
            bool groupIsValid = false;
            for (const auto settingItem : getUpdateSettingItems(settingGroup)) {
                if (settingItem == item) {
                    qCInfo(DCC_UPDATE) << "Remove item " << settingItem->updateType() << " from panel:" << controlPanel->controlPanelType();
                    removeItemAndNotDestruct(settingGroup, settingItem);
                    settingItem->setVisible(false);
                } else if (settingItem->updateItemInfo() && settingItem->updateItemInfo()->isUpdateAvailable()) {
                    groupIsValid = true;
                }
            }

            if (!groupIsValid) {
                qCInfo(DCC_UPDATE) << "There's no available item in group, remove the panel:" << controlPanel->controlPanelType();
                removeControlPanel(key);
            }
        }
    }
}

void MainControlPanel::updateContent(UpdateType updateType, UpdatesStatus status)
{
    // 如果这个类型没有可更新内容则不显示
    const auto updateItemInfo = m_model->updateItemInfo(updateType);
    if (!updateItemInfo || !updateItemInfo->isUpdateAvailable()) {
        qCInfo(DCC_UPDATE) << "Update is not available, won't handle it, type:" << updateType;
        return;
    }

    const auto controlType = UpdateModel::getControlPanelType(status);
    if (controlType == CPT_Invalid) {
        qCInfo(DCC_UPDATE) << "Control type is invalid";
        return;
    }

    if (!m_controlPanels.contains(controlType)) {
        createControlPanel(updateType, controlType);
    }

    addItem2Group(m_updateItems.value(updateType), m_controlPanels.value(controlType).second);
}

void MainControlPanel::onCheckUpdateModeChanged(int checkMode)
{
    // 更新check状态
    for (auto item : m_updateItems.values()) {
        item->setChecked(checkMode & item->updateType());
    }
}

QList<UpdateSettingItem*> MainControlPanel::getUpdateSettingItems(dcc::widgets::SettingsGroup* group)
{
    QList<UpdateSettingItem*> list = {};
    if (!group)
        return list;

    for (int i = 0; i < group->itemCount(); i++) {
        auto item = qobject_cast<UpdateSettingItem*>(group->getItem(i));
        if (!item)
            continue;

        list.append(item);
    }

    return list;
}

ControlPanelType MainControlPanel::getControlPanelTypeOfSettingGroup(dcc::widgets::SettingsGroup* group)
{
    auto it = m_controlPanels.begin();
    for (; it != m_controlPanels.end(); it++) {
        if (it.value().second == group) {
            return it.key();
        }
    }

    return CPT_Invalid;
}

void MainControlPanel::removeControlPanel(ControlPanelType type)
{
    qCInfo(DCC_UPDATE) << "Control type:" << type;
    auto it = m_controlPanels.find(type);
    if (it == m_controlPanels.end()) {
        qCWarning(DCC_UPDATE) << "Can not find control panel type:" << type;
        return;
    }

    if (it->first) {
        delete it->first;
        it->first = nullptr;
    }
    if (it->second) {
        // 从group中移除item，避免自动析构了
        for (const auto settingItem : getUpdateSettingItems(it->second)) {
            removeItemAndNotDestruct(it->second, settingItem);
            settingItem->setVisible(false);
        }

        delete it->second;
        it->second = nullptr;
    }

    auto layoutItem = m_contentWidgets.value(type, nullptr);
    if (layoutItem) {
        m_contentLayout->removeWidget(layoutItem);
        layoutItem->deleteLater();
    }
    m_controlPanels.remove(type);
}

void MainControlPanel::onControlTypeChanged()
{
    const auto controlTypes = m_model->controlTypes();
    for (auto type : m_controlPanels.keys()) {
        if (!controlTypes.contains(type)) {
            removeControlPanel(type);
        }
    }

    for (auto type : controlTypes) {
        onUpdateStatusChanged(type, m_model->updateStatus(type));
    }
}

}
}
