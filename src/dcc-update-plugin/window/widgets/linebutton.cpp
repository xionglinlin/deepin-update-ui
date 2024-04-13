// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "linebutton.h"

#include <QMouseEvent>
#include <QHBoxLayout>

#include <DPalette>
#include <DApplicationHelper>
#include <DFontSizeManager>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

LineButton::LineButton(const QString &leftText, const QPixmap &rightPixmap, QWidget *parent)
    : dcc::widgets::SettingsItem(parent)
    , m_enabled(true)
    , m_leftLabel(new QLabel(this))
    , m_rightLabel(new DTipLabel("", this))
{
    addBackground();
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->setSpacing(0);
    hlayout->setContentsMargins(10, 0, 10, 0);
    m_leftLabel->setText(leftText);
    hlayout->addWidget(m_leftLabel);
    hlayout->addStretch();
    m_rightLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_rightLabel->setPixmap(rightPixmap);
    DFontSizeManager::instance()->bind(m_rightLabel, DFontSizeManager::T8);
    hlayout->addWidget(m_rightLabel);
    setLayout(hlayout);
    setFixedHeight(36);
}

void LineButton::setLeftLabelText(const QString &text)
{
    m_leftLabel->setText(text);
}

void LineButton::setRightText(const QString &text)
{
    m_rightLabel->setText(text);
}
void LineButton::setRightPixmap(const QPixmap &pixmap)
{
    m_rightLabel->setPixmap(pixmap);
}

void LineButton::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void LineButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const DPalette &dp = DApplicationHelper::instance()->palette(this);
    QPainter p(this);
    p.setPen(Qt::NoPen);
    p.setBrush(dp.brush(underMouse() ? DPalette::ObviousBackground : DPalette::ItemBackground));
    p.drawRoundedRect(rect(), 8, 8);
}

bool LineButton::event(QEvent *event)
{
    if (m_enabled) {
        QMouseEvent *e = static_cast<QMouseEvent *>(event);
        if (e->type() == QEvent::MouseButtonPress && e->button() == Qt::LeftButton) {
            Q_EMIT clicked();
        } else if (e->type() == QEvent::Enter || e->type() == QEvent::Leave) {
            update();
        }
    }
    return QFrame::event(event);
}