// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatestatusitem.h"

#include "widgets/labels/normallabel.h"
#include "window/utils.h"

#include <QDebug>
#include <QVBoxLayout>
#include <QIcon>
#include <QPainter>
#include <DSysInfo>
#include <DApplicationHelper>
#include <DFontSizeManager>
#include <DHiDPIHelper>
#include <DPaletteHelper>

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace dcc::widgets;
using namespace dcc::update;
using namespace DCC_NAMESPACE;
using namespace DCC_NAMESPACE::update;

UpdateStatusItem::UpdateStatusItem(UpdateModel *model, QFrame *parent)
    : TranslucentFrame(parent)
    , m_model(model)
    , m_messageLabel(new NormalLabel)
    , m_progress(new DProgressBar(this))
    , m_checkUpdateButtonWidget(new QWidget(this))
    , m_checkUpdateBtn(new QPushButton(parent))
    , m_lastCheckTimeTip(new QLabel(this))
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->setSpacing(10);

    m_progress->setAccessibleName("LoadingItem_progress");
    m_progress->setRange(0, 100);
    m_progress->setFixedWidth(160);
    m_progress->setFixedHeight(12);
    m_progress->setTextVisible(false);

    QVBoxLayout *imgLayout = new QVBoxLayout;
    imgLayout->setAlignment(Qt::AlignCenter);
    m_labelImage = new QLabel;
    m_labelImage->setMinimumSize(128, 128);
    imgLayout->addWidget(m_labelImage, 0, Qt::AlignTop);
    imgLayout->addSpacing(10);

    m_titleLabel = new QLabel;
    DFontSizeManager::instance()->bind(m_titleLabel, DFontSizeManager::T6, QFont::DemiBold);

    m_checkUpdateBtn->setFixedSize(QSize(200, 36));
    QVBoxLayout *checkUpdateButtonLayout = new QVBoxLayout;
    checkUpdateButtonLayout->setSpacing(0);
    checkUpdateButtonLayout->setMargin(0);
    checkUpdateButtonLayout->addSpacing(10);
    checkUpdateButtonLayout->addWidget(m_checkUpdateBtn);
    m_checkUpdateButtonWidget->setLayout(checkUpdateButtonLayout);

    m_lastCheckTimeTip->setAlignment(Qt::AlignCenter);
    m_lastCheckTimeTip->setVisible(false);
    DFontSizeManager::instance()->bind(m_lastCheckTimeTip, DFontSizeManager::T8);

    layout->addStretch();
    layout->setSpacing(5);
    layout->setAlignment(Qt::AlignHCenter);
    layout->addLayout(imgLayout);
    layout->addWidget(m_progress, 0, Qt::AlignHCenter);
    layout->addWidget(m_titleLabel, 0, Qt::AlignHCenter);
    layout->addWidget(m_messageLabel, 0, Qt::AlignHCenter);
    layout->addWidget(m_checkUpdateButtonWidget, 0, Qt::AlignHCenter);
    layout->addWidget(m_lastCheckTimeTip, 0, Qt::AlignHCenter);
    layout->addStretch();

    setLayout(layout);

    connect(m_checkUpdateBtn, &QPushButton::clicked, this, &UpdateStatusItem::requestCheckUpdate);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [this] {
        setStatus(m_status);
    });
}

void UpdateStatusItem::setProgressValue(int value)
{
    m_progress->setValue(value);
}

void UpdateStatusItem::setProgressBarVisible(bool visible)
{
    m_progress->setVisible(visible);
}

void UpdateStatusItem::setMessage(const QString &message)
{
    m_messageLabel->setText(message);
}

void UpdateStatusItem::setVersionVisible(bool state)
{
    m_titleLabel->setVisible(state);
}

void UpdateStatusItem::setSystemVersion(const QString &version)
{
    Q_UNUSED(version);
    QString uVersion = DSysInfo::uosProductTypeName() + " " + DSysInfo::majorVersion();
    if (DSysInfo::uosType() != DSysInfo::UosServer)
        uVersion.append(" " + DSysInfo::uosEditionName());
    m_titleLabel->setText(uVersion);
}

void UpdateStatusItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const DPalette &dp = DPaletteHelper::instance()->palette(this);
    QPainter p(this);
    p.setPen(Qt::NoPen);
    p.setBrush(dp.brush(DPalette::ItemBackground));
    p.drawRoundedRect(rect(), 8, 8);

    return QFrame::paintEvent(event);
}

void UpdateStatusItem::setStatus(UpdatesStatus status)
{
    m_status = status;

    setProgressBarVisible(false);
    m_labelImage->setVisible(false);
    m_lastCheckTimeTip->setVisible(false);
    m_checkUpdateButtonWidget->setVisible(false);
    m_titleLabel->setVisible(false);

    const QString &themeName = DCC_NAMESPACE::getThemeName();

    switch (status) {
        case Default:
        case Updated:
            setMessage(tr("Your system is up to date"));
            m_labelImage->setPixmap(DHiDPIHelper::loadNxPixmap(QString(":/update/themes/%1/icons/up_to_date.svg").arg(themeName)));
            m_checkUpdateButtonWidget->setVisible(true);
            m_checkUpdateBtn->setText(tr("Check for Updates"));
            m_labelImage->setVisible(true);
            showLastCheckingTime();
            break;
        case Checking:
            setProgressBarVisible(true);
            m_labelImage->setVisible(true);
            m_labelImage->setPixmap(DHiDPIHelper::loadNxPixmap(QString(":/update/themes/%1/icons/checking.svg").arg(themeName)));
            setMessage(tr("Checking for updates, please wait..."));
            break;
        case CheckingFailed:
            m_labelImage->setVisible(true);
            m_labelImage->setPixmap(DHiDPIHelper::loadNxPixmap(QString(":/update/themes/%1/icons/update_failed.svg").arg(themeName)));
            handleUpdateError();
            break;
        case AllUpdateModeDisabled:
            m_labelImage->setPixmap(DHiDPIHelper::loadNxPixmap(QString(":/update/themes/%1/icons/nice_service.svg").arg(themeName)));
            setMessage(tr("Turn on the switches under Update Content to get better experiences"));
            m_labelImage->setVisible(true);
            break;
        default:
            break;
    }
}

void UpdateStatusItem::showLastCheckingTime()
{
    m_model->updateCheckUpdateTime();
    QString lastTime = m_model->lastCheckUpdateTime();
    qint64 secs = QDateTime::fromString(lastTime, "yyyy-MM-dd hh:mm:ss").toSecsSinceEpoch();
    m_lastCheckTimeTip->setText(tr("Last check: ") + lastTime);
    m_lastCheckTimeTip->setVisible((secs > 0) ? true : false);
}

void UpdateStatusItem::handleUpdateError()
{
    m_checkUpdateButtonWidget->setVisible(true);
    m_checkUpdateBtn->setText(tr("Check Again"));
    m_titleLabel->setVisible(true);
    m_titleLabel->setText(tr("Failed to check for updates"));
    showLastCheckingTime();
    setMessage(m_model->errorToText(m_model->lastError(dcc::update::CheckingFailed)));
}