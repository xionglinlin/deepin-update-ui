// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logwatcherhelper.h"

#include <QDebug>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingCall>

#include <unistd.h>
#include <fcntl.h>

#include "../dbus/updatedbusproxy.h"

LogWatcherHelper::LogWatcherHelper(UpdateDBusProxy *dbusProxy, QObject *parent)
    : QObject(parent)
    , m_data(QString())
    , m_dbusProxy(dbusProxy)
    , m_readFd(-1)
    , m_writeFd(-1)
    , m_socketNotifier(nullptr)
{

}

LogWatcherHelper::~LogWatcherHelper()
{
    stopWatchFile();
}

void LogWatcherHelper::startWatchFile()
{
    qDebug() << "Start watch update log via DBus interface";
    
    if (!m_dbusProxy) {
        qWarning() << "DBus proxy is null, cannot start watching";
        return;
    }
    
    if (m_readFd != -1) {
        qWarning() << "Log watcher already started";
        return;
    }

    // 设置管道并调用 DBus 接口
    setupLogPipe();
}

void LogWatcherHelper::stopWatchFile()
{
    qDebug() << "Stop watch update log";
    
    // 停止文件描述符监听
    if (m_socketNotifier) {
        m_socketNotifier->setEnabled(false);
        delete m_socketNotifier;
        m_socketNotifier = nullptr;
    }
    
    // 关闭文件描述符
    if (m_readFd != -1) {
        close(m_readFd);
        m_readFd = -1;
    }
    
    if (m_writeFd != -1) {
        close(m_writeFd);
        m_writeFd = -1;
    }

    // 重置数据
    m_data = QString();
}

void LogWatcherHelper::setupLogPipe()
{
    // 创建管道
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        qWarning() << "Failed to create pipe for log reading";
        return;
    }

    m_readFd = pipefd[0];
    m_writeFd = pipefd[1];

    // 设置读端为非阻塞
    int flags = fcntl(m_readFd, F_GETFL, 0);
    fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

    // 设置 QSocketNotifier 来监听读端文件描述符
    m_socketNotifier = new QSocketNotifier(m_readFd, QSocketNotifier::Read, this);
    connect(m_socketNotifier, &QSocketNotifier::activated, this, &LogWatcherHelper::onDataAvailable);

    // 第一步：先获取历史日志 (realtime=false)
    qDebug() << "First getting historical logs...";
    QDBusPendingCall historyCall = m_dbusProxy->GetUpdateDetails(m_writeFd, false);
    QDBusPendingCallWatcher* historyWatcher = new QDBusPendingCallWatcher(historyCall, this);
    
    connect(historyWatcher, &QDBusPendingCallWatcher::finished, this, [this, historyWatcher](void) {
        // 可能获取历史日志失败（比如没有历史日志），但是不影响实时日志的获取
        if (historyWatcher->isError()) {
            qWarning() << "GetUpdateDetails for history failed:" << historyWatcher->error().message();
            historyWatcher->deleteLater();
        }

        qDebug() << "Historical logs completed, starting realtime monitoring...";
        
        // 历史日志获取完成，直接启动实时日志监听
        startRealtimeLogAfterHistory();
        
        historyWatcher->deleteLater();
    });
}

void LogWatcherHelper::startRealtimeLogAfterHistory()
{
    // 启用文件描述符监听
    if (m_socketNotifier) {
        m_socketNotifier->setEnabled(true);
    }
    
    // 开始获取实时增量日志 (realtime=true)
    QDBusPendingCall realtimeCall = m_dbusProxy->GetUpdateDetails(m_writeFd, true);
    QDBusPendingCallWatcher* realtimeWatcher = new QDBusPendingCallWatcher(realtimeCall, this);
    
    connect(realtimeWatcher, &QDBusPendingCallWatcher::finished, this, [this, realtimeWatcher](void) {
        
        if (realtimeWatcher->isError()) {
            qWarning() << "GetUpdateDetails for realtime failed:" << realtimeWatcher->error().message();
            stopWatchFile();
        } else {
            qDebug() << "Realtime log monitoring started successfully";
        }
        
        realtimeWatcher->deleteLater();
    });
}

void LogWatcherHelper::onDataAvailable()
{
    readAvailableData();
}

void LogWatcherHelper::readAvailableData()
{
    if (m_readFd == -1) {
        return;
    }

    QByteArray buffer;
    char readBuffer[4096];
    ssize_t bytesRead;
    
    while ((bytesRead = read(m_readFd, readBuffer, sizeof(readBuffer))) > 0) {
        buffer.append(readBuffer, bytesRead);
    }
    
    if (!buffer.isEmpty()) {
        QString newContent = QString::fromUtf8(buffer);

        m_data.append(newContent);
        emit incrementalDataChanged(newContent);
    }
}
