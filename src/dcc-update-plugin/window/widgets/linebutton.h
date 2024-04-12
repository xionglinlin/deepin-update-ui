// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef LINEBUTTON_H
#define LINEBUTTON_H

#include <QObject>
#include <QLabel>

#include <DTipLabel>

#include "widgets/settingsitem.h"

class LineButton : public dcc::widgets::SettingsItem
{
    Q_OBJECT
public:
    explicit LineButton(const QString &leftText = QString(), const QPixmap &rightPixmap = QPixmap(), QWidget *parent = nullptr);
    virtual ~LineButton() {}

    void setLeftLabelText(const QString &text);
    void setRightText(const QString &text);
    void setRightPixmap(const QPixmap &pixmap);
    void setEnabled(bool enabled);

Q_SIGNALS:
    void clicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;

private:
    bool m_enabled;
    QLabel *m_leftLabel;
    QLabel *m_rightLabel;
};


#endif
