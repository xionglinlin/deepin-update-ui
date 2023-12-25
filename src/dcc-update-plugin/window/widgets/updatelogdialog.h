// SPDX-FileCopyrightText: 2016 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef REMINDERDDIALOG_H
#define REMINDERDDIALOG_H

#include <QTimer>

#include <DDialog>

DWIDGET_USE_NAMESPACE

class UpdateLogDialog : public DDialog
{
    Q_OBJECT
public:
    explicit UpdateLogDialog(const QString& log, QWidget *parent = nullptr);
    ~UpdateLogDialog() {}
};

#endif // REMINDERDDIALOG_H
