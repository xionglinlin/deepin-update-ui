// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "unknownupdateitem.h"

#include <DFontSizeManager>

using namespace dcc;
using namespace dcc::widgets;
using namespace dcc::update;


UnknownUpdateItem::UnknownUpdateItem(QWidget *parent)
    : UpdateSettingItem(UpdateType::UnknownUpdate, parent)
{
    init();
}

void UnknownUpdateItem::init()
{
    setIcon(":/update/updatev20/dcc_unknown_update.svg");
    m_detailLabel->setVisible(false);
    m_showMoreButton->setEnabled(false);
    m_showMoreButton->setVisible(false);
    m_dateLabel->setVisible(false);
    m_versionLabel->setEnabled(false);
    auto pal = m_versionLabel->palette();
    QColor base_color = pal.text().color();
    base_color.setAlpha(255 / 10 * 6);
    pal.setColor(QPalette::Text, base_color);
    m_versionLabel->setPalette(pal);
    DFontSizeManager::instance()->bind(m_versionLabel, DFontSizeManager::T8);
}
