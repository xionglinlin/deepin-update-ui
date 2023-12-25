// SPDX-FileCopyrightText: 2015 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "public_func.h"
#include "dbus/dbuslockservice.h"

#include <QJsonDocument>

#include <com_deepin_daemon_accounts.h>
#include <com_deepin_daemon_accounts_user.h>

using AccountsInter = com::deepin::daemon::Accounts;
using UserInter = com::deepin::daemon::accounts::User;

QPixmap loadPixmap(const QString &file)
{
    qreal ratio = 1.0;
    qreal devicePixel = qApp->devicePixelRatio();

    QPixmap pixmap;

    if (!qFuzzyCompare(ratio, devicePixel)) {
        QImageReader reader;
        reader.setFileName(qt_findAtNxFile(file, devicePixel, &ratio));
        if (reader.canRead()) {
            reader.setScaledSize(reader.size() * (devicePixel / ratio));
            pixmap = QPixmap::fromImage(reader.read());
            pixmap.setDevicePixelRatio(devicePixel);
        }
    } else {
        pixmap.load(file);
    }

    return pixmap;
}

QString getCurrentLocale()
{
    static const QString DEFAULT_LOCALE = QStringLiteral("en_US");

    DBusLockService lockService("com.deepin.dde.LockService", "/com/deepin/dde/LockService", QDBusConnection::systemBus());
    const QString &currentUserJson = lockService.CurrentUser();
    qInfo() << "Get current locale, current user:" << currentUserJson;

    QJsonParseError jsonParseError;
    const auto& userDoc = QJsonDocument::fromJson(currentUserJson.toUtf8(), &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError || userDoc.isEmpty()) {
        qWarning("Failed to obtain current user information from lock service");
        return DEFAULT_LOCALE;
    }

    const auto& userObj = userDoc.object();
    const auto& uid = userObj.value("Uid").toInt();
    if (uid == 0) {
        qWarning() << "Current user's uid is invalid";
        return DEFAULT_LOCALE;
    }

    const QString& path = QStringLiteral("/com/deepin/daemon/Accounts/User") + QString::number(uid);
    qInfo() << "Current user account path: " << path;
    UserInter userInter("com.deepin.daemon.Accounts", path, QDBusConnection::systemBus());
    if (!userInter.isValid()) {
        qWarning() << "User DBus interface is invalid, error: " << userInter.lastError().message();
        return DEFAULT_LOCALE;
    }
    userInter.setSync(true);
    userInter.setTimeout(3000);
    return userInter.locale();
}
