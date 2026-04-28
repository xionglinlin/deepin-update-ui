//SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
//SPDX-License-Identifier: GPL-3.0-or-later
#include "mirrorinfolist.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logMirrorInfo, "dde.update.mirrorinfo")

MirrorInfo::MirrorInfo()
{
}

const QDBusArgument &operator>>(const QDBusArgument &argument, MirrorInfo &info)
{
    qCDebug(logMirrorInfo) << "Deserializing MirrorInfo from DBus";
    argument.beginStructure();
    argument >> info.m_id;
    argument >> info.m_url;
    argument >> info.m_name;
    argument.endStructure();
    qCDebug(logMirrorInfo) << "MirrorInfo deserialized - ID:" << info.m_id << "name:" << info.m_name;

    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const MirrorInfo &info)
{
    qCDebug(logMirrorInfo) << "Serializing MirrorInfo to DBus - ID:" << info.m_id << "name:" << info.m_name;
    argument.beginStructure();
    argument << info.m_id;
    argument << info.m_url;
    argument << info.m_name;
    argument.endStructure();

    return argument;
}

QDebug operator<<(QDebug argument, const MirrorInfo &info)
{
    qCDebug(logMirrorInfo) << "Debug output for MirrorInfo";
    argument << "mirror id: " << info.m_id;
    argument << "mirror url: " << info.m_url;
    argument << "mirror name: " << info.m_name;

    return argument;
}

void registerMirrorInfoListMetaType()
{
    qCDebug(logMirrorInfo) << "Registering MirrorInfo meta types";
    qRegisterMetaType<MirrorInfo>();
    qDBusRegisterMetaType<MirrorInfo>();
    qRegisterMetaType<MirrorInfoList>();
    qDBusRegisterMetaType<MirrorInfoList>();
}
