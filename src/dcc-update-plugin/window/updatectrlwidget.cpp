// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatectrlwidget.h"
#include "tipsitem.h"
#include "updatemodel.h"
#include "updatestatusitem.h"
#include "widgets/labels/tipslabel.h"
#include "widgets/settingsgroup.h"
#include "window/utils.h"

#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QVBoxLayout>

#include <DConfig>
#include <DDBusSender>
#include <DDialog>
#include <DFontSizeManager>
#include <DPalette>
#include <DSysInfo>

#include <types/appupdateinfolist.h>

#define UpgradeWarningSize 500
#define FullUpdateBtnWidth 120

using namespace dcc;
using namespace dcc::update;
using namespace dcc::widgets;
using namespace DCC_NAMESPACE;
using namespace DCC_NAMESPACE::update;
using namespace Dtk::Widget;

const static QMap<UpdatesStatus, CtrlWidgetType> CtrlWidgetTypeMapping = {
    {Default, CtrlState_None},
    {Checking, CtrlState_Check},
    {CheckingFailed, CtrlState_Check},
    {CheckingSucceed, CtrlState_Update},
    {Updated, CtrlState_Check},
    {UpdatesAvailable, CtrlState_Update},
    {DownloadWaiting, CtrlState_Update},
    {Downloading, CtrlState_Update},
    {DownloadPaused, CtrlState_Update},
    {DownloadFailed, CtrlState_Update},
    {Downloaded, CtrlState_Update},
    {UpgradeWaiting, CtrlState_Update},
    {BackingUp, CtrlState_Update},
    {BackupFailed, CtrlState_Update},
    {BackupSuccess, CtrlState_Update},
    {UpgradeReady, CtrlState_Update},
    {Upgrading, CtrlState_Update},
    {UpgradeFailed, CtrlState_Update},
    {UpgradeSuccess, CtrlState_Update}
};

UpdateCtrlWidget::UpdateCtrlWidget(UpdateModel* model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
    , m_updateStatus(Default)
    , m_checkUpdateItem(new UpdateStatusItem(model))
    , m_checkUpdateWidget(new QWidget(this))
    , m_resultItem(new TipsItem())
    , m_versionTip(new DLabel(parent))
    , m_updateSize(0)
    , m_privacyPolicyLabel(new DLabel(this))
    , m_mainControlPanel(new MainControlPanel(model, this))
{
    initUI();
    setModel(model);
    initConnect();
}

void UpdateCtrlWidget::initUI()
{
    setAccessibleName("UpdateCtrlWidget");
    m_checkUpdateItem->setAccessibleName("checkUpdateItem");

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    DFontSizeManager::instance()->bind(m_versionTip, DFontSizeManager::T8);
    m_versionTip->setForegroundRole(DPalette::BrightText);
    m_versionTip->setEnabled(false);
    updateVersion();

    QHBoxLayout* resultItemLayout = new QHBoxLayout(m_checkUpdateWidget);
    resultItemLayout->setContentsMargins(QMargins(15, 0, 15, 0));
    resultItemLayout->addWidget(m_checkUpdateItem, 1);

    DFontSizeManager::instance()->bind(m_privacyPolicyLabel, DFontSizeManager::T8);
    m_privacyPolicyLabel->setAlignment(Qt::AlignHCenter);
    m_privacyPolicyLabel->setForegroundRole(DPalette::TextTips);
    m_privacyPolicyLabel->setTextFormat(Qt::RichText);
    m_privacyPolicyLabel->setWordWrap(true);
    m_privacyPolicyLabel->setOpenExternalLinks(true);
    m_privacyPolicyLabel->setText(tr("To use this software, you must accept the %1 that accompanies software updates.").arg(getPrivacyPolicyLink()));

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->setSpacing(10);
    layout->addSpacing(10);
    layout->addWidget(m_versionTip, 0, Qt::AlignHCenter);
    layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(m_resultItem, 1);
    layout->addWidget(m_checkUpdateWidget, 1);
    layout->addWidget(m_mainControlPanel, 1);
    layout->addStretch();
    layout->addWidget(m_privacyPolicyLabel);
    layout->addSpacing(10);

    setLayout(layout);
}

void UpdateCtrlWidget::initConnect()
{
    connect(m_checkUpdateItem, &UpdateStatusItem::requestCheckUpdate, m_model, &UpdateModel::beginCheckUpdate);
    connect(m_model, &UpdateModel::systemActivationChanged, this, [this] (UiActiveState state) {
        if (!m_model->isActivationValid())
            setStatus(Default);
    });
}

UpdateCtrlWidget::~UpdateCtrlWidget()
{
}

void UpdateCtrlWidget::setStatus(const UpdatesStatus& status)
{
    qCInfo(DCC_UPDATE) << "Status: " << status;
    if (m_updateStatus == Checking && (status == Default || status > CheckingSucceed)) {
        qCInfo(DCC_UPDATE) << "Checking, do not handle this status";
        return;
    }
    m_updateStatus = status;
    Q_EMIT notifyUpdateState(m_updateStatus);

    // 下面的几种状态需要拦截正常的更新状态
    UpdatesStatus tmpStats = m_updateStatus;
    if (!m_model->isActivationValid())
        tmpStats = SystemIsNotActive;

    if (LastoreDaemonDConfigStatusHelper::isUpdateDisabled(m_model->lastoreDaemonStatus()))
        tmpStats = UpdateIsDisabled;

    if (m_model->updateMode() == UpdateType::Invalid)
        tmpStats = AllUpdateModeDisabled;

    updateWidgetsVisible();
    if (tmpStats >= SystemIsNotActive) {
        m_mainControlPanel->setVisible(false);
        switch (tmpStats) {
        case UpdatesStatus::SystemIsNotActive:
            qCInfo(DCC_UPDATE) << "System is not active";
            m_versionTip->setVisible(false);
            m_resultItem->setVisible(true);
            m_resultItem->setStatus(UpdatesStatus::SystemIsNotActive);
            break;
        case UpdatesStatus::UpdateIsDisabled:
            qCInfo(DCC_UPDATE) << "Update is disabled";
            m_versionTip->setVisible(false);
            m_resultItem->setVisible(true);
            m_resultItem->setStatus(UpdatesStatus::UpdateIsDisabled);
            break;
        case UpdatesStatus::AllUpdateModeDisabled:
            qCInfo(DCC_UPDATE) << "All update mode is disabled";
            m_checkUpdateWidget->setVisible(true);
            m_checkUpdateItem->setStatus(AllUpdateModeDisabled);
            break;
        default:
            break;
        }

        return;
    }

    auto ctrlState = CtrlWidgetTypeMapping.value(m_updateStatus, CtrlState_Check);

    // 更新流程中
    switch (ctrlState) {
    case CtrlState_Check:
        m_checkUpdateWidget->setVisible(true);
        m_mainControlPanel->setVisible(false);
        m_checkUpdateItem->setStatus(m_updateStatus);
        break;
    case CtrlState_Update:
        m_mainControlPanel->setVisible(true);
        m_privacyPolicyLabel->setVisible(true);
        break;
    default:
        break;
    }
}

void UpdateCtrlWidget::setUpdateProgress(const double value)
{
    m_checkUpdateItem->setProgressValue(static_cast<int>(value * 100));
}

void UpdateCtrlWidget::setModel(UpdateModel* model)
{
    m_model = model;

    qRegisterMetaType<UpdateErrorType>("UpdateErrorType");

    connect(m_model, &UpdateModel::updateStatusChanged, this, [this] (ControlPanelType, UpdatesStatus status) {
        setStatus(status);
    });
    connect(m_model, &UpdateModel::lastStatusChanged, this, &UpdateCtrlWidget::setStatus);
    connect(m_model, &UpdateModel::updateProgressChanged, this, &UpdateCtrlWidget::setUpdateProgress);
    connect(m_model, &UpdateModel::updateModeChanged, this, &UpdateCtrlWidget::onUpdateModeChanged);

    setUpdateProgress(m_model->checkUpdateProgress());

    if (m_model->enterCheckUpdate()) {
        setStatus(UpdatesStatus::Checking);
    } else {
        setStatus(m_model->lastStatus());
    }
    connect(m_model, &UpdateModel::systemVersionChanged, this, &UpdateCtrlWidget::updateVersion);
    connect(m_model, &UpdateModel::baselineChanged, this, &UpdateCtrlWidget::updateVersion);
}

void UpdateCtrlWidget::onUpdateModeChanged(quint64 mode)
{
    // 在Checking和CheckingFailed状态时，无法判断是否当前系统是否是最新的
    if (m_updateStatus != Checking && m_updateStatus != CheckingFailed && m_model->isUpdateToDate()) {
        setStatus(Updated);
    } else {
        setStatus(m_model->lastStatus());
    }
}

void UpdateCtrlWidget::updateVersion()
{
    QString minorVersion;
    if (m_model->showVersion() == "baseline" && !m_model->baseline().isEmpty()) {
        minorVersion = m_model->baseline();
    } else {
        minorVersion = Dtk::Core::DSysInfo::minorVersion();
    }
    QString sVersion = QString("%1 %2").arg(Dtk::Core::DSysInfo::uosProductTypeName()).arg(minorVersion);
    m_versionTip->setText(tr("Current Edition") + ": " + sVersion);
}

void UpdateCtrlWidget::updateWidgetsVisible()
{
    m_versionTip->setVisible(true);
    m_resultItem->setVisible(false);
    m_checkUpdateWidget->setVisible(false);
    m_privacyPolicyLabel->setVisible(false);
}

QString UpdateCtrlWidget::getPrivacyPolicyLink() const
{
    static QStringList supportedRegion = { "cn", "en", "tw", "hk", "ti", "uy" };
    static QStringList communitySupportRegion = { "cn", "en" };
    const QString& systemLocaleName = QLocale::system().name();
    if (systemLocaleName.length() != QString("zh_CN").length()) {
        qCWarning(DCC_UPDATE) << "Get system locale name failed:" << systemLocaleName;
    }
    QString region = systemLocaleName.right(2).toLower();

    QString addr = "https://www.uniontech.com/agreement/privacy-";
    if (DCC_NAMESPACE::IsCommunitySystem) {
        addr = "https://www.uniontech.com/agreement/deepin-privacy-";
        if (!communitySupportRegion.contains(region))
            region = "en";
    } else if (!supportedRegion.contains(region)) {
        region = "en";
    }

    return QString("<a style=text-decoration:none href = %1%2>%3</a>").arg(addr).arg(region).arg(tr("Privacy Policy"));
}
