// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "indicatorcheckbox.h"

#include <DStyle>
#include <QPainter>

#include <math.h>

IndicatorCheckBox::IndicatorCheckBox(QWidget *parent)
    : QWidget(parent)
    , m_isChecked(false)
{

}

void IndicatorCheckBox::setChecked(bool checked)
{
    if (m_isChecked == checked) {
        return;
    }
    m_isChecked = checked;
    Q_EMIT checkStateChanged(checked);
    update();
}

void drawMarkElement(QPainter *pa, const QRectF &rect)
{
    pa->drawLine(rect.x(), rect.y() + rect.height() * 0.6, rect.x() + rect.width() * 0.4, rect.bottom());
    pa->drawLine(rect.x() + rect.width() * 0.4, rect.bottom(), rect.right(), rect.y());
}

void drawMark(QPainter *pa, const QRectF &rect, const QColor &boxInside, const QColor &boxOutside, const qreal penWidth, const int outLineLeng)
{
    QPen pen(boxInside);
    pen.setWidthF(penWidth);
    pa->setPen(pen);
    pen.setJoinStyle(Qt::RoundJoin);

    drawMarkElement(pa, rect);

    if (outLineLeng == 0)
        return;

    double xWide = (rect.width() / 2.0);
    int yHigh = rect.height();
    double length = sqrt(pow(xWide, 2) + pow(yHigh, 2));
    double x = rect.right() + (outLineLeng / length) * xWide;
    double y = rect.y() - (outLineLeng / length) * yHigh;

    pen.setColor(boxOutside);
    pa->setPen(pen);
    pa->drawLine(QPointF(rect.topRight()), QPointF(x, y));
}

void drawIndicatorChecked(QPainter *pa, const QRectF &rect)
{
    QRectF mark(0, 0, rect.width() * 0.575, rect.height() * 0.475);
    mark.moveCenter(rect.center());
    mark.setLeft(mark.left() + rect.width() * 0.09);
    QStyleOption opt;
    QColor brush = opt.palette.color(DPalette::Highlight);
    pa->setPen(Qt::NoPen);
    pa->setBrush(brush);

    pa->drawEllipse(rect);
    drawMark(pa, mark, opt.palette.color(DPalette::Window), opt.palette.color(DPalette::Window), 1.5 , 0);
}

void drawIndicatorUnchecked(QPainter *pa, const QRectF &rect)
{
    QStyleOption opt;
    QPen pen(opt.palette.color(DPalette::Window));
    pen.setWidthF(1);
    pa->drawEllipse(rect);
}

void IndicatorCheckBox::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (!isEnabled()) {
        painter.setOpacity(0.5);
    }

    if (m_isChecked) {
        drawIndicatorChecked(&painter, rect().adjusted(1, 1, -1, -1));
    } else {
        drawIndicatorUnchecked(&painter, rect().adjusted(1, 1, -1, -1));
    }
}
