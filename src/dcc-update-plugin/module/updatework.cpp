// SPDX-FileCopyrightText: 2011 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatework.h"
#include "signalbridge.h"
#include "widgets/utils.h"
#include "window/dconfigwatcher.h"
#include "window/utils.h"
#include "updateloghelper.h"

#include <QApplication>
#include <QDBusError>
#include <QDesktopServices>
#include <QFuture>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVariant>
#include <QtConcurrent>

#include <DDBusSender>

#define MIN_NM_ACTIVE 50
#define UPDATE_PACKAGE_SIZE 0
using namespace DCC_NAMESPACE;

const QString TestingChannelPackage = "deepin-unstable-source";
const QString ChangeLogFile = "/usr/share/deepin/release-note/UpdateInfo.json";
const QString ChangeLogDic = "/usr/share/deepin/";
const QString UpdateLogTmpFile = "/tmp/deepin-update-log.json";

const int LOWEST_BATTERY_PERCENT = 60;

namespace dcc {
namespace update {

static int TestMirrorSpeedInternal(const QString& url, QPointer<QObject> baseObject)
{
    if (!baseObject || QCoreApplication::closingDown()) {
        return -1;
    }

    QStringList args;
    args << url << "-s"
         << "1";

    QProcess process;
    process.start("netselect", args);

    if (!process.waitForStarted()) {
        return 10000;
    }

    do {
        if (!baseObject || QCoreApplication::closingDown()) {
            process.kill();
            process.terminate();
            process.waitForFinished(1000);

            return -1;
        }

        if (process.waitForFinished(500))
            break;
    } while (process.state() == QProcess::Running);

    const QString output = process.readAllStandardOutput().trimmed();
    const QStringList result = output.split(' ');

    if (!result.first().isEmpty()) {
        return result.first().toInt();
    }

    return 10000;
}

UpdateWorker::UpdateWorker(UpdateModel* model, QObject* parent)
    : QObject(parent)
    , m_model(model)
    , m_checkUpdateJob(nullptr)
    , m_downloadJob(nullptr)
    , m_fixErrorJob(nullptr)
    , m_distUpgradeJob(nullptr)
    , m_lastoreSessionHelper(nullptr)
    , m_updateInter(nullptr)
    , m_managerInter(nullptr)
    , m_smartMirrorInter(nullptr)
    , m_iconTheme(nullptr)
    , m_iconThemeState("")
{
}

UpdateWorker::~UpdateWorker()
{
    deleteJob(m_downloadJob);
    deleteJob(m_checkUpdateJob);
    deleteJob(m_fixErrorJob);
}

void UpdateWorker::init()
{
    qRegisterMetaType<UpdatesStatus>("UpdatesStatus");
    qRegisterMetaType<UiActiveState>("UiActiveState");
    qRegisterMetaType<ControlPanelType>("ControlPanelType");

    m_lastoreSessionHelper = new LastoressionHelper(
        "com.deepin.LastoreSessionHelper",
        "/com/deepin/LastoreSessionHelper",
        QDBusConnection::sessionBus(),
        this);
    m_updateInter = new UpdateInter(
        "com.deepin.lastore",
        "/com/deepin/lastore",
        QDBusConnection::systemBus(),
        this);
    m_managerInter = new ManagerInter(
        "com.deepin.lastore",
        "/com/deepin/lastore",
        QDBusConnection::systemBus(),
        this);
    m_smartMirrorInter = new SmartMirrorInter(
        "com.deepin.lastore.Smartmirror",
        "/com/deepin/lastore/Smartmirror",
        QDBusConnection::systemBus(),
        this);
    m_iconTheme = new Appearance(
        "com.deepin.daemon.Appearance",
        "/com/deepin/daemon/Appearance",
        QDBusConnection::sessionBus(),
        this);
    m_powerInter = new PowerInter("com.deepin.system.Power", "/com/deepin/system/Power", QDBusConnection::systemBus(), this);

    m_managerInter->setSync(false);
    m_updateInter->setSync(false);
    m_lastoreSessionHelper->setSync(false);
    m_smartMirrorInter->setSync(false, false);
    m_iconTheme->setSync(false);

    connect(m_managerInter, &ManagerInter::JobListChanged, this, &UpdateWorker::onJobListChanged);
    connect(m_managerInter, &ManagerInter::AutoCleanChanged, m_model, &UpdateModel::setAutoCleanCache);
    connect(m_managerInter, &ManagerInter::UpdateModeChanged, this, &UpdateWorker::onUpdateModeChanged);
    connect(m_updateInter, &__Updater::AutoDownloadUpdatesChanged, m_model, &UpdateModel::setAutoDownloadUpdates);
    connect(m_updateInter, &__Updater::MirrorSourceChanged, m_model, &UpdateModel::setDefaultMirror);
    QDBusConnection::systemBus().connect("com.deepin.lastore",
        "/com/deepin/lastore",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        m_model, SLOT(onUpdatePropertiesChanged(QString, QVariantMap, QStringList)));
    connect(m_updateInter, &UpdateInter::UpdateNotifyChanged, m_model, &UpdateModel::setUpdateNotify);
    connect(m_updateInter, &UpdateInter::ClassifiedUpdatablePackagesChanged, this, &UpdateWorker::onClassifiedUpdatablePackagesChanged);

    if (IsCommunitySystem) {
        connect(m_smartMirrorInter, &SmartMirrorInter::EnableChanged, m_model, &UpdateModel::setSmartMirrorSwitch);
        connect(m_smartMirrorInter, &SmartMirrorInter::serviceValidChanged, this, &UpdateWorker::onSmartMirrorServiceIsValid);
        connect(
            m_smartMirrorInter, &SmartMirrorInter::serviceStartFinished, this, [=] {
                QTimer::singleShot(100, this, [=] {
                    m_model->setSmartMirrorSwitch(m_smartMirrorInter->enable());
                });
            },
            Qt::UniqueConnection);
    }

    //图片主题
    connect(m_iconTheme, &Appearance::IconThemeChanged, this, &UpdateWorker::onIconThemeChanged);
#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    connect(m_lastoreSessionHelper, &LastoressionHelper::SourceCheckEnabledChanged, m_model, &UpdateModel::setSourceCheck);
#endif
    connect(m_powerInter, &PowerInter::BatteryPercentageChanged, this, &UpdateWorker::checkPower);
    connect(m_powerInter, &PowerInter::OnBatteryChanged, this, &UpdateWorker::checkPower);

    QString sVersion = QString("%1 %2").arg(DSysInfo::uosProductTypeName()).arg(DSysInfo::majorVersion());
    if (!IsServerSystem)
        sVersion.append(" " + DSysInfo::uosEditionName());
    m_model->setSystemVersionInfo(sVersion);

    const auto server = valueByQSettings<QString>(DCC_CONFIG_FILES, "Testing", "Server", "");
    if (!server.isEmpty()) {
        m_model->setTestingChannelServer(server);
        if (m_managerInter->PackageExists(TestingChannelPackage)) {
            m_model->setTestingChannelStatus(UpdateModel::TestingChannelStatus::Joined);
        } else {
            m_model->setTestingChannelStatus(UpdateModel::TestingChannelStatus::NotJoined);
        }
    }

    connect(&SignalBridge::ref(), &SignalBridge::requestCheckUpdateModeChanged, this, &UpdateWorker::onRequestCheckUpdateModeChanged);
    connect(&SignalBridge::ref(), &SignalBridge::requestDownload, this, &UpdateWorker::startDownload);
    connect(&SignalBridge::ref(), &SignalBridge::requestRetry, this, &UpdateWorker::onRequestRetry);
    connect(&SignalBridge::ref(), &SignalBridge::requestBackgroundInstall, this, &UpdateWorker::doUpgrade);
    connect(&SignalBridge::ref(), &SignalBridge::requestStopDownload, this, &UpdateWorker::stopDownload);
}

void UpdateWorker::licenseStateChangeSlot()
{
    QFutureWatcher<void>* watcher = new QFutureWatcher<void>();
    connect(watcher, &QFutureWatcher<void>::finished, watcher, &QFutureWatcher<void>::deleteLater);
    QFuture<void> future = QtConcurrent::run(this, &UpdateWorker::getLicenseState);
    watcher->setFuture(future);
}

void UpdateWorker::getLicenseState()
{
    if (DSysInfo::DeepinDesktop == DSysInfo::deepinType()) {
        m_model->setSystemActivation(UiActiveState::Authorized);
        return;
    }
    QDBusInterface licenseInfo("com.deepin.license",
        "/com/deepin/license/Info",
        "com.deepin.license.Info",
        QDBusConnection::systemBus());
    if (!licenseInfo.isValid()) {
        qCWarning(DCC_UPDATE) << "License info dbus is invalid.";
        return;
    }
    UiActiveState reply = static_cast<UiActiveState>(licenseInfo.property("AuthorizationState").toInt());
    m_model->setSystemActivation(reply);
}

void UpdateWorker::activate()
{
    qCInfo(DCC_UPDATE) << "Active update worker";
#ifndef DISABLE_SYS_UPDATE_MIRRORS
    refreshMirrors();
#endif
    m_managerInter->setSync(true);
    m_updateInter->setSync(true);
    QString checkTime;
    m_updateInter->GetCheckIntervalAndTime(checkTime);
    m_model->setLastCheckUpdateTime(checkTime);
    m_model->setAutoCleanCache(m_managerInter->autoClean());
    m_model->setAutoDownloadUpdates(m_updateInter->autoDownloadUpdates());
    m_model->setUpdateMode(m_managerInter->updateMode());
    // FIXME 使用m_managerInter->property 无法获取到属性
    QDBusInterface managerInter("com.deepin.lastore",
        "/com/deepin/lastore",
        "com.deepin.lastore.Manager",
        QDBusConnection::systemBus());
    m_model->setCheckUpdateMode(managerInter.property("CheckUpdateMode").toInt());
    m_model->setUpdateNotify(m_updateInter->updateNotify());
    m_model->setUpdateStatus(managerInter.property("UpdateStatus").toByteArray());
    // FIXME 使用m_updateInter->property 无法获取到属性
    QDBusInterface updaterInter("com.deepin.lastore",
        "/com/deepin/lastore",
        "com.deepin.lastore.Updater",
        QDBusConnection::systemBus());
    QString config = updaterInter.property("IdleDownloadConfig").toString();
    m_model->setIdleDownloadConfig(IdleDownloadConfig::toConfig(config.toLatin1()));
    m_model->setSpeedLimitConfig(updaterInter.property("DownloadSpeedLimitConfig").toByteArray());
    m_model->setP2PUpdateEnabled(updaterInter.property("P2PUpdateEnable").toBool());

    if (IsCommunitySystem) {
        m_model->setSmartMirrorSwitch(m_smartMirrorInter->enable());
        onSmartMirrorServiceIsValid(m_smartMirrorInter->isValid());
    }
#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    m_model->setSourceCheck(m_lastoreSessionHelper->sourceCheckEnabled());
#endif

#ifndef DISABLE_SYS_UPDATE_MIRRORS
    refreshMirrors();
#endif

    checkPower();

    licenseStateChangeSlot();
    QDBusConnection::systemBus().connect("com.deepin.license", "/com/deepin/license/Info",
        "com.deepin.license.Info", "LicenseStateChange",
        this, SLOT(licenseStateChangeSlot()));

    QFutureWatcher<QString>* iconWatcher = new QFutureWatcher<QString>();
    connect(iconWatcher, &QFutureWatcher<QString>::finished, this, [=] {
        m_iconThemeState = iconWatcher->result();
        iconWatcher->deleteLater();
    });
    iconWatcher->setFuture(QtConcurrent::run([=] {
        bool isSync = m_iconTheme->sync();
        m_iconTheme->setSync(true);
        const QString& iconTheme = m_iconTheme->iconTheme();
        m_iconTheme->setSync(isSync);
        return iconTheme;
    }));

    if (LastoreDaemonDConfigStatusHelper::isUpdateDisabled(m_model->lastoreDaemonStatus())) {
        m_model->setLastStatus(UpdatesStatus::UpdateIsDisabled, __LINE__);
    } else {
        // 获取当前的job
        const QList<QDBusObjectPath> jobs = m_managerInter->jobList();
        if (jobs.count() > 0) {
            onJobListChanged(jobs);
            // 如果处于下载中或者安装中的时候直接显示更新内容，不检查更新
            const bool isDownloading = m_downloadJob && m_downloadJob->status() != "failed";
            const bool isUpgrading = m_distUpgradeJob && m_distUpgradeJob->status() != "failed";
            if (isUpgrading || isDownloading) {
                auto watcher = new QDBusPendingCallWatcher(m_managerInter->asyncCall("GetUpdateLogs", SystemUpdate | SecurityUpdate), this);
                connect(watcher, &QDBusPendingCallWatcher::finished, [this, watcher] {
                    watcher->deleteLater();
                    if (!watcher->isError()) {
                        QDBusPendingReply<QString> reply = watcher->reply();
                        UpdateLogHelper::ref().handleUpdateLog(reply.value());
                    } else {
                        qWarning() << "Get update log failed";
                    }
                    // 日志处理完了再显示更新内容界面
                    m_model->setLastStatus(CheckingSucceed, __LINE__);
                    setUpdateInfo();
                });
            }
        }
    }

    m_managerInter->setSync(false);
    m_updateInter->setSync(false);
}

void UpdateWorker::deactivate()
{
}

void UpdateWorker::checkForUpdates()
{
    qCInfo(DCC_UPDATE) << "Check for updates";
    if (LastoreDaemonDConfigStatusHelper::isUpdateDisabled(m_model->lastoreDaemonStatus())) {
        qCWarning(DCC_UPDATE) << "Update is disabled";
        return;
    }

    const auto activeState = m_model->systemActivation();
    if (!m_model->isActivationValid()) {
        qCWarning(DCC_UPDATE) << "System activation is invalid: " << activeState;
        return;
    }

    if (checkDbusIsValid()) {
        qCWarning(DCC_UPDATE) << "Check Dbus's validation failed do nothing";
        return;
    }

    if (m_checkUpdateJob) {
        qCWarning(DCC_UPDATE) << "Is checking update, won't do it again";
        return;
    }

    const auto& allUpdateStatuses = m_model->allUpdateStatus();
    if (allUpdateStatuses.contains(BackingUp)
        || allUpdateStatuses.contains(Upgrading)
        || allUpdateStatuses.contains(Downloading)
        || allUpdateStatuses.contains(DownloadPaused)) {
        qCInfo(DCC_UPDATE) << "Lastore daemon is busy now, current statuses:" << allUpdateStatuses;
        return;
    }

    m_model->resetDownloadInfo(); // 在检查更新前重置数据，避免有上次检查的数据残留

    QDBusPendingCall call = m_managerInter->UpdateSource();
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, call, watcher] {
        watcher->deleteLater();
        if (call.isError()) {
            qCWarning(DCC_UPDATE) << "Check update failed, error: " << call.error().message();
            m_model->setLastStatus(UpdatesStatus::CheckingFailed, __LINE__);
            cleanLaStoreJob(m_checkUpdateJob);
        }
    });
}

void UpdateWorker::setUpdateInfo()
{
    m_updateInter->setSync(true);
    const QMap<QString, QStringList>& updatePackages = m_updateInter->classifiedUpdatablePackages();
    m_updateInter->setSync(false);

    QMap<UpdateType, UpdateItemInfo*> updateInfoMap = getAllUpdateInfo(updatePackages);
    bool isUpdated = true;
    for (auto info : updateInfoMap.values()) {
        m_model->addUpdateInfo(info);
        if (info->isUpdateAvailable()) {
            isUpdated = false;
        }
    }
    m_model->refreshUpdateStatus();
    m_model->setLastStatus(isUpdated ? Updated : UpdatesAvailable, __LINE__);
}

/**
 * @brief 获取更新信息，包大小、日志等内容
 *
 * @param updatePackages 例如：{{“system”, {"dde-session-ui, code"}}, {"security", {"dde-session-shell"}}}
 * @return QMap<ClassifyUpdateType, UpdateItemInfo *>
 */
QMap<UpdateType, UpdateItemInfo*> UpdateWorker::getAllUpdateInfo(const QMap<QString, QStringList>& updatePackages)
{
    const QStringList& systemPackages = updatePackages.value(SYSTEM_UPGRADE_TYPE_STRING);
    const QStringList& securityPackages = updatePackages.value(SECURITY_UPGRADE_TYPE_STRING);
    const QStringList& unknownPackages = updatePackages.value(UNKNOWN_UPGRADE_STRING);
    const quint64 updateMode = m_model->updateMode();
    QMap<UpdateType, UpdateItemInfo*> resultMap;

    UpdateItemInfo* systemItemInfo = new UpdateItemInfo(SystemUpdate);
    systemItemInfo->setTypeString(SYSTEM_UPGRADE_TYPE_STRING);
    systemItemInfo->setName(tr("System Updates"));
    systemItemInfo->setExplain(tr("Fixed some known bugs and security vulnerabilities"));
    systemItemInfo->setPackages(systemPackages);
    setUpdateItemDownloadSize(systemItemInfo);
    systemItemInfo->setUpdateModeEnabled(updateMode & UpdateType::SystemUpdate);
    resultMap.insert(UpdateType::SystemUpdate, systemItemInfo);

    UpdateItemInfo* securityItemInfo = new UpdateItemInfo(SecurityUpdate);
    securityItemInfo->setTypeString(SECURITY_UPGRADE_TYPE_STRING);
    securityItemInfo->setName(tr("Security Updates"));
    securityItemInfo->setExplain(tr("Fixed some known bugs and security vulnerabilities"));
    securityItemInfo->setPackages(securityPackages);
    setUpdateItemDownloadSize(securityItemInfo);
    securityItemInfo->setUpdateModeEnabled(updateMode & UpdateType::SecurityUpdate);
    resultMap.insert(UpdateType::SecurityUpdate, securityItemInfo);

    UpdateItemInfo* unkownItemInfo = new UpdateItemInfo(UnknownUpdate);
    unkownItemInfo->setTypeString(UNKNOWN_UPGRADE_STRING);
    unkownItemInfo->setName(tr("Third-party Repositories"));
    unkownItemInfo->setPackages(unknownPackages);
    setUpdateItemDownloadSize(unkownItemInfo);
    unkownItemInfo->setUpdateModeEnabled(updateMode & UpdateType::UnknownUpdate);
    resultMap.insert(UpdateType::UnknownUpdate, unkownItemInfo);

    // 将更新日志根据`系统更新`or`安全更新`进行分类，并保存留用
    for (UpdateType type : resultMap.keys()) {
        UpdateLogHelper::ref().updateItemInfo(resultMap.value(type));
    }

    return resultMap;
}

bool UpdateWorker::checkDbusIsValid()
{
    return checkJobIsValid(m_checkUpdateJob) && checkJobIsValid(m_downloadJob);
}

void UpdateWorker::onSmartMirrorServiceIsValid(bool valid)
{
    m_smartMirrorInter->setSync(false);

    if (!valid) {
        m_smartMirrorInter->startServiceProcess();
    }
}

void UpdateWorker::createCheckUpdateJob(const QString& jobPath)
{
    qCInfo(DCC_UPDATE) << "Create check update job: " << jobPath;

    if (m_checkUpdateJob != nullptr) {
        qCInfo(DCC_UPDATE) << "Check update job existed";
        return;
    }
    m_checkUpdateJob = new JobInter("com.deepin.lastore", jobPath, QDBusConnection::systemBus(), this);

    connect(m_checkUpdateJob, &__Job::StatusChanged, this, &UpdateWorker::onCheckUpdateStatusChanged);
    connect(m_checkUpdateJob, &__Job::ProgressChanged, m_model, &UpdateModel::setCheckUpdateProgress, Qt::QueuedConnection);
    connect(qApp, &QApplication::aboutToQuit, this, [this] {
        if (m_checkUpdateJob) {
            delete m_checkUpdateJob.data();
        }
    });

    m_checkUpdateJob->ProgressChanged(m_checkUpdateJob->progress());
    m_checkUpdateJob->StatusChanged(m_checkUpdateJob->status());
}

void UpdateWorker::startDownload(int updateTypes)
{
    qCInfo(DCC_UPDATE) << "Start download, update types: " << updateTypes;
    cleanLaStoreJob(m_downloadJob);

    // 直接设置为正在下载状态, 否则切换下载界面等待时间稍长,体验不好
    m_model->setLastStatus(UpdatesStatus::DownloadWaiting, __LINE__, updateTypes);

    // 开始下载
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(m_managerInter->asyncCall("PrepareDistUpgradePartly", updateTypes), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, watcher] {
        if (!watcher->isError()) {
            watcher->reply().path();
            QDBusPendingReply<QDBusObjectPath> reply = watcher->reply();
            QDBusObjectPath data = reply.value();
            if (data.path().isEmpty()) {
                qCWarning(DCC_UPDATE) << "Download job path is empty, error:" << watcher->error().message();
                return;
            }
            setDownloadJob(data.path());
        } else {
            const QString& errorMessage = watcher->error().message();
            qCWarning(DCC_UPDATE) << "Start download failed, error:" << errorMessage;
            m_model->setLastErrorLog(DownloadFailed, errorMessage);
            m_model->setLastError(DownloadFailed, analyzeJobErrorMessage(errorMessage, DownloadFailed));
            cleanLaStoreJob(m_downloadJob);
        }
    });
}

void UpdateWorker::setIdleDownloadConfig(const IdleDownloadConfig& config)
{
    QDBusPendingCall call = m_updateInter->asyncCall("SetIdleDownloadConfig", QString(config.toJson()));
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [call, watcher] {
        watcher->deleteLater();
        if (call.isError()) {
            qCWarning(DCC_UPDATE) << "Set idle download config error:" << call.error().message();
        }
    });
}

void UpdateWorker::setUpdateMode(const quint64 updateMode)
{
    m_managerInter->setUpdateMode(updateMode);
}

void UpdateWorker::setAutoDownloadUpdates(const bool& autoDownload)
{
    m_updateInter->SetAutoDownloadUpdates(autoDownload);
    if (autoDownload == false) {
        m_updateInter->setAutoInstallUpdates(false);
    }
}

void UpdateWorker::setMirrorSource(const MirrorInfo& mirror)
{
    m_updateInter->SetMirrorSource(mirror.m_id);
}

void UpdateWorker::checkTestingChannelStatus()
{
    qCDebug(DCC_UPDATE) << "Testing:"
             << "check testing join status";
    const auto server = m_model->getTestingChannelServer();
    const auto machineID = m_model->getMachineID();
    auto http = new QNetworkAccessManager(this);
    QNetworkRequest request;
    request.setUrl(QUrl(server + QString("/api/v2/public/testing/machine/status/") + machineID));
    request.setRawHeader("content-type", "application/json");
    connect(http, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply) {
        reply->deleteLater();
        http->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(DCC_UPDATE) << "Testing:"
                     << "Network Error" << reply->errorString();
            return;
        }
        auto data = reply->readAll();
        qCDebug(DCC_UPDATE) << "Testing:"
                 << "machine status body" << data;
        auto doc = QJsonDocument::fromJson(data);
        auto obj = doc.object();
        auto status = obj["data"].toObject()["status"].toString();
        // Exit the loop if switch status is disable
        if (m_model->getTestingChannelStatus() != UpdateModel::TestingChannelStatus::WaitJoined) {
            return;
        }
        // If user has joined then install testing source package;
        if (status == "joined") {
            m_model->setTestingChannelStatus(UpdateModel::TestingChannelStatus::Joined);
            qCDebug(DCC_UPDATE) << "Testing:"
                     << "Install testing channel package";
            // 安装内测源之前执行一次apt update，避免刚安装的系统没有仓库索引导致安装失败
            checkForUpdates();
            // 延迟1秒是为了把安装任务放在update之后
            QThread::sleep(1);
            // lastore会自动等待apt update完成后再执行安装。
            m_managerInter->InstallPackage("testing channel", TestingChannelPackage);
            return;
        }
        // Run again after sleep
        QTimer::singleShot(5000, this, &UpdateWorker::checkTestingChannelStatus);
    });
    http->get(request);
}

void UpdateWorker::setTestingChannelEnable(const bool& enable)
{
    qCDebug(DCC_UPDATE) << "Testing:"
             << "TestingChannelEnableChange" << enable;
    if (enable) {
        m_model->setTestingChannelStatus(UpdateModel::TestingChannelStatus::WaitJoined);
    } else {
        m_model->setTestingChannelStatus(UpdateModel::TestingChannelStatus::NotJoined);
    }

    const auto server = m_model->getTestingChannelServer();
    const auto machineID = m_model->getMachineID();

    // 无论是加入还是退出都先在服务器标记退出
    // 避免重装系统后机器码相同，没申请就自动加入
    auto http = new QNetworkAccessManager(this);
    QNetworkRequest request;
    request.setUrl(QUrl(server + QString("/api/v2/public/testing/machine/") + machineID));
    request.setRawHeader("content-type", "application/json");
    connect(http, &QNetworkAccessManager::finished, this, [http](QNetworkReply* reply) {
        reply->deleteLater();
        http->deleteLater();
    });
    http->deleteResource(request);

    /* Disable Testing Channel */
    if (!enable) {
        // Uninstall testing source package if it is installed
        if (m_managerInter->PackageExists(TestingChannelPackage)) {
            qCDebug(DCC_UPDATE) << "Testing:"
                     << "Uninstall testing channel package";
            m_managerInter->RemovePackage("testing channel", TestingChannelPackage);
        }
        return;
    }

    /* Enable Testing Channel */
    auto u = m_model->getTestingChannelJoinURL();
    qCDebug(DCC_UPDATE) << "Testing:"
             << "open join page" << u.toString();
    QDesktopServices::openUrl(u);

    // Loop to check if user have joined
    QTimer::singleShot(1000, this, &UpdateWorker::checkTestingChannelStatus);
}

QString UpdateWorker::getTestingChannelSource()
{
    auto sourceFile = QString("/etc/apt/sources.list.d/%1.list").arg(TestingChannelPackage);
    QFile f(sourceFile);
    if (!f.open(QFile::ReadOnly | QFile::Text)) {
        return "";
    }
    QTextStream in(&f);
    while (!in.atEnd()) {
        auto line = in.readLine();
        if (line.startsWith("deb")) {
            auto fields = line.split(" ", QString::SkipEmptyParts);
            if (fields.length() >= 2) {
                auto sourceURL = fields[1];
                if (sourceURL.endsWith("/")) {
                    sourceURL.truncate(sourceURL.length() - 1);
                }
                return sourceURL;
            }
        }
    }
    return "";
}

// get all sources of the package
QStringList UpdateWorker::getSourcesOfPackage(const QString pkg, const QString version)
{
    QStringList sources;
    QProcess aptCacheProcess(this);
    QStringList args;
    args.append("madison");
    args.append(pkg);
    // exec apt-cache madison $pkg
    aptCacheProcess.start("apt-cache", args);
    aptCacheProcess.waitForFinished();
    while (aptCacheProcess.canReadLine()) {
        QString line(aptCacheProcess.readLine());
        auto fields = line.split("|", QString::SkipEmptyParts);
        for (QString& field : fields) {
            field = field.trimmed();
        }
        if (fields.length() <= 2) {
            continue;
        }
        auto p = fields[0], ver = fields[1], src = fields[2];
        src.truncate(fields[2].indexOf(" "));
        if (p == pkg) {
            if (version.length() == 0 || version == ver) {
                sources.append(src);
            }
        }
    }
    return sources.toSet().toList();
}

// checkCanExitTestingChannel check if the current env can exit internal test channel
void UpdateWorker::checkCanExitTestingChannel()
{
    // 如果在执行apt-get clean之后使用apt-cache会很慢，比正常情况慢23倍
    // 执行一次apt update，可以让apt-cache恢复到正常速度
    checkForUpdates();
    auto testingChannelSource = getTestingChannelSource();
    QProcess dpkgProcess(this);
    // exec dpkg -l
    dpkgProcess.start("dpkg", QStringList("-l"));
    dpkgProcess.waitForStarted();
    // read stdout by line
    while (dpkgProcess.state() == QProcess::Running) {
        dpkgProcess.waitForReadyRead();
        while (dpkgProcess.canReadLine()) {
            QString line(dpkgProcess.readLine());
            // skip uninstalled
            if (!line.startsWith("ii")) {
                continue;
            }
            auto field = line.split(" ", QString::SkipEmptyParts);
            // skip unknown format line
            if (field.length() <= 2) {
                continue;
            }
            auto pkg = field[1], version = field[2];
            // skip non system software
            if (!pkg.contains("dde") && !pkg.contains("deepin") && !pkg.contains("dtk") && !pkg.contains("uos")) {
                continue;
            }
            // Does the package exists only in the internal test source
            auto sources = getSourcesOfPackage(pkg, version);
            if (sources.length() == 1 && sources[0].contains(testingChannelSource)) {
                m_model->setCanExitTestingChannel(false);
                return;
            }
        }
    }
    m_model->setCanExitTestingChannel(true);
    return;
}

#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
void UpdateWorker::setSourceCheck(bool enable)
{
    m_lastoreSessionHelper->SetSourceCheckEnabled(enable);
}
#endif

void UpdateWorker::testMirrorSpeed()
{
    QList<MirrorInfo> mirrors = m_model->mirrorInfos();

    QStringList urlList;
    for (MirrorInfo& info : mirrors) {
        urlList << info.m_url;
    }

    // reset the data;
    m_model->setMirrorSpeedInfo(QMap<QString, int>());

    QFutureWatcher<int>* watcher = new QFutureWatcher<int>();
    connect(watcher, &QFutureWatcher<int>::resultReadyAt, [this, urlList, watcher, mirrors](int index) {
        QMap<QString, int> speedInfo = m_model->mirrorSpeedInfo();

        int result = watcher->resultAt(index);
        QString mirrorId = mirrors.at(index).m_id;
        speedInfo[mirrorId] = result;

        m_model->setMirrorSpeedInfo(speedInfo);
    });

    QPointer<QObject> guest(this);
    QFuture<int> future = QtConcurrent::mapped(urlList, std::bind(TestMirrorSpeedInternal, std::placeholders::_1, guest));
    watcher->setFuture(future);
}

void UpdateWorker::checkNetselect()
{
    QProcess* process = new QProcess;
    process->start("netselect", QStringList() << "127.0.0.1");
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if ((error == QProcess::FailedToStart) || (error == QProcess::Crashed)) {
            m_model->setNetselectExist(false);
            process->deleteLater();
        }
    });
    connect(process, static_cast<void (QProcess::*)(int)>(&QProcess::finished), this, [this, process](int result) {
        bool isNetselectExist = 0 == result;
        if (!isNetselectExist) {
            qCDebug(DCC_UPDATE) << "Netselect 127.0.0.1 :" << isNetselectExist;
        }
        m_model->setNetselectExist(isNetselectExist);
        process->deleteLater();
    });
}

void UpdateWorker::setSmartMirror(bool enable)
{
    m_smartMirrorInter->SetEnable(enable);

    QTimer::singleShot(100, this, [=] {
        Q_EMIT m_smartMirrorInter->serviceValidChanged(m_smartMirrorInter->isValid());
    });
}

#ifndef DISABLE_SYS_UPDATE_MIRRORS
void UpdateWorker::refreshMirrors()
{
    qCDebug(DCC_UPDATE) << QDir::currentPath();
    QFile file(":/update/themes/common/config/mirrors.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCDebug(DCC_UPDATE) << file.errorString();
        return;
    }
    QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();
    QList<MirrorInfo> list;
    for (auto item : array) {
        QJsonObject obj = item.toObject();
        MirrorInfo info;
        info.m_id = obj.value("id").toString();
        QString locale = QLocale::system().name();
        if (!(QLocale::system().name() == "zh_CN" || QLocale::system().name() == "zh_TW")) {
            locale = "zh_CN";
        }
        info.m_name = obj.value(QString("name_locale.%1").arg(locale)).toString();
        info.m_url = obj.value("url").toString();
        list << info;
    }
    m_model->setMirrorInfos(list);
    m_model->setDefaultMirror(list[0].m_id);
}
#endif

void UpdateWorker::setCheckUpdatesJob(const QString& jobPath)
{
    qCInfo(DCC_UPDATE) << "Set check updates job";
    UpdatesStatus state = m_model->updateStatus(CPT_Downloading);
    if (UpdatesStatus::Downloading != state && UpdatesStatus::DownloadPaused != state) {
        m_model->setLastStatus(UpdatesStatus::Checking, __LINE__);
    }
    createCheckUpdateJob(jobPath);
}

void UpdateWorker::setDownloadJob(const QString& jobPath)
{
    qCInfo(DCC_UPDATE) << "Set download job: " << jobPath;
    QMutexLocker locker(&m_downloadMutex);

    m_downloadJob = new JobInter("com.deepin.lastore", jobPath, QDBusConnection::systemBus(), this);
    connect(m_downloadJob, &__Job::ProgressChanged, m_model, &UpdateModel::setDownloadProgress);
    connect(m_downloadJob, &__Job::StatusChanged, this, &UpdateWorker::onDownloadStatusChanged);
    connect(m_downloadJob, &__Job::DescriptionChanged, this, [this](const QString &description) {
        if (m_downloadJob->status() == "failed") {
            m_model->setLastErrorLog(DownloadFailed, description);
        }
    });
    onDownloadStatusChanged(m_downloadJob->status());
    m_model->setDownloadProgress(m_downloadJob->progress());
}

void UpdateWorker::setAutoCleanCache(const bool autoCleanCache)
{
    m_managerInter->SetAutoClean(autoCleanCache);
}

void UpdateWorker::onJobListChanged(const QList<QDBusObjectPath>& jobs)
{
    qCInfo(DCC_UPDATE) << "Job list changed, size:" << jobs.size();

    for (const auto& job : jobs) {
        const QString jobPath = job.path();
        JobInter jobInter("com.deepin.lastore", jobPath, QDBusConnection::systemBus());

        // id maybe scrapped
        const QString& id = jobInter.id();
        if (!jobInter.isValid() || id.isEmpty())
            continue;

        qCInfo(DCC_UPDATE) << "Job id: " << id << ", job path: " << jobPath;
        if ((id == "update_source" || id == "custom_update") && m_checkUpdateJob == nullptr) {
            setCheckUpdatesJob(jobPath);
        } else if (id == "dist_upgrade" && m_distUpgradeJob == nullptr) {
            setDistUpgradeJob(jobPath);
        } else if (id == "prepare_dist_upgrade" && m_downloadJob == nullptr) {
            setDownloadJob(jobPath);
        }
    }
}

void UpdateWorker::setDistUpgradeJob(const QString& jobPath)
{
    qCInfo(DCC_UPDATE) << "Create dist upgrade job, path:" << jobPath;
    if (m_distUpgradeJob || jobPath.isEmpty()) {
        qCInfo(DCC_UPDATE) << "Job is not null or job path is empty";
        return;
    }

    m_distUpgradeJob = new JobInter("com.deepin.lastore", jobPath, QDBusConnection::systemBus(), this);
    connect(m_distUpgradeJob, &__Job::ProgressChanged, m_model, &UpdateModel::setDistUpgradeProgress);
    connect(m_distUpgradeJob, &__Job::StatusChanged, this, &UpdateWorker::onDistUpgradeStatusChanged);
    connect(m_distUpgradeJob, &__Job::DescriptionChanged, this, [this](const QString &description) {
        if (m_distUpgradeJob->status() == "failed") {
            m_model->setLastErrorLog(UpgradeFailed, description);
        }
    });
    m_model->setDistUpgradeProgress(m_distUpgradeJob->progress());
    onDistUpgradeStatusChanged(m_distUpgradeJob->status());
}

void UpdateWorker::onDistUpgradeStatusChanged(const QString& status)
{
    // 无需处理ready状态
    static const QMap<QString, UpdatesStatus> DIST_UPGRADE_STATUS_MAP = {
        { "running", Upgrading },
        { "failed", UpgradeFailed },
        { "succeed", UpgradeSuccess },
        { "end", UpgradeComplete }
    };

    qCInfo(DCC_UPDATE) << "Dist upgrade status changed:" << status;
    if (DIST_UPGRADE_STATUS_MAP.contains(status)) {
        const auto updateStatus = DIST_UPGRADE_STATUS_MAP.value(status);
        if (updateStatus == UpgradeComplete) {
            cleanLaStoreJob(m_distUpgradeJob);
        } else {
            if (updateStatus == UpgradeFailed && m_distUpgradeJob) {
                const QString& description = m_distUpgradeJob->description();
                m_model->setLastErrorLog(UpgradeFailed, description);
                m_model->setLastError(UpgradeFailed, analyzeJobErrorMessage(description));

            }
        }
    } else {
        qCWarning(DCC_UPDATE) << "Unknown dist upgrade status";
    }
}

void UpdateWorker::onIconThemeChanged(const QString& theme)
{
    m_iconThemeState = theme;
}

void UpdateWorker::onCheckUpdateStatusChanged(const QString& value)
{
    qCInfo(DCC_UPDATE) << "Check update status changed: " << value;
    if (value == "failed" || value.isEmpty()) {
        if (m_checkUpdateJob != nullptr) {
            m_managerInter->CleanJob(m_checkUpdateJob->id());
            const auto& description = m_checkUpdateJob->description();
            m_model->setLastErrorLog(CheckingFailed, description);
            m_model->setLastError(CheckingFailed, analyzeJobErrorMessage(description, CheckingFailed));
            m_model->setLastStatus(CheckingFailed, __LINE__);
            deleteJob(m_checkUpdateJob);
        }
    } else if (value == "success" || value == "succeed") {
        auto watcher = new QDBusPendingCallWatcher(m_managerInter->asyncCall("GetUpdateLogs", SystemUpdate | SecurityUpdate), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, [this, watcher] {
            watcher->deleteLater();
            if (!watcher->isError()) {
                QDBusPendingReply<QString> reply = watcher->reply();
                UpdateLogHelper::ref().handleUpdateLog(reply.value());
            } else {
                qWarning() << "Get update log failed";
            }
            // 日志处理完了再显示更新内容界面
            m_model->setLastStatus(CheckingSucceed, __LINE__);
            setUpdateInfo();
        });
    } else if (value == "end") {
        deleteJob(m_checkUpdateJob);
    }
}

void UpdateWorker::refreshHistoryAppsInfo()
{
    m_model->setHistoryAppInfos(m_updateInter->ApplicationUpdateInfos(QLocale::system().name()));
}

void UpdateWorker::refreshLastTimeAndCheckCircle()
{
    QString checkTime;
    m_updateInter->GetCheckIntervalAndTime(checkTime);
    m_model->setLastCheckUpdateTime(checkTime);
}

void UpdateWorker::setUpdateNotify(const bool notify)
{
    m_updateInter->SetUpdateNotify(notify);
}

void UpdateWorker::onDownloadJobCtrl(int updateCtrlType)
{
    if (m_downloadJob == nullptr) {
        qCWarning(DCC_UPDATE) << "Download job is nullptr";
        return;
    }

    switch (updateCtrlType) {
    case UpdateCtrlType::Start:
        qCInfo(DCC_UPDATE) << "Start download job";
        m_managerInter->StartJob(m_downloadJob->id());
        break;
    case UpdateCtrlType::Pause:
        qCInfo(DCC_UPDATE) << "Pause download job";
        m_managerInter->PauseJob(m_downloadJob->id());
        break;
    }
}

bool UpdateWorker::checkJobIsValid(QPointer<JobInter> dbusJob)
{
    if (dbusJob.isNull())
        return false;

    if (dbusJob->isValid())
        return true;

    dbusJob->deleteLater();
    return false;
}

void UpdateWorker::deleteJob(QPointer<JobInter> dbusJob)
{
    if (!dbusJob.isNull()) {
        dbusJob->deleteLater();
        dbusJob = nullptr;
    }
}

void UpdateWorker::cleanLaStoreJob(QPointer<JobInter> dbusJob)
{
    if (dbusJob != nullptr) {
        m_managerInter->CleanJob(dbusJob->id());
        deleteJob(dbusJob);
    }
}

UpdateErrorType UpdateWorker::analyzeJobErrorMessage(const QString& jobDescription, UpdatesStatus status)
{
    qCWarning(DCC_UPDATE) << "Job description:" << jobDescription;

    QJsonParseError err_rpt;
    QJsonDocument jobErrorMessage = QJsonDocument::fromJson(jobDescription.toUtf8(), &err_rpt);

    if (err_rpt.error != QJsonParseError::NoError) {
        qCWarning(DCC_UPDATE) << "Parse json failed";
        return UnKnown;
    }
    const QJsonObject& object = jobErrorMessage.object();
    QString errorType = object.value("ErrType").toString();
    if (errorType.contains("fetchFailed", Qt::CaseInsensitive) || errorType.contains("IndexDownloadFailed", Qt::CaseInsensitive)) {
        if (status == DownloadFailed) {
            return DownloadingNoNetwork;
        }

        return NoNetwork;
    }
    if (errorType.contains("unmetDependencies", Qt::CaseInsensitive) || errorType.contains("dependenciesBroken", Qt::CaseInsensitive)) {
        return DependenciesBrokenError;
    }
    if (errorType.contains("insufficientSpace", Qt::CaseInsensitive)) {
        if (status == DownloadFailed) {
            return DownloadingNoSpace;
        }
        return NoSpace;
    }
    if (errorType.contains("interrupted", Qt::CaseInsensitive)) {
        return DpkgInterrupted;
    }

    if (errorType.contains("platformUnreachable", Qt::CaseInsensitive)) {
        return PlatformUnreachable;
    }

    if (errorType.contains("invalidSourceList", Qt::CaseInsensitive)) {
        return InvalidSourceList;
    }

    return UnKnown;
}

void UpdateWorker::onDownloadStatusChanged(const QString& value)
{
    qCInfo(DCC_UPDATE) << "Download status changed: " << value;
    if (value == "failed") {
        const auto& description = m_downloadJob->description();
        m_model->setLastErrorLog(DownloadFailed, description);
        m_model->setLastError(DownloadFailed, analyzeJobErrorMessage(description, DownloadFailed));
    } else if (value == "end") {
        // 有多个下载项时,每下载完一个就会收到`end`,全部下载完毕后再析构job
        if (m_model->allUpdateStatus().contains(Downloading)) {
            qCInfo(DCC_UPDATE) << "Downloading, do not handle `end` status";
            return;
        }
        deleteJob(m_downloadJob);
    }
}

bool UpdateWorker::isUpdatesAvailable(const QMap<QString, QStringList>& updatablePackages)
{
    QMap<UpdateType, QString> keyMap;
    keyMap.insert(UpdateType::SystemUpdate, SYSTEM_UPGRADE_TYPE_STRING);
    keyMap.insert(UpdateType::UnknownUpdate, UNKNOWN_UPGRADE_STRING);
    keyMap.insert(UpdateType::SecurityUpdate, SECURITY_UPGRADE_TYPE_STRING);
    bool available = false;
    for (auto item : keyMap.keys()) {
        if ((m_model->updateMode() & item) && updatablePackages.value(keyMap.value(item)).count() > UPDATE_PACKAGE_SIZE) {
            available = true;
            break;
        }
    }

    return available;
}

void UpdateWorker::onClassifiedUpdatablePackagesChanged(const QMap<QString, QStringList>& packages)
{
    m_model->updatePackages(packages);
}

void UpdateWorker::onRequestRetry(int type, int updateTypes)
{
    const auto controlType = static_cast<ControlPanelType>(type);
    const auto updateStatus = m_model->updateStatus(controlType);
    const auto lastError = m_model->lastError(m_model->updateStatus(controlType));

    qWarning() << "Control type:" << controlType
               << ", update status:" << updateStatus
               << ", update types:" << updateTypes;

    if (updateStatus == UpgradeFailed && lastError == DpkgInterrupted) {
        if (m_fixErrorJob != nullptr) {
            qCWarning(DCC_UPDATE) << "Fix error job is nullptr";
            return;
        }
        const QString& errorTypeString = UpdateModel::updateErrorToString(lastError);
        qCInfo(DCC_UPDATE) << "Call `FixError` function, error type:" << errorTypeString;
        QDBusReply<QDBusObjectPath> reply = m_managerInter->call("FixError", errorTypeString);
        if (reply.isValid()) {
            QString path = reply.value().path();
            m_fixErrorJob = new JobInter("com.deepin.lastore", path, QDBusConnection::systemBus());
            connect(m_fixErrorJob, &JobInter::StatusChanged, this, [updateTypes, lastError, this](const QString status) {
                qCInfo(DCC_UPDATE) << "Fix error job status changed :" << status;
                if (status == "succeed" || status == "failed" || status == "end") {
                    deleteJob(m_fixErrorJob);
                    if (status == "succeed") {
                        doUpgrade(updateTypes, false);
                    } else if (status == "failed") {
                        m_model->setLastError(UpgradeFailed, lastError);
                    }
                }
            });
        } else {
            qCWarning(DCC_UPDATE) << "Call `FixError` reply is invalid, error: " << reply.error().message();
        }
        return;
    }

    if (updateStatus == UpgradeFailed || updateStatus == BackupFailed) {
        doUpgrade(updateTypes, updateStatus == BackupFailed);
        return;
    }

    if (updateStatus == DownloadFailed) {
        startDownload(updateTypes);
        return;
    }

    if (updateStatus == CheckingFailed) {
        checkForUpdates();
        return;
    }

    if (lastError == UnKnown || lastError == NoError) {
        qCWarning(DCC_UPDATE) << "Unknown error, recheck update";
        checkForUpdates();
    }
}

void UpdateWorker::setUpdateItemDownloadSize(UpdateItemInfo* updateItem)
{
    if (updateItem->packages().isEmpty()) {
        return;
    }

    QDBusPendingCall call = m_managerInter->asyncCall("QueryAllSizeWithSource", updateItem->updateType());
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, updateItem, [updateItem, call, watcher] {
        watcher->deleteLater();
        if (!call.isError()) {
            QDBusReply<qlonglong> reply = call.reply();
            qlonglong value = reply.value();
            qCInfo(DCC_UPDATE) << "Packages' size:" << value << ", name:" << updateItem->name();
            updateItem->setDownloadSize(value);
        } else {
            qCWarning(DCC_UPDATE) << "Get packages size error:" << call.error().message();
        }
    });
}

void UpdateWorker::onRequestLastoreHeartBeat()
{
    m_updateInter->asyncCall("GetCheckIntervalAndTime");
}

/**
 * @brief 发行版or内测版
 *
 * @return 1: 发行版，2：内测版
 */
int UpdateWorker::isUnstableResource() const
{
    const int RELEASE_VERSION = 1;
    const int UNSTABLE_VERSION = 2;

    DConfig* config = DConfig::create("org.deepin.unstable", "org.deepin.unstable", QString(), nullptr);
    config->deleteLater();
    if (!config) {
        qCWarning(DCC_UPDATE) << "Can not find org.deepin.unstable or an error occurred in DTK";
        return RELEASE_VERSION;
    }

    if (!config->keyList().contains("updateUnstable")) {
        return RELEASE_VERSION;
    }

    const QString& value = config->value("updateUnstable", "Enabled").toString();
    return "Enabled" == value ? UNSTABLE_VERSION : RELEASE_VERSION;
}

void UpdateWorker::setDownloadSpeedLimitConfig(const QString& config)
{
    QDBusPendingCall call = m_updateInter->asyncCall("SetDownloadSpeedLimit", config);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [call, watcher] {
        watcher->deleteLater();
        if (call.isError()) {
            qCWarning(DCC_UPDATE) << "Set download speed limit config error: " << call.error().message();
        }
    });
}

void UpdateWorker::onUpdateModeChanged(qulonglong value)
{
    m_model->setUpdateMode(value);
}

void UpdateWorker::onRequestCheckUpdateModeChanged(int type, bool isChecked)
{
    QDBusInterface managerInter("com.deepin.lastore",
        "/com/deepin/lastore",
        "com.deepin.lastore.Manager",
        QDBusConnection::systemBus());
    const int currentMode = m_model->checkUpdateMode();
    const int outMode = isChecked ? (currentMode | type) : (currentMode & ~type);
    managerInter.setProperty("CheckUpdateMode", outMode);
}

void UpdateWorker::doUpgrade(int updateTypes, bool doBackup)
{
    qCInfo(DCC_UPDATE) << "Do upgrade, update types:" << updateTypes << ", whether do backup:" << doBackup;

    cleanLaStoreJob(m_distUpgradeJob);

    qCInfo(DCC_UPDATE) << "Update types:" << updateTypes << ", do backup:" << doBackup;
    QDBusPendingCall call = m_managerInter->asyncCall("DistUpgradePartly", updateTypes, doBackup);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, updateTypes, call, watcher] {
        watcher->deleteLater();
        if (!call.isError()) {
            m_model->setLastStatus(UpgradeWaiting, __LINE__, updateTypes);
            QList<QVariant> outArgs = call.reply().arguments();
            if (outArgs.count() > 0) {
                setDistUpgradeJob(outArgs.at(0).toString());
            }
        } else {
            qCInfo(DCC_UPDATE) << "Call `DistUpgradePartly` failed, error:" << call.error().message();
        }
    });
}

void UpdateWorker::stopDownload()
{
    if (!m_downloadJob) {
        qCWarning(DCC_UPDATE) << "Download job is null";
        return;
    }

    cleanLaStoreJob(m_downloadJob);
}

/**
 * @brief 使用电源或者电量大于60认为电量满足条件
 *
 */
void UpdateWorker::checkPower()
{
    qCInfo(DCC_UPDATE) << "Check power";
    bool onBattery = m_powerInter->onBattery();
    if (!onBattery) {
        m_model->setBatterIsOK(true);
        return;
    }

    double data = m_powerInter->batteryPercentage();
    int batteryPercentage = uint(qMin(100.0, qMax(0.0, data)));
    m_model->setBatterIsOK(batteryPercentage >= LOWEST_BATTERY_PERCENT);
}

void UpdateWorker::setP2PUpdateEnabled(bool enabled)
{
    // Don't care the return value
    m_updateInter->asyncCall("SetP2PUpdateEnable", enabled);
}

}
}
