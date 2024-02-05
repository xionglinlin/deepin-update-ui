// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatecontrolpanel.h"
#include "signalbridge.h"
#include "widgets/basiclistdelegate.h"
#include "updatelogdialog.h"

#include <DDBusSender>
#include <DDialog>
#include <DFontSizeManager>
#include <DHiDPIHelper>
#include <DToolTip>

using namespace dcc;
using namespace dcc::widgets;
using namespace dcc::update;
using namespace Dtk::Widget;
DCORE_USE_NAMESPACE

#define FullUpdateBtnWidth 120

enum ControlWidget {
    PROGRESS_WIDGET = 0,
    DOWNLOAD_BUTTON,
    RETRY_BUTTON,
    UPGRADE_BUTTON,
    SPINNER_WIDGET,
    CONTINUE_UPGRADE,
    REBOOT_BUTTON,
};

enum TextLabelType {
    DownloadSize = 0,
    InfoTips,
    ErrorTips,
    ShowLogDialog,
};

int UpdateControlPanel::InstallProgressBeginValue = 0;
static const int BACKUP_START_PROGRESS = 20;
static const int BACKUP_SUCCESS_PROGRESS = 50;

UpdateControlPanel::UpdateControlPanel(QWidget* parent)
    : QWidget { parent }
    , m_successIcon(new QLabel(this))
    , m_updateTipsLab(new DLabel(this))
    , m_updateSizeLab(new DLabel(this))
    , m_progressLabel(new DLabel(this))
    , m_showLogButton(new NoMarginDCommandLinkButton(tr("View logs"), this))
    , m_tipLabelWidget(new SingleContentWidget(this))
    , m_progressLabelRightSpacer(new QSpacerItem(0, 0))
    , m_errorTipsLabel(new DLabel(this))
    , m_infoTipsLabel(new DLabel(this))
    , m_controlWidget(new SingleContentWidget(this))
    , m_startButton(new DIconButton(this))
    , m_stopButton(new DIconButton(this))
    , m_progress(new DProgressBar(this))
    , m_downloadingWidget(new QWidget(this))
    , m_downloadBtn(new QPushButton(this))
    , m_retryBtn(new QPushButton(this))
    , m_updateBtn(new QPushButton(this))
    , m_continueUpgrade(new QPushButton(this))
    , m_rebootButton(new QPushButton(this))
    , m_reBackupButton(new QPushButton(this))
    , m_spinner(new DSpinner(this))
    , m_spinnerWidget(new QWidget(this))
    , m_buttonStatus(ButtonStatus::invalid)
    , m_updateStatus(Default)
    , m_model(nullptr)
{
    initUi();
    initConnect();
}

UpdateControlPanel::~UpdateControlPanel()
{
}

void UpdateControlPanel::initUi()
{
    m_successIcon->setFixedSize(20, 20);
    const auto iconPath = ":/update/themes/common/icons/success.svg";
    const auto iconSize = QSize(20 * qApp->devicePixelRatio(), 20 * qApp->devicePixelRatio());
    m_successIcon->setPixmap(DHiDPIHelper::loadNxPixmap(iconPath).scaled(iconSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    m_successIcon->setVisible(false);

    m_updateTipsLab->setText(tr("Updates Available"));
    DFontSizeManager::instance()->bind(m_updateTipsLab, DFontSizeManager::T5, QFont::DemiBold);
    m_updateTipsLab->setForegroundRole(DPalette::TextTitle);

    DFontSizeManager::instance()->bind(m_updateSizeLab, DFontSizeManager::T8);
    m_updateSizeLab->setForegroundRole(DPalette::TextTips);
    m_updateSizeLab->setElideMode(Qt::ElideRight);
    DToolTip::setToolTipShowMode(m_updateSizeLab, DToolTip::ShowWhenElided);

    DFontSizeManager::instance()->bind(m_errorTipsLabel, DFontSizeManager::T8);
    QPalette palette = m_errorTipsLabel->palette();
    palette.setColor(QPalette::WindowText, "#FF5736");
    m_errorTipsLabel->setPalette(palette);
    m_errorTipsLabel->setWordWrap(true);
    m_errorTipsLabel->setAlignment(Qt::AlignLeft);
    m_errorTipsLabel->setElideMode(Qt::ElideRight);
    DToolTip::setToolTipShowMode(m_errorTipsLabel, DToolTip::ShowWhenElided);

    DFontSizeManager::instance()->bind(m_infoTipsLabel, DFontSizeManager::T8);
    m_infoTipsLabel->setForegroundRole(DPalette::TextTips);
    m_infoTipsLabel->setWordWrap(true);
    m_infoTipsLabel->setAlignment(Qt::AlignLeft);
    m_infoTipsLabel->setElideMode(Qt::ElideRight);
    DToolTip::setToolTipShowMode(m_infoTipsLabel, DToolTip::ShowWhenElided);

    DFontSizeManager::instance()->bind(m_showLogButton, DFontSizeManager::T8);

    auto titleLayout = new QHBoxLayout;
    titleLayout->setSpacing(8);
    titleLayout->addSpacing(8);
    titleLayout->addWidget(m_successIcon);
    titleLayout->addWidget(m_updateTipsLab);
    titleLayout->addStretch(1);

    m_tipLabelWidget->insertWidget(DownloadSize, m_updateSizeLab);
    m_tipLabelWidget->insertWidget(ErrorTips, m_errorTipsLabel);
    m_tipLabelWidget->insertWidget(InfoTips, m_infoTipsLabel);
    m_tipLabelWidget->insertWidget(ShowLogDialog, m_showLogButton, Qt::AlignLeft);
    auto tipLabelLayout = new QHBoxLayout;
    tipLabelLayout->setMargin(0);
    tipLabelLayout->setSpacing(0);
    tipLabelLayout->addSpacing(8);
    tipLabelLayout->addWidget(m_tipLabelWidget);

    QVBoxLayout* updateTitleFirstVLay = new QVBoxLayout;
    updateTitleFirstVLay->setMargin(0);
    updateTitleFirstVLay->setSpacing(0);
    updateTitleFirstVLay->addStretch(1);
    updateTitleFirstVLay->addLayout(titleLayout);
    updateTitleFirstVLay->addSpacing(5);
    updateTitleFirstVLay->addLayout(tipLabelLayout);
    updateTitleFirstVLay->addStretch(1);

    m_progress->setFixedHeight(8);
    m_progress->setRange(0, 100);
    m_progress->setAlignment(Qt::AlignRight);
    m_progress->setFixedWidth(240);
    m_progress->setVisible(true);
    m_progress->setValue(0);

    QHBoxLayout* buttonLay = new QHBoxLayout();
    m_startButton->setIcon(QIcon::fromTheme("dcc_start"));
    m_startButton->setIconSize(QSize(24, 24));
    m_startButton->setFlat(true);
    m_startButton->setFixedSize(24, 24);

    m_stopButton->setIcon(QIcon::fromTheme("dcc_stop"));
    m_stopButton->setIconSize(QSize(24, 24));
    m_stopButton->setFlat(true);
    m_stopButton->setFixedSize(24, 24);

    buttonLay->setSpacing(0);
    buttonLay->setMargin(0);
    buttonLay->addWidget(m_progress);
    buttonLay->addSpacing(6);
    buttonLay->addWidget(m_startButton);
    buttonLay->addSpacing(10);
    buttonLay->addWidget(m_stopButton);

    DFontSizeManager::instance()->bind(m_progressLabel, DFontSizeManager::T8);
    m_progressLabel->setScaledContents(true);
    m_progressLabel->setAlignment(Qt::AlignRight);
    m_progressLabelRightSpacer->changeSize(m_startButton->width() + m_stopButton->width() + 16, 0);
    QHBoxLayout* progressLabelLayout = new QHBoxLayout();
    progressLabelLayout->addStretch(1);
    progressLabelLayout->setSpacing(0);
    progressLabelLayout->setMargin(0);
    progressLabelLayout->addWidget(m_progressLabel);
    progressLabelLayout->addSpacerItem(m_progressLabelRightSpacer);

    QVBoxLayout* downloadingLayout = new QVBoxLayout();
    downloadingLayout->addSpacing(0);
    downloadingLayout->setMargin(0);
    downloadingLayout->addStretch();
    downloadingLayout->addLayout(buttonLay);
    downloadingLayout->addLayout(progressLabelLayout);
    downloadingLayout->addStretch();
    m_downloadingWidget->setLayout(downloadingLayout);

    auto initButton = [this](QPushButton* button, const QString& text) {
        button->setText(getElidedText(button, text, Qt::ElideRight, FullUpdateBtnWidth - 10));
        button->setFixedSize(FullUpdateBtnWidth, 36);
        button->setToolTip(text);
    };
    initButton(m_downloadBtn, tr("Download All"));
    initButton(m_retryBtn, tr("Try Again"));
    initButton(m_updateBtn, tr("Install Now"));
    initButton(m_continueUpgrade, tr("Proceed to Update"));
    initButton(m_rebootButton, tr("Reboot now"));
    initButton(m_downloadBtn, tr("Download All"));
    initButton(m_reBackupButton, tr("Try Again"));
    m_reBackupButton->setVisible(false);

    m_spinner->setFixedSize(24, 24);
    QHBoxLayout* spinnerLayout = new QHBoxLayout(m_spinnerWidget);
    spinnerLayout->addStretch(1);
    spinnerLayout->addWidget(m_spinner);

    m_controlWidget->insertWidget(PROGRESS_WIDGET, m_downloadingWidget);
    m_controlWidget->insertWidget(DOWNLOAD_BUTTON, m_downloadBtn);
    m_controlWidget->insertWidget(RETRY_BUTTON, m_retryBtn);
    m_controlWidget->insertWidget(UPGRADE_BUTTON, m_updateBtn);
    m_controlWidget->insertWidget(SPINNER_WIDGET, m_spinnerWidget);
    m_controlWidget->insertWidget(CONTINUE_UPGRADE, m_continueUpgrade);
    m_controlWidget->insertWidget(REBOOT_BUTTON, m_rebootButton);
    m_controlWidget->setCurrentIndex(DOWNLOAD_BUTTON);

    QHBoxLayout* main = new QHBoxLayout();
    main->setSpacing(0);
    main->setMargin(0);
    main->addLayout(updateTitleFirstVLay, 1);
    main->addWidget(m_reBackupButton);
    main->addSpacing(10);
    main->addWidget(m_controlWidget, 0);

    setLayout(main);
}

void UpdateControlPanel::setModel(UpdateModel* model)
{
    m_model = model;
    connect(m_model, &UpdateModel::downloadProgressChanged, this, &UpdateControlPanel::setProgressValue);
    connect(m_model, &UpdateModel::updateModeChanged, this, &UpdateControlPanel::refreshDownloadSize);
    connect(m_model, &UpdateModel::notifyDownloadSizeChanged, this, &UpdateControlPanel::refreshDownloadSize);
    connect(m_model, &UpdateModel::checkUpdateModeChanged, this, &UpdateControlPanel::onCheckUpdateModeChanged);
    connect(m_model, &UpdateModel::lastoreDaemonStatusChanged, this, &UpdateControlPanel::updateWidgets);
    connect(m_model, &UpdateModel::updateStatusChanged, this, &UpdateControlPanel::onUpdateStatusChanged);
    connect(m_model, &UpdateModel::distUpgradeProgressChanged, this, &UpdateControlPanel::onUpgradeProgressChanged);
    connect(m_model, &UpdateModel::lastStatusChanged, this, &UpdateControlPanel::updateCurrentWidgetEnabledState);
    connect(m_model, &UpdateModel::controlTypeChanged, this, [this] {
        refreshDownloadSize();
        setUpdateStatus(m_model->updateStatus(m_controlPanelType));
        updateCurrentWidgetEnabledState();
    });
    connect(m_model, &UpdateModel::lastErrorChanged, this, [this](UpdatesStatus status, UpdateErrorType error) {
        if (m_model->updateStatus(m_controlPanelType) != status) {
            return;
        }
        updateWidgets();
    });
    connect(m_model, &UpdateModel::batterStatusChanged, this, &UpdateControlPanel::onBatteryStatusChanged);
    connect(m_model, &UpdateModel::notifyBackupSuccess, this, [this] {
        qCInfo(DCC_UPDATE) << "Backup success";
        setInstallProgressBeginValue(true);
    });
    refreshDownloadSize();
    setProgressValue(m_model->downloadProgress());
    setEnabled(m_model->updateMode() >= UpdateType::SystemUpdate);
    onCheckUpdateModeChanged(m_model->checkUpdateMode());
    setUpdateStatus(m_model->updateStatus(m_controlPanelType));
    updateCurrentWidgetEnabledState();
    onBatteryStatusChanged(m_model->batterIsOK());
    if (m_model->getLastoreDaemonStatus().backupStatus == BackupSuccess) {
        setInstallProgressBeginValue(true);
    }
    onUpgradeProgressChanged(m_model->distUpgradeProgress());
}

void UpdateControlPanel::initConnect()
{
    connect(m_startButton, &DIconButton::clicked, this, &UpdateControlPanel::onCtrlButtonClicked);
    connect(m_stopButton, &DIconButton::clicked, &SignalBridge::ref(), &SignalBridge::requestStopDownload);
    connect(m_downloadBtn, &QPushButton::clicked, this, [this] {
        setButtonStatus(ButtonStatus::pause);
        setProgressValue(0);
        startSpinner();
        Q_EMIT SignalBridge::ref().requestDownload(updateTypes() & m_model->checkUpdateMode());
    });
    connect(m_retryBtn, &QPushButton::clicked, this, [this] {
        setProgressValue(0);
        startSpinner();
        Q_EMIT SignalBridge::ref().requestRetry(m_controlPanelType, updateTypes() & m_model->checkUpdateMode());
    });
    connect(m_updateBtn, &QPushButton::clicked, this, &UpdateControlPanel::onUpdateButtonClicked);
    connect(m_continueUpgrade, &QPushButton::clicked, this, [this] {
        startSpinner();
        Q_EMIT SignalBridge::ref().requestBackgroundInstall(updateTypes() & m_model->checkUpdateMode(), false);
    });
    connect(m_rebootButton, &QPushButton::clicked, this, [] {
        DDBusSender()
            .service("com.deepin.dde.shutdownFront")
            .interface("com.deepin.dde.shutdownFront")
            .path("/com/deepin/dde/shutdownFront")
            .method("Restart")
            .call();
    });
    connect(m_reBackupButton, &QPushButton::clicked, this, [this] {
        setProgressValue(0);
        startSpinner();
        Q_EMIT SignalBridge::ref().requestRetry(m_controlPanelType, updateTypes() & m_model->checkUpdateMode());
    });
    connect(m_showLogButton, &DCommandLinkButton::clicked, this, [this] {
        auto dialog = new UpdateLogDialog(m_model->lastErrorLog(m_updateStatus), this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
    });
}

void UpdateControlPanel::setProgressValue(double progress)
{
    if (m_controlPanelType != CPT_Downloading) {
        return;
    }

    int value = progress * 100;
    if (value < 0 || value > 100)
        return;

    m_progress->setValue(value);
    setProgressText(QString("%1%").arg(value));
}

void UpdateControlPanel::setButtonIcon(ButtonStatus status)
{
    switch (status) {
    case ButtonStatus::start:
        m_startButton->setIcon(QIcon::fromTheme("dcc_start"));
        break;
    case ButtonStatus::pause:
        m_startButton->setIcon(QIcon::fromTheme("dcc_pause"));
        break;
    case ButtonStatus::retry:
        m_startButton->setIcon(QIcon::fromTheme("dcc_retry"));
        break;
    default:
        m_startButton->setIcon(static_cast<QStyle::StandardPixmap>(-1));
        break;
    }
}

void UpdateControlPanel::setUpdateStatus(UpdatesStatus status)
{
    qCInfo(DCC_UPDATE) << "Incoming:" << status
            << ", current:" << m_updateStatus
            << ", control type:" << m_controlPanelType;

    m_updateStatus = status;
    static QMap<UpdatesStatus, QString> updateTips = {
        { UpdatesAvailable, tr("Updates available") },
        { DownloadWaiting, tr("Preparing for downloading...") },
        { Downloading, tr("Downloading updates...") },
        { DownloadPaused, tr("Downloading paused") },
        { Downloaded, tr("Downloading completed") },
        { DownloadFailed, tr("Downloading updates failed") },
        { UpgradeWaiting, tr("Preparing for updates…") },
        { UpgradeReady, tr("Preparing for updates…") },
        { BackingUp, tr("Backing up…") },
        { BackupSuccess, tr("Installing updates...") },
        { BackupFailed, tr("Backup failed") },
        { Upgrading, tr("Installing updates...") },
        { UpgradeFailed, tr("Updates failed") },
        { UpgradeSuccess, tr("Updates successful") }
    };

    if (UpdateModel::getSupportUpdateTypes(m_controlPanelType).contains(m_updateStatus)) {
        m_updateTipsLab->setText(updateTips.value(m_updateStatus));
        updateWidgets();

        if (m_updateStatus == DownloadPaused)
            setButtonStatus(ButtonStatus::start);
        else if (m_updateStatus == Downloading)
            setButtonStatus(ButtonStatus::pause);
    }
}

void UpdateControlPanel::setButtonStatus(const ButtonStatus& value)
{
    m_buttonStatus = value;
    setButtonIcon(value);
    if (value == ButtonStatus::invalid) {
        m_startButton->setEnabled(false);
    }
}

void UpdateControlPanel::setProgressText(const QString& text, const QString& toolTip)
{
    m_progressLabel->setText(getElidedText(m_progressLabel, text, Qt::ElideRight, m_progressLabel->maximumWidth() - 10, 0, __LINE__));
    m_progressLabel->setToolTip(toolTip);
}

// used to display long string: "12345678" -> "12345..."
QString UpdateControlPanel::getElidedText(QWidget* widget, QString data, Qt::TextElideMode mode, int width, int flags, int line)
{
    QString retTxt = data;
    if (retTxt == "")
        return retTxt;

    QFontMetrics fontMetrics(font());
    int fontWidth = fontMetrics.width(data);
    if (fontWidth > width)
        retTxt = widget->fontMetrics().elidedText(data, mode, width, flags);

    return retTxt;
}

void UpdateControlPanel::onCtrlButtonClicked()
{
    ButtonStatus status = ButtonStatus::invalid;
    switch (m_buttonStatus) {
    case ButtonStatus::start:
        status = ButtonStatus::pause;
        Q_EMIT StartDownload();
        break;
    case ButtonStatus::pause:
        status = ButtonStatus::start;
        Q_EMIT PauseDownload();
        break;
    case ButtonStatus::retry:
        status = ButtonStatus::invalid;
        setProgressText("");
        break;
    default:
        break;
    }

    setButtonStatus(status);
}

void UpdateControlPanel::refreshDownloadSize()
{
    setDownloadSize(m_model->downloadSize(updateTypes() & m_model->checkUpdateMode()));
}

void UpdateControlPanel::setDownloadSize(qlonglong downloadSize)
{
    m_downloadSize = downloadSize;
    QString updateSize = formatCap(m_downloadSize);
    updateSize = tr("Size") + ": " + updateSize;
    m_updateSizeLab->setText(updateSize);
    updateText();
}

void UpdateControlPanel::updateWidgets()
{
    qCInfo(DCC_UPDATE) << "Update status:" << m_updateStatus
            << ", control type" << m_controlPanelType
            << ", types:" << updateTypes();

    // 把控件是否显示的判断值保存在map中，自动排序后True排在最后一个，显示最后一个控件即可
    QMap<UpdatesStatus, ControlWidget> controlWidgets = {
        { DownloadFailed, RETRY_BUTTON },
        { UpdatesAvailable, DOWNLOAD_BUTTON },
        { Downloading, PROGRESS_WIDGET },
        { DownloadPaused, PROGRESS_WIDGET },
        { Downloaded, UPGRADE_BUTTON },
        { UpgradeWaiting, SPINNER_WIDGET},
        { UpgradeReady, SPINNER_WIDGET },
        { BackupFailed, CONTINUE_UPGRADE },
        { BackingUp, PROGRESS_WIDGET },
        { Upgrading, PROGRESS_WIDGET },
        { BackupSuccess, PROGRESS_WIDGET },
        { UpgradeFailed, RETRY_BUTTON },
        { UpgradeSuccess, REBOOT_BUTTON },
    };
    if (!controlWidgets.contains(m_updateStatus)) {
        return;
    }

    // 特殊场景：备份失败且是未知错误时，显示两个按钮：重试和继续更新
    m_reBackupButton->setVisible(BackupFailed == m_updateStatus && m_model->lastError(BackupFailed) == BackupFailedUnknownReason);
    m_controlWidget->setCurrentIndex(controlWidgets.value(m_updateStatus));
    if (m_controlWidget->currentIndex() != SPINNER_WIDGET) {
        m_spinner->stop();
    }
    updateCurrentWidgetEnabledState();
    updateBackupProgress();
    updateText();
    onBatteryStatusChanged(m_model->batterIsOK());
}

void UpdateControlPanel::onUpdateStatusChanged(ControlPanelType type, UpdatesStatus status)
{
    // 取消Ready的状态
    if (Downloading == status && CPT_Available == m_controlPanelType) {
        setUpdateStatus(UpdatesAvailable);
    } else if ((Upgrading == status || BackingUp == status) && CPT_Downloaded == m_controlPanelType) {
        setUpdateStatus(Downloaded);
    }

    if (type != m_controlPanelType) {
        updateCurrentWidgetEnabledState();
        return;
    }

    setUpdateStatus(status);
}

void UpdateControlPanel::setControlPanelType(ControlPanelType type)
{
    m_controlPanelType = type;

    // 更新时无法暂停，不显示控制按钮
    if (m_controlPanelType == CPT_Upgrade) {
        m_startButton->setVisible(false);
        m_stopButton->setVisible(false);
        m_progressLabelRightSpacer->changeSize(16, 0);
        layout()->invalidate();
    } else if (m_controlPanelType == CPT_NeedRestart) {
        m_successIcon->setVisible(true);
    }
}

void UpdateControlPanel::onCheckUpdateModeChanged(int checkUpdateMode)
{
    refreshDownloadSize();
    updateCurrentWidgetEnabledState();
}

void UpdateControlPanel::updateCurrentWidgetEnabledState()
{
    if (!m_model->batterIsOK() && (m_updateStatus == Downloaded || m_updateStatus == BackupFailed)) {
        onBatteryStatusChanged(m_model->batterIsOK());
        return;
    }

    const bool enabled = (m_model->checkUpdateMode() & updateTypes()) && allowToSetEnabled();
    m_controlWidget->currentWidget()->setEnabled(enabled);
    if (m_reBackupButton->isVisible()){
        m_reBackupButton->setEnabled(enabled);
    }
    Q_EMIT requestSetUpdateItemCheckBoxEnabled(allowToSetEnabled() && m_controlWidget->currentIndex() != SPINNER_WIDGET);
}

bool UpdateControlPanel::allowToSetEnabled() const
{
    auto isPanelTypeExist = [this](ControlPanelType type) -> bool {
        return m_model->controlTypes().contains(type);
    };
    auto isWaitingStatusExist = [this](UpdatesStatus status) -> bool {
        return m_model->allWaitingStatus().contains(status);
    };

    const auto currentWidgetIndex = m_controlWidget->currentIndex();
    const bool readyForDownload = currentWidgetIndex == DOWNLOAD_BUTTON || (currentWidgetIndex == RETRY_BUTTON && m_updateStatus == DownloadFailed );
    const bool readyForUpgrade = currentWidgetIndex == UPGRADE_BUTTON || currentWidgetIndex == CONTINUE_UPGRADE || (currentWidgetIndex == RETRY_BUTTON && m_updateStatus == UpgradeFailed);

    if (currentWidgetIndex == SPINNER_WIDGET
     || (readyForDownload && (isPanelTypeExist(CPT_Downloading) || isWaitingStatusExist(DownloadWaiting)))
     || (readyForUpgrade && (isPanelTypeExist(CPT_Upgrade) || isWaitingStatusExist(UpgradeWaiting)))) {
        return false;
    }

    return true;
}

void UpdateControlPanel::updateBackupProgress()
{
    if (m_updateStatus != BackingUp && m_updateStatus != BackupSuccess) {
        return;
    }
    setInstallProgressBeginValue(m_updateStatus == BackupSuccess);
}

void UpdateControlPanel::setInstallProgressBeginValue(bool success)
{
    if (CPT_Upgrade != m_controlPanelType) {
        return;
    }

    InstallProgressBeginValue = success ? BACKUP_SUCCESS_PROGRESS : BACKUP_START_PROGRESS;
    if(InstallProgressBeginValue > m_progress->value()) {
        m_progress->setValue(InstallProgressBeginValue);
        setProgressText(QString("%1%").arg(InstallProgressBeginValue));
    }
}

void UpdateControlPanel::onUpgradeProgressChanged(double value)
{
    if (m_controlPanelType != CPT_Upgrade) {
        return;
    }

    double tmpValue = value * (100 - InstallProgressBeginValue);
    // 在备份完成后,如果InstallProgressBeginValue=50，那么更新进度需要大于等于2进度条才会增加,等待时间过长,体验不好.
    if (InstallProgressBeginValue > 0 && tmpValue < 1 && tmpValue > 0)
        tmpValue = 1.0;

    int iProgress = InstallProgressBeginValue + static_cast<int>(tmpValue);
    // 进度条不能大于100，不能小于0，不能回退
    if (iProgress > 100 || iProgress < 0 || iProgress <= m_progress->value())
        return;

    m_progress->setValue(iProgress);
    setProgressText(QString("%1%").arg(iProgress));
}

int UpdateControlPanel::updateTypes() const
{
    if (!m_model) {
        return 0;
    }
    return m_model->updateTypes(m_controlPanelType);
}

void UpdateControlPanel::onUpdateButtonClicked()
{
    DDialog* chooseInstallTypeDialog = new DDialog(this);
    chooseInstallTypeDialog->setAttribute(Qt::WA_DeleteOnClose, true);
    connect(m_model, &UpdateModel::checkUpdateModeChanged, chooseInstallTypeDialog, [chooseInstallTypeDialog] {
        chooseInstallTypeDialog->close();
    });
    connect(m_model, &UpdateModel::updateStatusChanged, chooseInstallTypeDialog, [chooseInstallTypeDialog] {
        chooseInstallTypeDialog->close();
    });
    chooseInstallTypeDialog->setMinimumSize(422, 188);
    const auto iconSize = QSize(32 * qApp->devicePixelRatio(), 32 * qApp->devicePixelRatio());
    const auto iconPath = ":/update/themes/common/icons/dcc_update.svg";
    chooseInstallTypeDialog->setIcon(QIcon(DHiDPIHelper::loadNxPixmap(iconPath).scaled(iconSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)));
    chooseInstallTypeDialog->setAttribute(Qt::WA_DeleteOnClose);

    int backgroundInstall = chooseInstallTypeDialog->addButton(tr("Silent Installation"));
    int installAndReboot = chooseInstallTypeDialog->addButton(tr("Update and Reboot"));
    int installAndShutdown = chooseInstallTypeDialog->addButton(tr("Update and Shut Down"), true, DDialog::ButtonRecommend);
    for (auto button : chooseInstallTypeDialog->getButtons()) {
        const QString& originText = button->text();
        const QString& text = getElidedText(button, button->text(), Qt::ElideRight, FullUpdateBtnWidth - 10, 0, __LINE__);
        button->setText(text);
        button->setToolTip(originText);
    }
    chooseInstallTypeDialog->setMessage(tr("The updates have been already downloaded. What do you want to do?"));
    chooseInstallTypeDialog->setFixedWidth(422);
    connect(chooseInstallTypeDialog, &DDialog::buttonClicked, this, [this, backgroundInstall, installAndReboot, installAndShutdown] (int index, const QString &text) {
        // ret: 0: 后台安装，1：安装并重启，2：安装并关机
        if (index == backgroundInstall) {
            Q_EMIT SignalBridge::ref().requestBackgroundInstall(updateTypes() & m_model->checkUpdateMode(), true);
        } else if (index == installAndReboot || index == installAndShutdown) {
            QString method = index == installAndReboot ? "UpdateAndReboot" : "UpdateAndShutdown";
            DDBusSender()
                .service("com.deepin.dde.lockFront")
                .interface("com.deepin.dde.shutdownFront")
                .path("/com/deepin/dde/shutdownFront")
                .method(method)
                .call();
        } else {
            // User clicked `Close` button, do nothing
        }
    });

    chooseInstallTypeDialog->show();
}

void UpdateControlPanel::onBatteryStatusChanged(bool isOK)
{
    if (m_updateStatus != Downloaded && m_updateStatus != BackupFailed) {
        return;
    }

    bool isWidgetEnabled = isOK && (m_model->checkUpdateMode() & updateTypes()) && allowToSetEnabled();
    m_continueUpgrade->setEnabled(isWidgetEnabled);
    m_updateBtn->setEnabled(isWidgetEnabled);
    m_reBackupButton->setEnabled(isWidgetEnabled);

    updateText();

    Q_EMIT requestSetUpdateItemCheckBoxEnabled(isOK && allowToSetEnabled() && m_controlWidget->currentIndex() != SPINNER_WIDGET);
}

bool UpdateControlPanel::getCheckBoxEnabledState() const
{
    if (m_updateStatus == Downloaded ||m_updateStatus == BackupFailed) {
        return m_model->batterIsOK() && allowToSetEnabled();
    }

    return allowToSetEnabled();
}

void UpdateControlPanel::showText(int type, const QString& text)
{
    m_tipLabelWidget->setCurrentIndex(type);
    if (type == ShowLogDialog) {
        return;
    }

    auto label = qobject_cast<QLabel*>(m_tipLabelWidget->currentWidget());
    if (!label) {
        return;
    }

    label->setText(text);
}

QPair<int, QString> UpdateControlPanel::getText()
{
    static const QString CAN_INSTALL_TEXT = tr("You can install updates when shut down or reboot");
    static const QString INSTALLING_UPDATES_TEXT = tr("Do not force a shutdown or power off when installing updates. Otherwise, your system may be damaged.");
    static const QString NEED_REBOOT = tr("Reboot to use the system and the applications properly");
    static const QString NEED_CHARGING = tr("The battery capacity is lower than 60%. To get successful updates, please plug in.");
    static QMap<UpdatesStatus, QPair<int, QString>> textMapping = {
        {Downloaded, qMakePair(InfoTips, CAN_INSTALL_TEXT)},
        {Upgrading, qMakePair(InfoTips, INSTALLING_UPDATES_TEXT)},
        {BackingUp, qMakePair(InfoTips, INSTALLING_UPDATES_TEXT)},
        {BackupSuccess, qMakePair(InfoTips, INSTALLING_UPDATES_TEXT)},
        {UpgradeSuccess, qMakePair(InfoTips, NEED_REBOOT)},
    };

    if(m_updateStatus == Downloaded && !m_model->batterIsOK()) {
        return qMakePair(ErrorTips, NEED_CHARGING);
    }

    if (textMapping.contains(m_updateStatus)) {
        return textMapping.value(m_updateStatus);
    }

    if (m_updateStatus == DownloadFailed || m_updateStatus == BackupFailed || m_updateStatus == UpgradeFailed) {
        QString text = UpdateModel::errorToText(m_model->lastError(m_updateStatus));
        if (m_model->lastError(DownloadFailed) == DownloadingNoSpace) {
            // 下载所需空间减去 `/var` 目录所在分区可用空间的大小
            qlonglong bytes = m_model->downloadSize(m_model->updateMode()) - QStorageInfo("/var").bytesAvailable();
            bytes = bytes > 0 ? bytes : 10 * 1024 * 1024; // 这种异常情况给个默认值，避免出现释放值小于等于0的提示文案
            text = text.arg(formatCap(bytes));
        }
        if (m_model->lastError(m_updateStatus) == UnKnown) {
            return qMakePair(ShowLogDialog, QString());
        }
        return qMakePair(ErrorTips, text);
    }

    return qMakePair(DownloadSize, m_updateSizeLab->text());
}

void UpdateControlPanel::updateText()
{
    // 更新错误、更新包大小等提示信息
    const auto& pair = getText();
    showText(pair.first, pair.second);
}

void UpdateControlPanel::startSpinner()
{
    m_spinner->start();
    m_controlWidget->setCurrentIndex(SPINNER_WIDGET);
    updateCurrentWidgetEnabledState();
}
