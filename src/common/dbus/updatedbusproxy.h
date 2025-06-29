// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef UPDATEDBUSPROXY_H
#define UPDATEDBUSPROXY_H

#include <DDBusInterface>

#include <QDBusObjectPath>
#include <QDBusPendingReply>
#include <QObject>
#include <QDBusServiceWatcher>

typedef QMap<QString, QStringList> LastoreUpdatePackagesInfo;
typedef QMap<QString, double> BatteryPercentageInfo;

using Dtk::Core::DDBusInterface;

class QDBusMessage;
class QDBusInterface;

class UpdateDBusProxy : public QObject
{
    Q_OBJECT
public:
    explicit UpdateDBusProxy(QObject *parent = nullptr);
    ~UpdateDBusProxy();

public:
    // hostname
    QString staticHostname() const;

    // updater
    Q_PROPERTY(bool UpdateNotify READ updateNotify NOTIFY UpdateNotifyChanged)
    bool updateNotify();
    void SetUpdateNotify(bool in0);
    LastoreUpdatePackagesInfo classifiedUpdatablePackages();
    double GetCheckIntervalAndTime(QString &out1);

    Q_PROPERTY(bool AutoDownloadUpdates READ autoDownloadUpdates NOTIFY AutoDownloadUpdatesChanged)
    bool autoDownloadUpdates();
    void SetAutoDownloadUpdates(bool in0);

    Q_PROPERTY(bool AutoInstallUpdates READ autoInstallUpdates WRITE setAutoInstallUpdates NOTIFY
                       AutoInstallUpdatesChanged)
    bool autoInstallUpdates();
    void setAutoInstallUpdates(bool value);

    qulonglong autoInstallUpdateType();

    Q_PROPERTY(bool AutoCheckUpdates READ autoCheckUpdates NOTIFY AutoCheckUpdatesChanged)
    bool autoCheckUpdates();
    void SetAutoCheckUpdates(bool in0);
    void SetMirrorSource(const QString &in0);

    // ManagerInter
    Q_PROPERTY(bool AutoClean READ autoClean NOTIFY AutoCleanChanged)
    bool autoClean();

    Q_PROPERTY(qulonglong UpdateMode READ updateMode WRITE setUpdateMode NOTIFY UpdateModeChanged)
    quint64 updateMode();
    void setUpdateMode(quint64 value);

    Q_PROPERTY(QList<QDBusObjectPath> JobList READ jobList NOTIFY JobListChanged)
    QList<QDBusObjectPath> jobList();

    Q_PROPERTY(QString updateStatus  READ updateStatus NOTIFY UpdateStatusChanged)
    QString updateStatus();

    Q_PROPERTY(bool ImmutableAutoRecovery READ immutableAutoRecovery NOTIFY ImmutableAutoRecoveryChanged)
    bool immutableAutoRecovery();

    QString hardwareId();

    quint64 checkUpdateMode();
    void setCheckUpdateMode(quint64 value);

    QString idleDownloadConfig();
    QString downloadSpeedLimitConfig();
    bool p2pUpdateEnable();

    QDBusPendingCall CanRollback();
    QDBusPendingCall ConfirmRollback(bool confirm);

    bool managerInterIsValid() const { return m_managerInter && m_managerInter->isValid(); }

    QDBusPendingCall Poweroff(bool reboot = false);

    QDBusPendingReply<QDBusObjectPath> UpdateSource();
    void CleanJob(const QString &in0);
    void SetAutoClean(bool in0);
    void StartJob(const QString &in0);
    void PauseJob(const QString &in0);
    QDBusPendingReply<QDBusObjectPath> InstallPackage(const QString &jobname,
                                                      const QString &packages);
    QDBusPendingReply<QDBusObjectPath> RemovePackage(const QString &jobname,
                                                     const QString &packages);
    QDBusPendingReply<QList<QDBusObjectPath> > ClassifiedUpgrade(qulonglong in0);
    QDBusPendingReply<qlonglong> PackagesDownloadSize(const QStringList &in0);
    QDBusPendingReply<bool> PackageExists(const QString &pkgid);
    QDBusPendingReply<QDBusObjectPath> DistUpgrade();
    QDBusPendingReply<QDBusObjectPath> DistUpgradePartly(int updateTypes, bool doBackup);
    QDBusPendingReply<void> SetDownloadSpeedLimit(const QString &config);
    QDBusPendingReply<qlonglong> QueryAllSizeWithSource(int updateType);
    QDBusPendingReply<QString> GetUpdateLogs(int updateType);
    QDBusPendingReply<void> SetIdleDownloadConfig(const QString &config);
    QDBusPendingReply<QDBusObjectPath> PrepareDistUpgradePartly(int updateMode);
    QDBusPendingReply<QDBusObjectPath> fixError(const QString &errorType);
    QDBusPendingCall CheckUpgrade(int checkMode, int checkOrder);
    QDBusPendingReply<void> ExportUpdateDetails(const QString &filename);


    // Power
    bool onBattery();
    BatteryPercentageInfo batteryPercentage();

    // Atomic Upgrade
    void commit(const QString &commitDate);

    bool atomBackupIsRunning();
    // Smart Mirror
    Q_PROPERTY(bool Enable READ enable NOTIFY EnableChanged)
    bool enable() const;
    void SetEnable(bool enable);

    // lockService
    QString CurrentUser();

    // shutdownFront
    void Restart();
    void UpdateAndReboot();
    void UpdateAndShutdown();
signals:
    // updater
    void UpdateNotifyChanged(bool value) const;
    void AutoDownloadUpdatesChanged(bool value) const;
    void AutoInstallUpdatesChanged(bool value) const;
    void AutoInstallUpdateTypeChanged(qulonglong value) const;
    void MirrorSourceChanged(const QString &value) const;
    void AutoCheckUpdatesChanged(bool value) const;
    void ClassifiedUpdatablePackagesChanged(LastoreUpdatePackagesInfo value) const;

    // ManagerInter
    void JobListChanged(const QList<QDBusObjectPath> &value) const;
    void AutoCleanChanged(bool value) const;
    void UpdateModeChanged(qulonglong value) const;
    void UpdateStatusChanged(QString value) const;
    void ImmutableAutoRecoveryChanged(bool value) const;
    void managerInterServiceValidChanged(bool value) const;

    // Power
    void OnBatteryChanged(bool value) const;
    void BatteryPercentageChanged(BatteryPercentageInfo value) const;

    // Atomic Upgrade
    void StateChanged(int operate, int state, QString version, QString message);
    void RunningChanged(bool value) const;
    // Smart Mirror
    void EnableChanged(bool enable);
private:
    DDBusInterface *m_hostname1Inter;
    DDBusInterface *m_updateInter;
    DDBusInterface *m_managerInter;
    DDBusInterface *m_powerInter;
    DDBusInterface *m_atomicUpgradeInter;
    DDBusInterface *m_smartMirrorInter;
    DDBusInterface *m_login1Inter;
    DDBusInterface *m_lockServiceInter;
    DDBusInterface *m_shutdownFrontInter;

    QDBusServiceWatcher *m_interWatcher;
};

#endif // UPDATEDBUSPROXY_H
