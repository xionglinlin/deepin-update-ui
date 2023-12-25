// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "safeupdateitem.h"

#include <DFontSizeManager>

using namespace dcc;
using namespace dcc::widgets;
using namespace dcc::update;

SecurityUpdateItem::SecurityUpdateItem(QWidget *parent)
    : UpdateSettingItem(SecurityUpdate, parent)
{
    init();
}

void SecurityUpdateItem::init()
{
    setIcon(":/update/updatev20/dcc_safe_update.svg");
    m_showMoreButton->setVisible(false);
    m_versionLabel->setVisible(false);
    DFontSizeManager::instance()->bind(m_detailLabel, DFontSizeManager::T7, QFont::Normal);
    m_detailLabel->setForegroundRole(DPalette::TextTitle);
}

void SecurityUpdateItem::showMore()
{
    m_showMoreButton->setVisible(false);
    for (auto item : m_updateDetailItemList)
        item->setVisible(true);
}

void SecurityUpdateItem::hideMore()
{
    m_showMoreButton->setVisible(true);
    for (auto item : m_updateDetailItemList)
        item->setVisible(false);
}

void SecurityUpdateItem::setData(UpdateItemInfo *updateItemInfo)
{
    UpdateSettingItem::setData(updateItemInfo);

    if (!m_updateDetailItemList.isEmpty()) {
        for (UpdateDetailInfoItem* item : m_updateDetailItemList) {
            m_settingsGroup->removeItem(item);
        }
        qDeleteAll(m_updateDetailItemList);
        m_updateDetailItemList.clear();
    }

    for (const auto& info : updateItemInfo->detailInfos()) {
        const QString currentVersion = info.name;
        UpdateDetailInfoItem* detailInfoItem = new UpdateDetailInfoItem(this);
        // 根据需求，将最后一个字符替换为0（原因是不想用户看到版本频繁的更新）
        QString detail = info.vulLevel;
        if (!info.info.isEmpty()) {
             detail += "\n" + info.info;
        }
        detailInfoItem->setTitle(info.name);
        detailInfoItem->setDetailData(detail);
        detailInfoItem->setLinkData(QString());
        detailInfoItem->setVisible(false);
        detailInfoItem->setUpdateType(SecurityUpdate);
        detailInfoItem->setContentsMargins(5, 15, 20, 10);
        m_updateDetailItemList.append(detailInfoItem);
        m_settingsGroup->appendItem(detailInfoItem);

        connect(detailInfoItem, &UpdateDetailInfoItem::requestHideDetails, this, &SecurityUpdateItem::hideMore);
    }

    if (m_updateDetailItemList.count() > 0)
        m_updateDetailItemList.last()->setIsLastItem(true);

    m_showMoreButton->setVisible(m_updateDetailItemList.count());
}


