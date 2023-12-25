// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "upgradehistorydialog.h"
#include "widgets/contentwidget.h"
#include "widgets/translucentframe.h"

#include <QScrollArea>
#include <QScroller>
#include <QFile>

#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DHiDPIHelper>
#include <DWindowManagerHelper>

DWIDGET_USE_NAMESPACE

using namespace dcc::update;

UpgradeHistoryDialog::UpgradeHistoryDialog(QWidget *parent)
    : DDialog(parent)
    , m_contentLayout(nullptr)
    , m_normalHistoryWidget(new QWidget(this))
    , m_noHistoryWidget(new QWidget(this))
{
    initUI();

    QDBusInterface managerInter("com.deepin.lastore",
        "/com/deepin/lastore",
        "com.deepin.lastore.Manager",
        QDBusConnection::systemBus());

    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(managerInter.asyncCall("GetHistoryLogs"), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, watcher] {
        watcher->deleteLater();
        if (!watcher->isError()) {
            QDBusPendingReply<QString> reply = watcher->reply();
            updateLayout(UpdateLogHelper::ref().handleHistoryUpdateLog(reply.value()));
        } else {
            updateLayout(UpdateLogHelper::ref().handleHistoryUpdateLog("{}"));
            qCWarning(DCC_UPDATE) << "Get history upgrade log, error:" << watcher->error().message();
        }
    });
}

UpgradeHistoryDialog::~UpgradeHistoryDialog()
{

}

DLabel *UpgradeHistoryDialog::createLable()
{
    auto titleLabel = new DLabel(tr("Update History"), this);
    titleLabel->setForegroundRole(DPalette::TextTitle);
    DFontSizeManager::instance()->bind(titleLabel, DFontSizeManager::T6, QFont::DemiBold);
    return titleLabel;
}

void UpgradeHistoryDialog::initUI()
{
    setFixedSize(712, 530);
    setContentLayoutContentsMargins(QMargins(0, 0, 0, 0));
    setIcon(QIcon::fromTheme("preferences-system"));
    // DDialog 中默认添加了一个buttonLayout，这个layout的 margin 影响了布局，遍历获取所有的 layout，去掉所有的 margin
    for (const auto &l : findChildren<QHBoxLayout*>()) {
        l->setContentsMargins(0, 0, 0, 0);
    }
    m_normalHistoryWidget->setVisible(false);
    m_noHistoryWidget->setVisible(false);

    auto noHistoryIcon = new QLabel(this);
    noHistoryIcon->setFixedSize(132, 132);
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        noHistoryIcon->setPixmap(DHiDPIHelper::loadNxPixmap(":/update/themes/light/icons/no-update-history.svg"));
    } else {
        noHistoryIcon->setPixmap(DHiDPIHelper::loadNxPixmap(":/update/themes/dark/icons/no-update-history.svg"));
    }

    auto noHistoryLabel = new DLabel(tr("No update history"), this);
    DFontSizeManager::instance()->bind(noHistoryLabel, DFontSizeManager::T6);

    auto contentArea = new QScrollArea;
    contentArea->setAccessibleName("ContentWidget_contentArea");
    contentArea->setWidgetResizable(true);
    contentArea->setFrameStyle(QFrame::NoFrame);
    contentArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    contentArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    contentArea->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
    contentArea->setContentsMargins(0, 0, 0, 0);

    QScroller::grabGesture(contentArea->viewport(), QScroller::LeftMouseButtonGesture);
    QScroller *scroller = QScroller::scroller(contentArea);
    QScrollerProperties sp;
    sp.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    scroller->setScrollerProperties(sp);

    auto historyWidget = new QWidget;
    m_contentLayout = new QVBoxLayout(historyWidget);
    if (!QGuiApplication::platformName().startsWith("wayland", Qt::CaseInsensitive) && Dtk::Gui::DWindowManagerHelper::instance()->hasComposite()) {
        historyWidget->setAttribute(Qt::WA_TranslucentBackground);
    }
    m_contentLayout->setSpacing(10);
    m_contentLayout->setContentsMargins(70, 0, 70 ,0);
    contentArea->setWidget(historyWidget);

    auto noHistoryLayout = new QVBoxLayout(m_noHistoryWidget);
    noHistoryLayout->setSpacing(0);
    noHistoryLayout->setMargin(0);
    noHistoryLayout->addWidget(createLable(), 0, Qt::AlignHCenter);
    noHistoryLayout->addStretch();
    noHistoryLayout->addWidget(noHistoryIcon, 0, Qt::AlignHCenter);
    noHistoryLayout->addSpacing(5);
    noHistoryLayout->addWidget(noHistoryLabel, 0, Qt::AlignHCenter);
    noHistoryLayout->addSpacing(100); // 不居中，偏上
    noHistoryLayout->addStretch();

    auto normaleHistoryLayout = new QVBoxLayout(m_normalHistoryWidget);
    normaleHistoryLayout->setContentsMargins(0, 0, 0, 0);
    normaleHistoryLayout->setSpacing(10);
    normaleHistoryLayout->addWidget(createLable(), 0, Qt::AlignHCenter);
    normaleHistoryLayout->addWidget(contentArea);
}

void UpgradeHistoryDialog::updateLayout(const QList<HistoryItemInfo>& items)
{
    if (items.isEmpty()) {
        m_noHistoryWidget->setVisible(true);
        addContent(m_noHistoryWidget);
        return;
    }

    m_normalHistoryWidget->setVisible(true);
    addContent(m_normalHistoryWidget);
    for (const auto &item : items) {
        m_contentLayout->addWidget(new HistoryUpdateItem(item, this));
    }
    m_contentLayout->addSpacing(20);
}

void UpgradeHistoryDialog::initConnections()
{

}