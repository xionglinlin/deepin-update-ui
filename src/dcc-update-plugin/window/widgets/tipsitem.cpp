// SPDX-FileCopyrightText: 2011 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "tipsitem.h"
#include "window/utils.h"
#include "widgets/labels/normallabel.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>
#include <dhidpihelper.h>
#include <DGuiApplicationHelper>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

namespace dcc {
namespace update {

TipsItem::TipsItem(QFrame *parent)
    : SettingsItem(parent),
      m_message(new dcc::widgets::NormalLabel),
      m_icon(new QLabel),
      m_pix(""),
      m_status(UpdatesStatus::Default)
{
    m_icon->setFixedSize(128, 128);

    m_message->setWordWrap(true);
    m_message->setAlignment(Qt::AlignCenter);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->setSpacing(0);

    layout->addStretch();
    layout->addWidget(m_icon, 0, Qt::AlignHCenter);
    layout->addSpacing(10);
    layout->addWidget(m_message);
    layout->addSpacing(50);
    layout->addStretch();

    setLayout(layout);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [this] {
        setStatus(m_status);
    });
}

void TipsItem::setStatus(UpdatesStatus type)
{
    const QString &themeName = DCC_NAMESPACE::getThemeName();
    m_status = type;
    switch (type) {
    case UpdatesStatus::SystemIsNotActive:
        m_pix = QString(":/update/themes/%1/icons/noactive.svg").arg(themeName);
        m_icon->setPixmap(DHiDPIHelper::loadNxPixmap(m_pix));
        setMessage(tr("Your system is not activated, and it failed to connect to update services."));
        break;
    case UpdatesStatus::UpdateIsDisabled:
        m_pix = QString(":/update/themes/%1/icons/prohibit.svg").arg(themeName);
        m_icon->setPixmap(DHiDPIHelper::loadNxPixmap(m_pix));
        setMessage(tr("The system updates are disabled. Please contact your administrator for help."));
        break;
    default:
        qCWarning(DCC_UPDATE) << "Unknown status:" << type;
        break;
    }
}

void TipsItem::setMessage(const QString &message)
{
    m_message->setText(message);
}

} // namespace update
} // namespace dcc
