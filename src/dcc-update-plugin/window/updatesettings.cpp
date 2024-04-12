// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatesettings.h"
#include "updatemodel.h"
#include "widgets/nextpagewidget.h"
#include "widgets/settingsgroup.h"
#include "widgets/switchwidget.h"
#include "widgets/translucentframe.h"
#include "window/gsettingwatcher.h"
#include "window/utils.h"
#include "linebutton.h"

#include <DDialog>
#include <DFontSizeManager>
#include <DTipLabel>
#include <DWaterProgress>

#include <QGSettings>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QVBoxLayout>

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace dcc;
using namespace dcc::widgets;
using namespace dcc::update;
using namespace DCC_NAMESPACE;
using namespace DCC_NAMESPACE::update;

static const QString DEFAULT_TIME_FORMAT = "hh:mm";
namespace dcc {
namespace update{
class NextPage : public NextPageWidget
{
public:
    explicit NextPage(QFrame *parent = nullptr, bool bLeftInterval = true)
        :NextPageWidget (parent,bLeftInterval)
        , m_icon(new TipsLabel)
    {
        layout()->addWidget(m_icon);
    }

    void setBtnHiden(const bool hiden)
    {
        m_nextPageBtn->setHidden(hiden);
    }

    void setIconIcon(const QPixmap &icon)
    {
        m_icon->setPixmap(icon);
    }
protected:
    TipsLabel *m_icon;
};
}
}


UpdateSettings::UpdateSettings(UpdateModel* model, QWidget* parent)
    : ContentWidget(parent)
    , m_model(model)
    , m_updateModeSettingItem(new UpdateModeSettingItem(model, this))
    , m_downloadSpeedLimitSwitch(nullptr)
    , m_downloadSpeedLimitSpinBox(new DSpinBox(this))
    , m_downloadSpeedLimitWidget(new QWidget(this))
    , m_autoDownloadUpdateTips(new DTipLabel(tr("Switch it on to automatically download the updates in wireless or wired network"), this))
    , m_testingChannelTips(new DTipLabel(tr("Join the internal testing channel to get deepin latest updates")))
    , m_testingChannelLinkLabel(new QLabel(""))
    , m_idleDownloadCheckBox(nullptr)
    , m_startTimeLabel(new QLabel(tr("Start at"), this))
    , m_endTimeLabel(new QLabel(tr("End at"), this))
    , m_startTimeEdit(new Dtk::Widget::DTimeEdit(this))
    , m_endTimeEdit(new Dtk::Widget::DTimeEdit(this))
    , m_idleTimeDownloadWidget(new QWidget(this))
    , m_autoCleanCache(new SwitchWidget(this))
    , m_p2pUpdateSwitch(nullptr)
    , m_p2pUpdateWidget(nullptr)
    , m_upgradeHistoryDialog(nullptr)
{
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_downloadSpeedLimitSwitch = new SwitchWidget(tr("Limit Speed"), this);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_updateNotify = new SwitchWidget(tr("Updates Notification"), this);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_autoDownloadUpdate = new SwitchWidget(tr("Auto Download Updates"), this);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_testingChannel = new SwitchWidget(tr("Join Internal Testing Channel"), this);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_testingChannelHeadingLabel = new QLabel(tr("Updates from Internal Testing Sources"));
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_idleDownloadCheckBox = new QCheckBox(tr("Download when Inactive"), this);
    initUi();
    initConnection();
    setModel(model);
}

UpdateSettings::~UpdateSettings()
{
    GSettingWatcher::instance()->erase("updateUpdateNotify", m_updateNotify);
    GSettingWatcher::instance()->erase("updateAutoDownlaod", m_autoDownloadUpdate);
    GSettingWatcher::instance()->erase("updateCleanCache", m_autoCleanCache);
}

void UpdateSettings::initUi()
{
    setTitle(tr("Update Settings"));

    TranslucentFrame* contentWidget = new TranslucentFrame(this); // 添加一层半透明框架
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(10);
    contentLayout->addSpacing(20);

    m_updateModeSettingItem->setAccessibleName("UpdateModeSettingItem");
    contentLayout->addWidget(m_updateModeSettingItem);

    auto otherSettingsLabel = new QLabel(tr("Other settings"), this);
    otherSettingsLabel->setAlignment(Qt::AlignLeft);
    DFontSizeManager::instance()->bind(otherSettingsLabel, DFontSizeManager::T5, QFont::DemiBold);
    contentLayout->addWidget(otherSettingsLabel);

    SettingsGroup* checkUpdatesGrp = new SettingsGroup(nullptr, SettingsGroup::GroupBackground);
    checkUpdatesGrp->appendItem(m_autoDownloadUpdate);

    // 限速
    SettingsGroup* downloadSpeedLimitGrp = new SettingsGroup(nullptr, SettingsGroup::GroupBackground);
    m_downloadSpeedLimitSpinBox->setFixedWidth(280);
    m_downloadSpeedLimitSpinBox->setSuffix(" KB/s");
    m_downloadSpeedLimitSpinBox->setRange(1, 99999);
    m_downloadSpeedLimitSpinBox->installEventFilter(this);
    m_downloadSpeedLimitWidget->setVisible(false);
    QHBoxLayout* speedLimitLayout = new QHBoxLayout(m_downloadSpeedLimitWidget);
    speedLimitLayout->addWidget(m_downloadSpeedLimitSpinBox, 0);
    speedLimitLayout->addStretch();
    downloadSpeedLimitGrp->appendItem(m_downloadSpeedLimitSwitch);
    downloadSpeedLimitGrp->insertWidget(m_downloadSpeedLimitWidget);
    contentLayout->addWidget(downloadSpeedLimitGrp);

    // 闲时下载
    QVBoxLayout* idleDownloadLayout = new QVBoxLayout(this);
    idleDownloadLayout->addWidget(m_idleDownloadCheckBox, 0);
    QHBoxLayout* idleTimeSpinLayout = new QHBoxLayout(this);
    m_startTimeEdit->setDisplayFormat(DEFAULT_TIME_FORMAT);
    m_startTimeEdit->setAlignment(Qt::AlignCenter);
    m_startTimeEdit->setAccessibleName("Start_Time_Edit");
    m_startTimeEdit->setProperty("_d_dtk_spinBox", true);
    m_startTimeEdit->setWrapping(true);
    m_endTimeEdit->setDisplayFormat(DEFAULT_TIME_FORMAT);
    m_endTimeEdit->setAlignment(Qt::AlignCenter);
    m_endTimeEdit->setAccessibleName("End_Time_Edit");
    m_endTimeEdit->setProperty("_d_dtk_spinBox", true);
    m_endTimeEdit->setWrapping(true);
    idleTimeSpinLayout->addSpacing(30);
    idleTimeSpinLayout->addWidget(m_startTimeLabel, 0);
    idleTimeSpinLayout->addWidget(m_startTimeEdit, 0);
    idleTimeSpinLayout->addSpacing(30);
    idleTimeSpinLayout->addWidget(m_endTimeLabel, 0);
    idleTimeSpinLayout->addWidget(m_endTimeEdit, 0);
    idleTimeSpinLayout->addStretch(1);
    idleDownloadLayout->addLayout(idleTimeSpinLayout, 0);
    m_idleTimeDownloadWidget->setLayout(idleDownloadLayout);
    checkUpdatesGrp->insertWidget(m_idleTimeDownloadWidget);
    contentLayout->addWidget(checkUpdatesGrp);

    // 自动下载提示
    m_autoDownloadUpdateTips->setWordWrap(true);
    m_autoDownloadUpdateTips->setAlignment(Qt::AlignLeft);
    m_autoDownloadUpdateTips->setContentsMargins(10, 0, 10, 0);
    contentLayout->addWidget(m_autoDownloadUpdateTips);

    // 更新提醒
    SettingsGroup* updatesNotificationGrp = new SettingsGroup;
    updatesNotificationGrp->appendItem(m_updateNotify);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    // 清除软件包缓存
    m_autoCleanCache->setTitle(tr("Clear Package Cache"));
    m_autoCleanCache->setObjectName("AutoCleanCache");
    updatesNotificationGrp->appendItem(m_autoCleanCache);
    contentLayout->addWidget(m_updateNotify);
    contentLayout->addWidget(m_autoCleanCache);

    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    // p2p更新
    m_p2pUpdateSwitch = new SwitchWidget(tr("Transferring Cache"), this);
    m_p2pUpdateSwitch->addBackground();
    auto p2pTips = new DTipLabel(tr("Enable this option to transfer updates to computers that are locally connected to the network to speed up the download of updates to other computers."), this);
    p2pTips->setWordWrap(true);
    p2pTips->setAlignment(Qt::AlignLeft);
    p2pTips->setContentsMargins(10, 0, 10, 0);

    m_p2pUpdateWidget = new QWidget(this);
    auto p2pLayout = new QVBoxLayout(m_p2pUpdateWidget);
    p2pLayout->setMargin(0);
    p2pLayout->addWidget(m_p2pUpdateSwitch);
    p2pLayout->addWidget(p2pTips);
    DConfigWatcher::instance()->bind(DConfigWatcher::update, "p2pUpdateEnabled", m_p2pUpdateWidget);
    contentLayout->addWidget(m_p2pUpdateWidget);

#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    if (!IsServerSystem && !IsProfessionalSystem && !IsHomeSystem && !IsEducationSystem && !IsDeepinDesktop) {
        m_sourceCheck = new SwitchWidget(tr("System Repository Detection"), this);
        m_sourceCheck->addBackground();
        contentLayout->addWidget(m_sourceCheck);
        DTipLabel* sourceCheckTips = new DTipLabel(tr("Show a notification if system update repository has been modified"), this);
        sourceCheckTips->setWordWrap(true);
        sourceCheckTips->setAlignment(Qt::AlignLeft);
        sourceCheckTips->setContentsMargins(10, 0, 10, 0);
        contentLayout->addWidget(sourceCheckTips);
    }
#endif

    if (IsCommunitySystem) {
        m_smartMirrorBtn = new SwitchWidget(tr("Smart Mirror Switch"), this);
        m_smartMirrorBtn->addBackground();
        contentLayout->addWidget(m_smartMirrorBtn);

        DTipLabel* smartTips = new DTipLabel(tr("Switch it on to connect to the quickest mirror site automatically"), this);
        smartTips->setWordWrap(true);
        smartTips->setAlignment(Qt::AlignLeft);
        smartTips->setContentsMargins(10, 0, 10, 0);
        contentLayout->addWidget(smartTips);

        m_updateMirrors = new NextPage(nullptr, false);
        m_updateMirrors->setTitle(tr("Mirror List"));
        m_updateMirrors->setRightTxtWordWrap(true);
        m_updateMirrors->addBackground();
        QStyleOption opt;
        m_updateMirrors->setBtnHiden(true);
        m_updateMirrors->setIconIcon(DStyleHelper(m_updateMirrors->style()).standardIcon(DStyle::SP_ArrowEnter, &opt, nullptr).pixmap(10, 10));
        contentLayout->addWidget(m_updateMirrors);
        contentLayout->addSpacing(10);

        DFontSizeManager::instance()->bind(m_testingChannelHeadingLabel, DFontSizeManager::T5, QFont::DemiBold);
        m_testingChannelHeadingLabel->setContentsMargins(10, 0, 10, 0); // 左右边距为10
        contentLayout->addWidget(m_testingChannelHeadingLabel);
        // Add link label to switch button
        m_testingChannelLinkLabel->setOpenExternalLinks(true);
        m_testingChannelLinkLabel->setStyleSheet("font: 12px");
        auto mainLayout = m_testingChannel->getMainLayout();
        mainLayout->insertWidget(mainLayout->count() - 1, m_testingChannelLinkLabel);

        m_testingChannel->addBackground();
        contentLayout->addWidget(m_testingChannel);
        m_testingChannelTips->setWordWrap(true);
        m_testingChannelTips->setAlignment(Qt::AlignLeft);
        m_testingChannelTips->setContentsMargins(10, 0, 10, 0);
        contentLayout->addWidget(m_testingChannelTips);
        m_testingChannel->setVisible(false);
        m_testingChannelTips->setVisible(false);
    } else {
        m_testingChannel->hide();
        m_testingChannelTips->hide();
        m_testingChannelLinkLabel->hide();
        m_testingChannelHeadingLabel->hide();
    }

    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    // 历史更新记录
    QStyleOption opt;
    auto historyButton = new LineButton(tr("Update History"), DStyleHelper(this->style()).standardIcon(DStyle::SP_ArrowEnter, &opt, nullptr).pixmap(12, 12), this);
    connect(historyButton, &LineButton::clicked, this, [this] {
        if (!m_upgradeHistoryDialog) {
            m_upgradeHistoryDialog = new UpgradeHistoryDialog(this);
            m_upgradeHistoryDialog->setAttribute(Qt::WA_DeleteOnClose);
        }
        m_upgradeHistoryDialog->show();
        m_upgradeHistoryDialog->activateWindow();
    });

    DConfigWatcher::instance()->bind(DConfigWatcher::update,"updateHistoryEnabled", historyButton);
    contentLayout->addWidget(historyButton);

    contentLayout->setAlignment(Qt::AlignTop);
    contentLayout->setContentsMargins(46, 10, 46, 5);

    setContentsMargins(0, 0, 0, 10);
    setContent(contentWidget);
}

void UpdateSettings::initConnection()
{
    connect(m_updateModeSettingItem, &UpdateModeSettingItem::updateModeEnableStateChanged, this, [this](UpdateType type, bool enable) {
        const quint64 updateMode = enable ? m_model->updateMode() | type : m_model->updateMode() & (~type);
        Q_EMIT requestUpdateMode(updateMode);
    });
    connect(m_updateNotify, &SwitchWidget::checkedChanged, this, &UpdateSettings::requestSetUpdateNotify);
    connect(m_autoDownloadUpdate, &SwitchWidget::checkedChanged, this, [this](bool checked) {
        m_idleTimeDownloadWidget->setVisible(checked);
        if (!checked)
            m_idleDownloadCheckBox->setChecked(false);

        Q_EMIT requestSetAutoDownloadUpdates(checked);
    });
    connect(m_autoCleanCache, &SwitchWidget::checkedChanged, this, &UpdateSettings::requestSetAutoCleanCache);

#if 0
#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    if (!IsServerSystem && !IsProfessionalSystem && !IsHomeSystem && !IsCommunitySystem && !IsDeepinDesktop) {
        qCDebug(DCC_UPDATE) << "Connect source check";
        connect(m_sourceCheck, &SwitchWidget::checkedChanged, this, &UpdateSettings::requestSetSourceCheck);
    }
#endif
#endif

    if (IsCommunitySystem) {
        connect(m_updateMirrors, &NextPageWidget::clicked, this, &UpdateSettings::requestShowMirrorsView);
        connect(m_smartMirrorBtn, &SwitchWidget::checkedChanged, this, &UpdateSettings::requestEnableSmartMirror);

        connect(m_testingChannel, &SwitchWidget::checkedChanged, this, &UpdateSettings::onTestingChannelCheckChanged);
    }
    connect(m_idleDownloadCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
        auto config = m_model->idleDownloadConfig();
        const bool isIdleDownloadEnabled = state == Qt::CheckState::Checked;
        config.idleDownloadEnabled = isIdleDownloadEnabled;
        setIdleDownloadTimeWidgetsEnabled(isIdleDownloadEnabled);
        Q_EMIT requestSetIdleDownloadConfig(config);
    });

    // 规则：开始时间和结束时间不能相等，否则默认按相隔五分钟处理
    // 修改开始时间时，如果不满足规则，那么自动调整结束时间，结束时间=开始时间+5分钟
    // 修改结束时间时，如果不满足规则，那么自动调整开始时间，开始时间=结束时间-5分钟
    auto adjustTimeFunc = [](const QString& start, const QString& end, bool returnEndTime) -> QString {
        if (start != end)
            return returnEndTime ? end : start;

        static const int MIN_INTERVAL_SECS = 5 * 60;
        QDateTime dateTime(QDate::currentDate(), QTime::fromString(start));
        return returnEndTime ? dateTime.addSecs(MIN_INTERVAL_SECS).time().toString(DEFAULT_TIME_FORMAT)
                             : dateTime.addSecs(-MIN_INTERVAL_SECS).time().toString(DEFAULT_TIME_FORMAT);
    };

    connect(m_startTimeEdit, &DDateTimeEdit::timeChanged, this, [this, adjustTimeFunc](QTime time) {
        auto config = m_model->idleDownloadConfig();
        config.beginTime = time.toString(DEFAULT_TIME_FORMAT);
        config.endTime = adjustTimeFunc(config.beginTime, config.endTime, true);
        Q_EMIT requestSetIdleDownloadConfig(config);
    });
    connect(m_endTimeEdit, &DDateTimeEdit::timeChanged, this, [this, adjustTimeFunc](QTime time) {
        auto config = m_model->idleDownloadConfig();
        config.endTime = time.toString(DEFAULT_TIME_FORMAT);
        config.beginTime = adjustTimeFunc(config.beginTime, time.toString(DEFAULT_TIME_FORMAT), false);
        Q_EMIT requestSetIdleDownloadConfig(config);
    });

    connect(m_downloadSpeedLimitSwitch, &SwitchWidget::checkedChanged, this, [this](bool checked) {
        m_downloadSpeedLimitWidget->setVisible(checked);
        auto config = m_model->speedLimitConfig();
        config.downloadSpeedLimitEnabled = checked;
        Q_EMIT requestSetDownloadSpeedLimitConfig(config.toJson());
    });

    auto hideAlertMessage = [this] {
        m_downloadSpeedLimitSpinBox->setAlert(false);
        // DSpinBox没有提供hideAlertMessage方法，显示一个一毫秒的消息用来快速隐藏alert message
        m_downloadSpeedLimitSpinBox->showAlertMessage("", 1);
    };

    connect(m_downloadSpeedLimitSpinBox, qOverload<int>(&DSpinBox::valueChanged), this, [this, hideAlertMessage](int value) {
        hideAlertMessage();
        auto config = m_model->speedLimitConfig();
        config.limitSpeed = value;
        Q_EMIT requestSetDownloadSpeedLimitConfig(config.toJson());
    });
    connect(m_downloadSpeedLimitSpinBox->lineEdit(), &QLineEdit::textChanged, this, [this, hideAlertMessage](const QString& value) {
        static QString lastValidText;
        // 规则：1.只允许输入数字； 2.1～5位数字；3.第一个数字不能为0
        QRegExp exp("^([1-9][0-9]{0,4})$");
        QString tmpValue = value;
        tmpValue = tmpValue.replace(m_downloadSpeedLimitSpinBox->suffix(), "");
        if (tmpValue.isEmpty() || exp.exactMatch(tmpValue)) {
            lastValidText = tmpValue;
            hideAlertMessage();
            return;
        }

        m_downloadSpeedLimitSpinBox->lineEdit()->setText(lastValidText + m_downloadSpeedLimitSpinBox->suffix());
        if (lastValidText.size() == 5) // 当已经有五位数时，不提示错误，否则在前面输入0和+会提示，输入其他则不提示，表现不一致
            return;

        const static int TIME_OUT = 3000; // 显示时长
        m_downloadSpeedLimitSpinBox->setAlert(true);
        m_downloadSpeedLimitSpinBox->showAlertMessage(tr("Only numbers between 1-99999 are allowed"), TIME_OUT);
        QTimer::singleShot(TIME_OUT, this, [this] {
            m_downloadSpeedLimitSpinBox->setAlert(false);
        });
    });
    connect(m_downloadSpeedLimitSpinBox->lineEdit(), &QLineEdit::editingFinished, this, [hideAlertMessage] {
        hideAlertMessage();
    });
    connect(m_p2pUpdateSwitch, &SwitchWidget::checkedChanged, this, &UpdateSettings::requestSetP2PEnabled);
}

void UpdateSettings::updateWidgetsEnabledStatus()
{
    auto setCheckEnable = [](QWidget* widget, bool state, const QString& key, bool useDconfig) {
        QString status = DConfigWatcher::instance()->getStatus(DConfigWatcher::ModuleType::update, key);
        if (!useDconfig) {
            status = GSettingWatcher::instance()->get(key).toString();
        }

        widget->setEnabled("Enabled" == status && state);
    };

    const bool enable = m_model->updateMode() > UpdateType::Invalid
        && !LastoreDaemonDConfigStatusHelper::isUpdateDisabled(m_model->lastoreDaemonStatus());
    setCheckEnable(m_autoDownloadUpdate, enable, "updateAutoDownlaod", false);
    setCheckEnable(m_autoDownloadUpdateTips, enable, "updateAutoDownlaod", false);
    setCheckEnable(m_updateNotify, enable, "updateUpdateNotify", false);
    m_idleTimeDownloadWidget->setEnabled(m_autoDownloadUpdate->isEnabled());
    m_downloadSpeedLimitWidget->setEnabled(!LastoreDaemonDConfigStatusHelper::isUpdateDisabled(m_model->lastoreDaemonStatus()));
    m_downloadSpeedLimitSwitch->setEnabled(!LastoreDaemonDConfigStatusHelper::isUpdateDisabled(m_model->lastoreDaemonStatus()));
}

void UpdateSettings::setModel(UpdateModel* model)
{
    if (!model) {
        qCWarning(DCC_UPDATE) << "Update model is not nullptr";
        return;
    }
    m_model = model;

    auto updateIdleDownloadConfig = [this] {
        IdleDownloadConfig config = m_model->idleDownloadConfig();
        m_idleDownloadCheckBox->setChecked(config.idleDownloadEnabled);
        setIdleDownloadTimeWidgetsEnabled(config.idleDownloadEnabled);
        m_startTimeEdit->setTime(QTime::fromString(config.beginTime, DEFAULT_TIME_FORMAT));
        m_endTimeEdit->setTime(QTime::fromString(config.endTime, DEFAULT_TIME_FORMAT));
    };
    auto updateDownloadSpeedLimitConfig = [this] {
        DownloadSpeedLimitConfig config = m_model->speedLimitConfig();
        m_downloadSpeedLimitSwitch->setChecked(config.downloadSpeedLimitEnabled);
        m_downloadSpeedLimitWidget->setVisible(config.downloadSpeedLimitEnabled);
        m_downloadSpeedLimitSpinBox->setValue(config.limitSpeed);
    };
    connect(model, &UpdateModel::autoDownloadUpdatesChanged, m_autoDownloadUpdate, [this] (bool checked) {
        m_autoDownloadUpdate->setChecked(checked);
        m_idleTimeDownloadWidget->setVisible(checked);
        if (!checked)
            m_idleDownloadCheckBox->setChecked(false);
    });
    connect(model, &UpdateModel::downloadSpeedLimitConfigChanged, this, updateDownloadSpeedLimitConfig);
    connect(model, &UpdateModel::idleDownloadConfigChanged, this, updateIdleDownloadConfig);
    connect(model, &UpdateModel::updateNotifyChanged, m_updateNotify, &SwitchWidget::setChecked);
    connect(model, &UpdateModel::autoCleanCacheChanged, m_autoCleanCache, &SwitchWidget::setChecked);
    connect(model, &UpdateModel::updateModeChanged, this, &UpdateSettings::updateWidgetsEnabledStatus);
    connect(model, &UpdateModel::lastoreDaemonStatusChanged, this, &UpdateSettings::updateWidgetsEnabledStatus);
    connect(model, &UpdateModel::p2pUpdateEnableStateChanged, m_p2pUpdateSwitch, &SwitchWidget::setChecked);

    // 绑定gSettings配置
    GSettingWatcher::instance()->bind("updateUpdateNotify", m_updateNotify);
    GSettingWatcher::instance()->bind("updateAutoDownlaod", m_autoDownloadUpdate);
    GSettingWatcher::instance()->bind("updateCleanCache", m_autoCleanCache);

    // 设置控件状态
    m_updateNotify->setChecked(model->updateNotify());
    m_autoDownloadUpdate->setChecked(model->autoDownloadUpdates());
    m_idleTimeDownloadWidget->setVisible(model->autoDownloadUpdates());
    m_autoCleanCache->setChecked(m_model->autoCleanCache());
    updateIdleDownloadConfig();
    updateDownloadSpeedLimitConfig();
    m_p2pUpdateSwitch->setChecked(model->isP2PUpdateEnabled());

    connect(GSettingWatcher::instance(), &GSettingWatcher::notifyGSettingsChanged, this, [=](const QString& gsetting, const QString& state) {
        bool status = GSettingWatcher::instance()->get(gsetting).toString() == "Enabled"
            && m_model->updateMode() > UpdateType::Invalid;

        if (gsetting == "updateAutoDownlaod") {
            m_autoDownloadUpdate->setEnabled(status);
            m_idleTimeDownloadWidget->setEnabled(status);
            m_autoDownloadUpdateTips->setEnabled(status);
        }

        if (gsetting == "updateUpdateNotify") {
            m_updateNotify->setEnabled(status);
        }
    });
    updateWidgetsEnabledStatus();

#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    if (!IsServerSystem && !IsProfessionalSystem && !IsHomeSystem && !IsEducationSystem && !IsDeepinDesktop) {
        connect(model, &UpdateModel::sourceCheckChanged, m_sourceCheck, &SwitchWidget::setChecked);
        m_sourceCheck->setChecked(model->sourceCheck());
    }
#endif

    // 镜像源相关配置仅在社区版生效
    if (IsCommunitySystem) {
        auto setDefaultMirror = [this](const MirrorInfo& mirror) {
            m_updateMirrors->setValue(mirror.m_name);
        };

        if (!model->mirrorInfos().isEmpty()) {
            setDefaultMirror(model->defaultMirror());
        }

        connect(model, &UpdateModel::defaultMirrorChanged, this, setDefaultMirror);
        connect(model, &UpdateModel::smartMirrorSwitchChanged, m_smartMirrorBtn, &SwitchWidget::setChecked);
        m_smartMirrorBtn->setChecked(m_model->smartMirrorSwitch());

        auto setMirrorListVisible = [=](bool visible) {
            m_updateMirrors->setVisible(!visible);
        };

        connect(model, &UpdateModel::smartMirrorSwitchChanged, this, setMirrorListVisible);
        setMirrorListVisible(model->smartMirrorSwitch());

        auto hyperLink = QString("<a href='%1'>%2</a>").arg(m_model->getTestingChannelJoinURL().toString(), tr("here"));
        m_testingChannelLinkLabel->setText(tr("Click %1 to complete the application").arg(hyperLink));
        connect(model, &UpdateModel::testingChannelStatusChanged, this, &UpdateSettings::onTestingChannelStatusChanged);
        onTestingChannelStatusChanged();
    }
}

void UpdateSettings::onTestingChannelStatusChanged()
{
    const auto channelStatus = m_model->getTestingChannelStatus();
    if (channelStatus == UpdateModel::TestingChannelStatus::Hidden) {
        m_testingChannelHeadingLabel->hide();
        m_testingChannel->hide();
        m_testingChannelTips->hide();
        m_testingChannelLinkLabel->hide();
    } else {
        m_testingChannelHeadingLabel->show();
        m_testingChannel->show();
        m_testingChannelTips->show();
        m_testingChannelLinkLabel->setVisible(channelStatus == UpdateModel::TestingChannelStatus::WaitJoined);
        m_testingChannel->setChecked(channelStatus != UpdateModel::TestingChannelStatus::NotJoined);
    }
}

void UpdateSettings::onTestingChannelCheckChanged(const bool checked)
{
    const auto status = m_model->getTestingChannelStatus();
    if (checked) {
        Q_EMIT requestSetTestingChannelEnable(checked);
        return;
    }
    if (status != UpdateModel::TestingChannelStatus::Joined) {
        Q_EMIT requestSetTestingChannelEnable(checked);
        return;
    }

    auto dialog = new DDialog(this);
    dialog->setFixedWidth(400);
    dialog->setFixedHeight(280);

    auto label = new DLabel(dialog);
    label->setWordWrap(true);
    label->setText(tr("Checking system versions, please wait..."));

    auto progress = new DWaterProgress(dialog);
    progress->setFixedSize(100, 100);
    progress->setTextVisible(false);
    progress->setValue(50);
    progress->start();

    QWidget* content = new QWidget(dialog);
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(0, 0, 0, 0);
    content->setLayout(layout);
    dialog->addContent(content);

    layout->addStretch();
    layout->addWidget(label, 0, Qt::AlignHCenter);
    layout->addSpacing(20);
    layout->addWidget(progress, 0, Qt::AlignHCenter);
    layout->addStretch();

    connect(m_model, &UpdateModel::canExitTestingChannelChanged, dialog, [=](const bool can) {
        progress->setVisible(false);
        if (!can) {
            Q_EMIT requestSetTestingChannelEnable(checked);
            dialog->deleteLater();
            return;
        }
        const auto text = tr("If you leave the internal testing channel now, you may not be able to get the latest bug fixes and updates. Please leave after the official version is released to keep your system stable!");
        label->setText(text);
        dialog->addButton(tr("Leave"), false, DDialog::ButtonWarning);
        dialog->addButton(tr("Cancel"), true, DDialog::ButtonRecommend);
    });
    // 检查有可能会很快完成，对话框一闪而过会给人一种操作出错的感觉
    // 延迟一秒后再执行检查，可以让人有时间看到对话框，提升用户体验
    QTimer::singleShot(1000, this, [this] {
        Q_EMIT requestCheckCanExitTestingChannel();
    });

    connect(dialog, &DDialog::closed, this, [=]() {
        // clicked windows close button
        m_testingChannel->setChecked(true);
        dialog->deleteLater();
    });
    connect(dialog, &DDialog::buttonClicked, this, [=](int index, const QString& text) {
        if (index == 0) {
            // clicked the leave button
            Q_EMIT requestSetTestingChannelEnable(checked);
        } else {
            // clicked the cancel button
            m_testingChannel->setChecked(true);
        }
        dialog->deleteLater();
    });
    dialog->exec();
}

void UpdateSettings::setIdleDownloadTimeWidgetsEnabled(bool enabled)
{
    m_startTimeLabel->setEnabled(enabled);
    m_startTimeEdit->setEnabled(enabled);
    m_endTimeLabel->setEnabled(enabled);
    m_endTimeEdit->setEnabled(enabled);
}

void UpdateSettings::setWidgetsEnabled(bool enable)
{
    auto frame = findChild<TranslucentFrame*>();
    if (!frame)
        return;

    for (int i = 0; i < frame->layout()->count(); ++i) {
        QWidget* w = frame->layout()->itemAt(i)->widget();
        if (!w || w->objectName() == "AutoCleanCache")
            continue;

        w->setEnabled(enable);
    }
}

bool UpdateSettings::eventFilter(QObject* o, QEvent* e)
{
    if (o == m_downloadSpeedLimitSpinBox && e->type() == QEvent::MouseButtonPress) {
        m_downloadSpeedLimitSpinBox->setAlert(false);
        m_downloadSpeedLimitSpinBox->showAlertMessage("", 1);
    }
    return dcc::ContentWidget::eventFilter(o, e);
}
