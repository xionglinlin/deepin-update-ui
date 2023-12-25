// SPDX-FileCopyrightText: 2016 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatelogdialog.h"

#include <QApplication>
#include <QDebug>
#include <QVBoxLayout>
#include <QLabel>

#include <DHiDPIHelper>
#include <DPlainTextEdit>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DBlurEffectWidget>

UpdateLogDialog::UpdateLogDialog(const QString& log, QWidget *parent)
    : DDialog(parent)
{
    setFixedSize(370,306);
    setOnButtonClickedClose(false);

    const auto iconSize = QSize(40 * qApp->devicePixelRatio(), 40 * qApp->devicePixelRatio());
    const auto iconPath = ":/update/themes/common/icons/dcc_update.svg";
    setIcon(QIcon(DHiDPIHelper::loadNxPixmap(iconPath).scaled(iconSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)));

    setTitle(tr("Logs"));

    auto textEdit = new DPlainTextEdit(this);
    textEdit->setReadOnly(true);
    QPalette pl = textEdit->palette();
    pl.setBrush(QPalette::Base, QBrush(QColor(255,255,255,0)));
    textEdit->setPalette(pl);
    textEdit->setFrameShape(QFrame::NoFrame);
    textEdit->setPlainText(log);

    auto blurEffect = new DBlurEffectWidget;
    blurEffect->setAccessibleName("blurEffect");
    blurEffect->setMaskAlpha(128);
    blurEffect->setBlurRectXRadius(8);
    blurEffect->setBlurRectYRadius(8);
    blurEffect->setBlendMode(DBlurEffectWidget::InWidgetBlend);
    auto blurEffectLayout = new QVBoxLayout(blurEffect);
    blurEffectLayout->setMargin(10);
    blurEffectLayout->addWidget(textEdit, 1);

    auto *contentWidget = new QWidget(this);
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(0);
    contentLayout->setMargin(0);
    contentLayout->addSpacing(10);
    contentLayout->addWidget(blurEffect, 1);
    insertContent(0, contentWidget);
}
