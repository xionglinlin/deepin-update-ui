// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef INDICATORCHECKBOX_H
#define INDICATORCHECKBOX_H

#include <QWidget>

class IndicatorCheckBox : public QWidget
{
    Q_OBJECT
public:
    explicit IndicatorCheckBox(QWidget *parent = nullptr);

    void setChecked(bool checked);
    bool isChecked() const { return m_isChecked; }

signals:
    void checkStateChanged(bool);

protected:
    void paintEvent(QPaintEvent *e);

private:
    bool m_isChecked;
};

#endif // INDICATORCHECKBOX_H
