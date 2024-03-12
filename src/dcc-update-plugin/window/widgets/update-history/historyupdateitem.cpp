// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "historyupdateitem.h"
#include "window/utils.h"

#include <DFontSizeManager>
#include <DHiDPIHelper>

#include <QGraphicsOpacityEffect>

DWIDGET_USE_NAMESPACE

namespace dcc {
namespace update {

HistoryUpdateItem::HistoryUpdateItem(const HistoryItemInfo &info, QWidget *parent)
    : dcc::widgets::SettingsItem(parent)
    , m_icon(new dcc::widgets::SmallLabel(this))
    , m_titleLabel(new DLabel(this))
    , m_summaryLabel(new DLabel(this))
    , m_dateLabel(new DLabel(this))
    , m_contentLayout(nullptr)
    , m_infoWidget(new QWidget(this))
{
    initUi();
    addItem(info);
}

HistoryUpdateItem::~HistoryUpdateItem()
{

}

void HistoryUpdateItem::initUi()
{
    addBackground();

    m_icon->setFixedSize(48, 48);
    auto iconWidget = new QWidget();
    auto iconLayout = new QVBoxLayout(iconWidget);
    iconLayout->addWidget(m_icon);
    iconLayout->setContentsMargins(10, 6, 10, 10);
    iconWidget->setLayout(iconLayout);

    m_titleLabel->setForegroundRole(DPalette::TextTitle);
    m_titleLabel->setWordWrap(true);
    DFontSizeManager::instance()->bind(m_titleLabel, DFontSizeManager::T6, QFont::DemiBold);
    auto titleWidget = new QWidget(this);
    auto titleLayout = new QHBoxLayout(titleWidget);
    titleLayout->setMargin(0);
    titleLayout->addWidget(m_titleLabel, 0, Qt::AlignLeft);
    titleLayout->addStretch();

    m_summaryLabel->setWordWrap(true);
    m_summaryLabel->setAccessibleName("summary_label");
    m_summaryLabel->setForegroundRole(DPalette::TextTitle);
    DFontSizeManager::instance()->bind(m_summaryLabel, DFontSizeManager::T8);

    QHBoxLayout* dateLay = new QHBoxLayout();
    DFontSizeManager::instance()->bind(m_dateLabel, DFontSizeManager::T8);
    m_dateLabel->setObjectName("dateLabel");
    m_dateLabel->setForegroundRole(DPalette::TextTips);
    QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(this);
    // 设置透明度为0.7
    opacityEffect->setOpacity(0.7);
    m_dateLabel->setGraphicsEffect(opacityEffect);

    dateLay->addWidget(m_dateLabel, 0, Qt::AlignLeft | Qt::AlignTop);
    dateLay->setSpacing(0);

    auto contentWidget = new QWidget(this);
    m_contentLayout = new QVBoxLayout(contentWidget);
    m_contentLayout->setMargin(0);
    m_contentLayout->setSpacing(10);

    auto updateInfoLayout = new QVBoxLayout(m_infoWidget);
    updateInfoLayout->setSpacing(6);
    updateInfoLayout->addWidget(titleWidget);
    updateInfoLayout->addWidget(m_summaryLabel);
    updateInfoLayout->addWidget(contentWidget);
    updateInfoLayout->addSpacing(4);
    updateInfoLayout->addLayout(dateLay);
    updateInfoLayout->addStretch();
    m_infoWidget->setLayout(updateInfoLayout);

    QHBoxLayout* main = new QHBoxLayout;
    main->setMargin(0);
    main->setSpacing(0);
    main->setContentsMargins(10, 10, 0, 0);
    main->addWidget(iconWidget, 0, Qt::AlignTop);
    main->addWidget(m_infoWidget, 0, Qt::AlignTop);
    setLayout(main);
}

void HistoryUpdateItem::addItem(const HistoryItemInfo &info)
{
    m_dateLabel->setText(tr("Installation date: ") + info.upgradeTime);
    info.type == SystemUpdate ? addSystemItem(info) : addSecurityItem(info);
}

void HistoryUpdateItem::addSecurityItem(const HistoryItemInfo &info)
{
    m_icon->setPixmap(DHiDPIHelper::loadNxPixmap(":/update/updatev20/dcc_safe_update.svg"));
    m_titleLabel->setText(tr("Security Updates"));
    m_summaryLabel->setText(info.summary.isEmpty() ? tr("Fixed some known bugs and security vulnerabilities") : info.summary);

    for (const auto &item : info.details) {
        auto w = new QWidget(this);
        auto itemLayout = new QVBoxLayout(w);
        itemLayout->setMargin(0);
        itemLayout->setSpacing(2);

        auto nameLabel = createNameLabel();
        nameLabel->setText(item.name);

        auto vulLevelLabel = new DLabel(this);
        DFontSizeManager::instance()->bind(vulLevelLabel, DFontSizeManager::T8);
        vulLevelLabel->setForegroundRole(DPalette::TextTitle);
        vulLevelLabel->setText(item.vulLevel);
        vulLevelLabel->setAlignment(Qt::AlignJustify | Qt::AlignLeft);

        itemLayout->addWidget(nameLabel, 0, Qt::AlignLeft);
        itemLayout->addWidget(vulLevelLabel, 0, Qt::AlignLeft);

        if (!item.description.isEmpty()) {
            auto detailLabel = createDetailLabel();
            detailLabel->setText(item.description);
            itemLayout->addWidget(detailLabel, 0, Qt::AlignLeft);
        }

        m_contentLayout->addWidget(w, 0, Qt::AlignLeft);
    }
}

void HistoryUpdateItem::addSystemItem(const HistoryItemInfo &info)
{
    m_icon->setPixmap(DHiDPIHelper::loadNxPixmap(":/update/updatev20/dcc_system_update.svg"));
    m_titleLabel->setText(tr("System Updates"));
    const bool existDetails = !info.details.isEmpty();
    m_summaryLabel->setVisible(!existDetails);
    if (!existDetails) {
        m_summaryLabel->setText(tr("Fixed some known bugs and security vulnerabilities"));
    }

    for (const auto &item : info.details) {
        auto w = new QWidget(this);
        auto itemLayout = new QVBoxLayout(w);
        itemLayout->setMargin(0);
        itemLayout->setSpacing(2);

        auto nameLabel = createNameLabel();
        // 根据需求，将最后一个字符替换为0（原因是不想用户看到版本频繁的更新）
        QString name = item.name;
        if (dccV20::IsProfessionalSystem)
            name.replace(item.name.length() - 1, 1, '0');
        nameLabel->setText(tr("Version: ") + (DCC_NAMESPACE::IsServerSystem ? tr("Server") : tr("Desktop")) + name);

        auto detailLabel = createDetailLabel();
        detailLabel->setText(item.description);

        itemLayout->addWidget(nameLabel, 0, Qt::AlignLeft);
        itemLayout->addWidget(detailLabel, 0, Qt::AlignLeft);

        m_contentLayout->addWidget(w);
    }
}

DLabel* HistoryUpdateItem::createNameLabel()
{
    auto nameLabel = new DLabel(this);
    DFontSizeManager::instance()->bind(nameLabel, DFontSizeManager::T8);
    nameLabel->setForegroundRole(DPalette::TextTitle);
    nameLabel->setAlignment(Qt::AlignJustify | Qt::AlignLeft);

    return nameLabel;
}

DLabel* HistoryUpdateItem::createDetailLabel()
{
    auto detailLabel = new DLabel(this);
    DFontSizeManager::instance()->bind(detailLabel, DFontSizeManager::T8);
    detailLabel->setForegroundRole(DPalette::TextTips);
    detailLabel->adjustSize();
    detailLabel->setTextFormat(Qt::RichText);
    detailLabel->setAlignment(Qt::AlignJustify | Qt::AlignLeft);
    detailLabel->setWordWrap(true);
    detailLabel->setOpenExternalLinks(true);
    return detailLabel;
}

}
}