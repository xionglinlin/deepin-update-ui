// SPDX-FileCopyrightText: 2016 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef COMMON_H
#define COMMON_H

#include <QObject>
#include <QString>
#include <QLoggingCategory>

#include <qregexp.h>
#include <vector>

using namespace std;

namespace dcc {
namespace update {

Q_NAMESPACE

const double Epsion = 1e-6;
const QString SYSTEM_UPGRADE_TYPE_STRING = "system_upgrade";
const QString SECURITY_UPGRADE_TYPE_STRING = "security_upgrade";
const QString UNKNOWN_UPGRADE_STRING = "unknown_upgrade";

Q_DECLARE_LOGGING_CATEGORY(DCC_UPDATE)

enum UpdatesStatus {
    Default,
    Checking,
    CheckingFailed,
    CheckingSucceed,
    Updated,
    UpdatesAvailable,
    DownloadWaiting,
    Downloading,
    DownloadPaused,
    Downloaded,
    DownloadFailed,

    // 点击下载按钮后的中间状态
    UpgradeWaiting,

    // 备份相关
    BackingUp,
    BackupFailed,
    BackupSuccess,

    // 升级
    UpgradeReady, // 备份阶段状态更新类型的状态保持UpgradeReady，直到开始安装
    Upgrading,
    UpgradeFailed,
    UpgradeSuccess,
    UpgradeComplete,

    // 非真实更新状态，可随系统配置变化
    SystemIsNotActive,
    UpdateIsDisabled,
    AllUpdateModeDisabled
};
Q_ENUM_NS(UpdatesStatus)

enum UpdateErrorType {
    NoError,
    UnKnown,
    NoNetwork,
    DownloadingNoNetwork,
    DownloadingNoSpace,
    NoSpace,
    DependenciesBrokenError,
    DpkgInterrupted,
    CanNotBackup,
    BackupFailedUnknownReason,
    DpkgError,
    FileMissing,
    PlatformUnreachable,
    InvalidSourceList,
};
Q_ENUM_NS(UpdateErrorType)

enum UpdateType {
    Invalid = 0, // 无效
    SystemUpdate = 1 << 0, // 系统
    AppStoreUpdate = 1 << 1, // 应用商店（1050版本弃用）
    SecurityUpdate = 1 << 2, // 安全
    UnknownUpdate = 1 << 3, // 未知来源
    OnlySecurityUpdate = 1 << 4 // 仅安全更新（1060版本弃用）
};
Q_ENUM_NS(UpdateType)

enum UpdateCtrlType {
    Start = 0,
    Pause
};

enum ControlPanelType {
    CPT_Invalid,
    CPT_NeedRestart,
    CPT_UpgradeFailed,
    CPT_BackupFailed,
    CPT_Upgrade,
    CPT_Downloaded,
    CPT_DownloadFailed,
    CPT_Downloading,
    CPT_Available,
    CPT_Checking
};
Q_ENUM_NS(ControlPanelType)

enum CtrlWidgetType {
    CtrlState_None,
    CtrlState_Check,
    CtrlState_Update,
};
Q_ENUM_NS(CtrlWidgetType)

static inline QString formatCap(qulonglong cap, const int size = 1024)
{
    static QString type[] = { "B", "KB", "MB", "GB", "TB" };

    if (cap < qulonglong(size)) {
        return QString::number(cap) + type[0];
    }
    if (cap < qulonglong(size) * size) {
        return QString::number(double(cap) / size, 'f', 2) + type[1];
    }
    if (cap < qulonglong(size) * size * size) {
        return QString::number(double(cap) / size / size, 'f', 2) + type[2];
    }
    if (cap < qulonglong(size) * size * size * size) {
        return QString::number(double(cap) / size / size / size, 'f', 2) + type[3];
    }

    return QString::number(double(cap) / size / size / size / size, 'f', 2) + type[4];
}

static inline vector<double> getNumListFromStr(const QString& str)
{
    //筛选出字符串中的数字
    QRegExp rx("-?[1-9]\\d*\\.\\d*|0+.[0-9]+|-?0\\.\\d*[1-9]\\d*|-?\\d+");
    int pos = 0;
    vector<double> v;
    while ((pos = rx.indexIn(str, pos)) != -1) {
        pos += rx.matchedLength();
        v.push_back(rx.cap(0).toDouble());
    }
    return v;
}

}
}

#endif // COMMON_H
