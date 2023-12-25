// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <DFontSizeManager>
#include <float.h>

#include "systemupdateitem.h"
#include "window/utils.h"

using namespace dcc;
using namespace dcc::update;

SystemUpdateItem::SystemUpdateItem(QWidget* parent)
    : UpdateSettingItem(SystemUpdate, parent)
    , m_line(new DHorizontalLine(this))
    , m_lineWidget(new QWidget)
{
    setIcon(":/update/updatev20/dcc_system_update.svg");
    QVBoxLayout* lineLay = new QVBoxLayout();
    lineLay->setMargin(0);
    lineLay->addSpacing(10);
    lineLay->addWidget(m_line);
    m_lineWidget->setLayout(lineLay);
    m_settingsGroup->insertWidget(m_lineWidget);
    m_lineWidget->setVisible(false);
}

void SystemUpdateItem::showMore()
{
    m_showMoreButton->setVisible(false);
    m_lineWidget->setVisible(true);
    for (auto item : m_updateDetailItemList)
        item->setVisible(true);
}

void SystemUpdateItem::hideMore()
{
    m_showMoreButton->setVisible(true);
    m_lineWidget->setVisible(false);
    for (auto item : m_updateDetailItemList)
        item->setVisible(false);
}

void SystemUpdateItem::setData(UpdateItemInfo* updateItemInfo)
{
    UpdateSettingItem::setData(updateItemInfo);
    if (updateItemInfo->availableVersion().isEmpty() && updateItemInfo->updateTime().isEmpty()) {
        m_detailLabel->setVisible(false);
        m_showMoreButton->setEnabled(false);
        m_showMoreButton->setVisible(false);
        m_dateLabel->setVisible(false);
        m_versionLabel->setVisible(true);
        m_versionLabel->setText(updateItemInfo->explain());
        m_versionLabel->setContentsMargins(0, 4, 0, 0);
        DFontSizeManager::instance()->bind(m_versionLabel, DFontSizeManager::T8);
        m_versionLabel->setForegroundRole(DPalette::TextTitle);
    }

    QList<DetailInfo> detailInfoList = updateItemInfo->detailInfos();

    if (!m_updateDetailItemList.isEmpty()) {
        for (UpdateDetailInfoItem* item : m_updateDetailItemList) {
            m_settingsGroup->removeItem(item);
        }
        qDeleteAll(m_updateDetailItemList);
        m_updateDetailItemList.clear();
    }

    const QString systemVer = dccV20::IsCommunitySystem ? Dtk::Core::DSysInfo::deepinVersion() : Dtk::Core::DSysInfo::minorVersion();

    for (const auto& info : detailInfoList) {
        const QString currentVersion = info.name;
        if (subVersion(systemVer, currentVersion) > DBL_MIN || subVersion(currentVersion, updateItemInfo->availableVersion()) > DBL_MIN) {
            continue;
        }
        createDetailInfoItem(info);
    }

    if (m_updateDetailItemList.count() > 0)
        m_updateDetailItemList.last()->setIsLastItem(true);

    m_showMoreButton->setVisible(m_updateDetailItemList.count());
}

char SystemUpdateItem::getLastNumForString(const QString& value)
{
    QChar lastNum = QChar::Null;
    for (QChar item : value) {
        if (item.toLatin1() >= '0' && item.toLatin1() <= '9') {
            lastNum = item;
        }
    }

    return lastNum.toLatin1();
}

double SystemUpdateItem::subVersion(const QString& firstVersion, const QString& secondVersion)
{
    vector<double> firstVersionVec = getNumListFromStr(firstVersion);
    vector<double> secondVersionVec = getNumListFromStr(secondVersion);
    if (firstVersionVec.empty() || secondVersionVec.empty()) {
        return -1;
    }

    return firstVersionVec.at(0) - secondVersionVec.at(0);
}

void SystemUpdateItem::createDetailInfoItem(const DetailInfo& info)
{
    const QString& systemVersionType = DCC_NAMESPACE::IsServerSystem ? tr("Server") : tr("Desktop");
    UpdateDetailInfoItem* detailInfoItem = new UpdateDetailInfoItem(this);
    // 根据需求，将最后一个字符替换为0（原因是不想用户看到版本频繁的更新）
    QString name = info.name;
    if (dccV20::IsProfessionalSystem)
        name.replace(info.name.length() - 1, 1, '0');
    detailInfoItem->setTitle(systemVersionType + name);
    detailInfoItem->setDate(info.updateTime);
    detailInfoItem->setLinkData(info.link);
    detailInfoItem->setDetailData(info.info);
    detailInfoItem->setVisible(false);
    detailInfoItem->setContentsMargins(5, 15, 20, 10);
    m_updateDetailItemList.append(detailInfoItem);
    m_settingsGroup->appendItem(detailInfoItem);

    connect(detailInfoItem, &UpdateDetailInfoItem::requestHideDetails, this, &SystemUpdateItem::hideMore);
}
