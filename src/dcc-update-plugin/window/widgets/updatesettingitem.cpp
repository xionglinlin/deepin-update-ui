// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatesettingitem.h"
#include "signalbridge.h"
#include "widgets/basiclistdelegate.h"
#include "window/utils.h"

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QIcon>
#include <QScrollArea>
#include <QVBoxLayout>
#include <qpushbutton.h>
#include <QMouseEvent>

#include <DFontSizeManager>

using namespace dcc;
using namespace dcc::widgets;
using namespace dcc::update;
DWIDGET_USE_NAMESPACE

const static QList<UpdatesStatus> DisabledStatuses = { DownloadWaiting, Downloading, DownloadPaused, UpgradeWaiting, UpgradeReady, BackingUp, BackupSuccess, Upgrading };

UpdateSettingItem::UpdateSettingItem(UpdateType updateType, QWidget* parent)
    : SettingsItem(parent)
    , m_updateType(updateType)
    , m_icon(new SmallLabel(this))
    , m_titleLabel(new DLabel(this))
    , m_checkBox(new IndicatorCheckBox(this))
    , m_versionLabel(new DLabel(this))
    , m_detailLabel(new DTipLabel("", this))
    , m_dateLabel(new DLabel(this))
    , m_showMoreButton(new DCommandLinkButton("", this))
    , m_infoWidget(new QWidget(this))
    , m_settingsGroup(new dcc::widgets::SettingsGroup(this, SettingsGroup::BackgroundStyle::NoneBackground))
    , m_updateItemInfo(nullptr)
    , m_allowSettingEnabled(true)
{
    initUi();
    initConnect();
}

void UpdateSettingItem::initUi()
{
    m_icon->setFixedSize(48, 48);
    m_icon->setVisible(false);
    QWidget* widget = new QWidget();
    QVBoxLayout* vboxLay = new QVBoxLayout(widget);
    vboxLay->addWidget(m_icon);
    vboxLay->setContentsMargins(10, 6, 10, 10);
    widget->setLayout(vboxLay);

    QVBoxLayout* titleLay = new QVBoxLayout();
    titleLay->setMargin(0);
    m_titleLabel->setForegroundRole(DPalette::TextTitle);
    m_titleLabel->setWordWrap(true);
    DFontSizeManager::instance()->bind(m_titleLabel, DFontSizeManager::T6, QFont::DemiBold);
    titleLay->addWidget(m_titleLabel, 0, Qt::AlignTop);

    DFontSizeManager::instance()->bind(m_versionLabel, DFontSizeManager::T8);
    m_versionLabel->setForegroundRole(DPalette::TextTitle);
    m_versionLabel->setObjectName("versionLabel");

    m_checkBox->setFixedSize(16, 16);

    titleLay->addWidget(m_versionLabel);
    titleLay->addStretch();
    QHBoxLayout* hLay = new QHBoxLayout();
    hLay->addLayout(titleLay);
    hLay->addStretch(1);
    hLay->addWidget(m_checkBox);

    DFontSizeManager::instance()->bind(m_detailLabel, DFontSizeManager::T8);
    m_detailLabel->setForegroundRole(DPalette::TextTips);
    m_detailLabel->adjustSize();
    m_detailLabel->setTextFormat(Qt::RichText);
    m_detailLabel->setAlignment(Qt::AlignJustify | Qt::AlignLeft);
    m_detailLabel->setWordWrap(true);
    m_detailLabel->setOpenExternalLinks(true);

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

    m_showMoreButton->setText(tr("View More"));
    DFontSizeManager::instance()->bind(m_showMoreButton, DFontSizeManager::T8);
    m_showMoreButton->setForegroundRole(DPalette::Button);
    dateLay->addStretch();
    dateLay->addWidget(m_showMoreButton, 0, Qt::AlignTop);
    dateLay->setContentsMargins(0, 0, 8, 0);

    QVBoxLayout* updateInfoLayout = new QVBoxLayout();
    updateInfoLayout->setSpacing(0);
    updateInfoLayout->addLayout(hLay);
    updateInfoLayout->addWidget(m_detailLabel);
    m_detailLabel->setContentsMargins(0, 5, 0, 0);
    updateInfoLayout->addLayout(dateLay);
    updateInfoLayout->addStretch();
    m_infoWidget->setLayout(updateInfoLayout);

    QHBoxLayout* main = new QHBoxLayout;
    main->setMargin(0);
    main->setSpacing(0);
    main->setContentsMargins(10, 10, 0, 0);
    m_settingsGroup->insertWidget(m_infoWidget);
    m_settingsGroup->setSpacing(0);
    main->addWidget(widget, 0, Qt::AlignTop);
    main->addWidget(m_settingsGroup, 0, Qt::AlignTop);
    setLayout(main);
}

void UpdateSettingItem::setIconVisible(bool show)
{
    m_icon->setVisible(show);
}

void UpdateSettingItem::setIcon(QString path)
{
    const qreal ratio = devicePixelRatioF();
    QPixmap pix = loadPixmap(path).scaled(m_icon->size() * ratio,
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation);
    m_icon->setPixmap(pix);
}

void UpdateSettingItem::showMore()
{
    return;
}

UpdateType UpdateSettingItem::updateType() const
{
    return m_updateType;
}

void UpdateSettingItem::setData(UpdateItemInfo* updateItemInfo)
{
    if (m_updateItemInfo == updateItemInfo) {
        return;
    }

    if (updateItemInfo == nullptr) {
        setVisible(false);
        return;
    }

    m_updateItemInfo = updateItemInfo;

    connect(m_updateItemInfo, &UpdateItemInfo::updateStatusChanged, this, &UpdateSettingItem::setCheckBoxState);

    QString value = updateItemInfo->updateTime().isEmpty() ? "" : tr("Release date: ") + updateItemInfo->updateTime();
    m_dateLabel->setVisible(!value.isEmpty());
    m_dateLabel->setText(value);
    const QString& systemVersionType = DCC_NAMESPACE::IsServerSystem ? tr("Server") : tr("Desktop");
    QString version;
    if (!updateItemInfo->availableVersion().isEmpty()) {
        QString avaVersion = updateItemInfo->availableVersion();
        QString tmpVersion = avaVersion;
        if (dccV20::IsProfessionalSystem)
            tmpVersion = avaVersion.replace(avaVersion.length() - 1, 1, '0'); // 替换版本号的最后一位为‘0‘
        version = tr("Version") + ": " + systemVersionType + tmpVersion;
    }
    setVersion(version);
    m_titleLabel->setText(updateItemInfo->name());
    m_detailLabel->setVisible(!updateItemInfo->explain().isEmpty());
    if (!updateItemInfo->explain().isEmpty()) {
        m_detailLabel->setText(updateItemInfo->explain());
    }
    setCheckBoxState(updateItemInfo->updateStatus());
}

void UpdateSettingItem::initConnect()
{
    connect(m_showMoreButton, &DCommandLinkButton::clicked, this, &UpdateSettingItem::showMore);
    connect(m_checkBox, &IndicatorCheckBox::checkStateChanged, this, [this](bool checked) {
        Q_EMIT SignalBridge::ref().requestCheckUpdateModeChanged(m_updateType, checked);
    });
    connect(m_detailLabel, &QLabel::linkHovered, this, [this](const QString& link){
        m_hoveredLink = link;
    });
}

void UpdateSettingItem::setChecked(bool isChecked)
{
    m_checkBox->setChecked(isChecked);
}

void UpdateSettingItem::setVersion(QString version)
{
    m_versionLabel->setVisible(!version.isEmpty());
    if (!version.isEmpty()) {
        m_versionLabel->setText(version);
    }
}

void UpdateSettingItem::setCheckBoxState(UpdatesStatus status)
{
    m_checkBox->setVisible(UpgradeSuccess != status);
    m_checkBox->setEnabled(m_allowSettingEnabled && !DisabledStatuses.contains(status));
}

void UpdateSettingItem::setCheckBoxEnabled(bool enabled)
{
    m_allowSettingEnabled = enabled;
    bool isEnabledStatus = m_updateItemInfo && !DisabledStatuses.contains(m_updateItemInfo->updateStatus());
    m_checkBox->setEnabled(isEnabledStatus && enabled);
}

void UpdateSettingItem::mousePressEvent(QMouseEvent* event)
{
    dcc::widgets::SettingsItem::mousePressEvent(event);

    if (!m_checkBox->isEnabled() || !m_checkBox->isVisible() || event->button() != Qt::LeftButton || !m_hoveredLink.isEmpty()) {
        return;
    }

    m_checkBox->setChecked(!m_checkBox->isChecked());
}