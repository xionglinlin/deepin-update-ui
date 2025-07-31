// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGWATCHERHELPER_H
#define LOGWATCHERHELPER_H

#include <QObject>
#include <QSocketNotifier>

class UpdateDBusProxy;

class LogWatcherHelper : public QObject
{
    Q_OBJECT

public:
    explicit LogWatcherHelper(UpdateDBusProxy *dbusProxy, QObject *parent = nullptr);
    ~LogWatcherHelper();

    void startWatchFile();
    void stopWatchFile();

private slots:
    void onDataAvailable();

signals:
    void incrementalDataChanged(const QString &incrementaldata);
    void fileReset();

private:
    void setupLogPipe();
    void readAvailableData();
    void startRealtimeLogAfterHistory();
    
    QString m_data;
    UpdateDBusProxy *m_dbusProxy;
    int m_readFd;
    int m_writeFd;
    QSocketNotifier *m_socketNotifier;
};

#endif // LOGWATCHERHELPER_H
