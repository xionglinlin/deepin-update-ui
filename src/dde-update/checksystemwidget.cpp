// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "checksystemwidget.h"
#include "updateworker.h"

#include <DFontSizeManager>
#include <DHiDPIHelper>
#include <DPaletteHelper>
#include <DGuiApplicationHelper>

#include <QEvent>
#include <QApplication>
#include <QKeyEvent>

DWIDGET_USE_NAMESPACE

CheckProgressWidget::CheckProgressWidget(QWidget *parent)
    : QWidget(parent)
    , m_logo(nullptr)
    , m_tip(new QLabel(this))
    , m_waitingView(new DPictureSequenceView(this))
    , m_progressBar(nullptr)
    , m_waterProgress(nullptr)
    , m_progressText(nullptr)
{
    auto palette = m_tip->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    m_tip->setPalette(palette);
    DFontSizeManager::instance()->bind(m_tip, DFontSizeManager::T6);

    m_waitingView->setAccessibleName("WaitingUpdateSequenceView");
    m_waitingView->setFixedSize(20 * qApp->devicePixelRatio(), 20 * qApp->devicePixelRatio());
    m_waitingView->setSingleShot(false);
    QStringList pics;
    for (int i = 0; i < 40; ++i)
        pics << QString(":img/waiting_update/waiting_update_%1.png").arg(QString::number(i));
    m_waitingView->setPictureSequence(pics, true);

    QHBoxLayout *tipsLayout = new QHBoxLayout;
    tipsLayout->setSpacing(0);
    tipsLayout->addStretch();
    tipsLayout->addWidget(m_tip);
    tipsLayout->addSpacing(5);
    tipsLayout->addWidget(m_waitingView);
    tipsLayout->addStretch();

    if (UpdateModel::CSS_BeforeLogin == UpdateModel::instance()->checkSystemStage()) {
        m_logo = new QLabel(this);
        m_logo->setFixedSize(286, 57);
        m_logo->setPixmap(DHiDPIHelper::loadNxPixmap(":img/logo.svg"));

        m_tip->setText(tr("Checking in progress, almost completed. Thank you for your patience"));

        m_progressBar = new DProgressBar(this);
        m_progressBar->setFixedWidth(500);
        m_progressBar->setFixedHeight(8);
        m_progressBar->setRange(0, 100);
        m_progressBar->setAlignment(Qt::AlignRight);
        m_progressBar->setAccessibleName("ProgressBar");
        m_progressBar->setValue(1);
        DPaletteHelper::instance()->setPalette(m_progressBar, DGuiApplicationHelper::instance()->standardPalette(DGuiApplicationHelper::DarkType));

        m_progressText = new QLabel(this);
        m_progressText->setText("1%");
        DFontSizeManager::instance()->bind(m_progressText, DFontSizeManager::T6);
    } else {
        m_tip->setText(tr("Preparing"));

        m_waterProgress = new DWaterProgress(this);
        m_waterProgress->setFixedSize(98, 98);
        m_waterProgress->setValue(1);
    }

    QHBoxLayout *pProgressLayout = new QHBoxLayout;
    pProgressLayout->addStretch();
    if (m_progressBar)
        pProgressLayout->addWidget(m_progressBar, 0, Qt::AlignCenter);
    else
        pProgressLayout->addWidget(m_waterProgress, 0, Qt::AlignCenter);

    if (m_progressText) {
        pProgressLayout->addSpacing(10);
        pProgressLayout->addWidget(m_progressText, 0, Qt::AlignCenter);
    }
    pProgressLayout->addStretch();

    QVBoxLayout *pLayout = new QVBoxLayout(this);

    pLayout->addStretch(2);
    if (m_logo) {
        pLayout->addWidget(m_logo, 0, Qt::AlignCenter);
        pLayout->addSpacing(100);
    }
    pLayout->addLayout(pProgressLayout, 0);
    pLayout->addSpacing(m_progressBar ? 10 : 15);
    pLayout->addLayout(tipsLayout, 0);
    pLayout->addStretch(3);
}

bool CheckProgressWidget::event(QEvent *e)
{
    if (e->type() == QEvent::Show)
        m_waitingView->play();
    else if (e->type() == QEvent::Hide)
        m_waitingView->stop();

    return false;
}

void CheckProgressWidget::setValue(double value)
{
    int iProgress = static_cast<int>(value * 100);
    // 进度条不能大于100，不能小于0，不能回退
    if (iProgress > 100 || iProgress < 0 || iProgress <= (m_progressBar ? m_progressBar->value() : m_waterProgress->value()))
        return;

    qInfo() << "Check system progress value: " << value;
    m_progressBar ?  m_progressBar->setValue(iProgress) : m_waterProgress->setValue(iProgress);
    if (m_progressText)
        m_progressText->setText(QString::number(iProgress) + "%");
}

CheckResultWidget::CheckResultWidget(QWidget *parent)
    : QWidget(parent)
    , m_iconLabel(new QLabel(this))
    , m_title(new QLabel(this))
    , m_tips(new QLabel(this))
    , m_titleSpacer(new QSpacerItem(0, 0))
    , m_mainLayout(nullptr)
    , m_buttonSpacer(new QSpacerItem(0, 80))
{
    QPalette palette = this->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    setPalette(palette);

    m_iconLabel->setFixedSize(128, 128);

    m_title->setAlignment(Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(m_title, DFontSizeManager::T4);

    m_tips->setAlignment(Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(m_tips, DFontSizeManager::T6);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setMargin(0);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_iconLabel, 0, Qt::AlignCenter);
    m_mainLayout->addSpacerItem(m_titleSpacer);
    m_mainLayout->addWidget(m_title, 0, Qt::AlignCenter);
    m_mainLayout->addWidget(m_tips,0 , Qt::AlignCenter);
    m_mainLayout->addItem(m_buttonSpacer);
}

void CheckResultWidget::showResult(bool success)
{
    if (success) {
        qInfo() << "Check system completed, result: " << success;
        showSuccessFrame();
    } else {
        showErrorFrame();
    }
}

QString CheckResultWidget::systemVersion()
{
    QSettings lsbSetting("/etc/os-version", QSettings::IniFormat);
    lsbSetting.setIniCodec("utf-8");
    lsbSetting.beginGroup("Version");
    QLocale locale;

    if (locale.language() == QLocale::Chinese)
        return lsbSetting.value(QString("EditionName[%1]").arg(locale.name()), "").toString() +
            lsbSetting.value("MinorVersion").toString();

    return lsbSetting.value(QString("EditionName"), "").toString() +
        lsbSetting.value("MinorVersion").toString();
}

void CheckResultWidget::showSuccessFrame()
{
    qDeleteAll(m_actionButtons);
    m_actionButtons.clear();
    m_buttonSpacer->changeSize(0, 0);
    m_titleSpacer->changeSize(0, 30);
    m_iconLabel->setFixedSize(286, 57);
    m_iconLabel->setPixmap(DHiDPIHelper::loadNxPixmap(":img/logo.svg"));
    m_title->setText(systemVersion());
    createButtons({UpdateModel::EnterDesktop});
    if (!m_actionButtons.isEmpty())
        m_buttonSpacer->changeSize(0, 80);

    m_mainLayout->invalidate();
}

void CheckResultWidget::showErrorFrame()
{
    qInfo() << "Check system failed, show error frame";

    m_buttonSpacer->changeSize(0, 0);
    m_titleSpacer->changeSize(0, 0);
    m_iconLabel->setPixmap(DHiDPIHelper::loadNxPixmap(":img/failed.svg"));
    const QList<UpdateModel::UpdateAction> actions = {{UpdateModel::Reboot, UpdateModel::EnterDesktop}};
    m_title->setText(tr("There are some issues with the system."));

    createButtons(actions);
    if (!m_actionButtons.isEmpty())
        m_buttonSpacer->changeSize(0, 80);

    m_mainLayout->invalidate();
}

void CheckResultWidget::createButtons(const QList<UpdateModel::UpdateAction> &actions)
{
    qDeleteAll(m_actionButtons);
    m_actionButtons.clear();

    m_checkedButton = nullptr;
    for (auto action : actions) {
        auto button = new QPushButton(UpdateModel::updateActionText(action), this);
        button->setFixedSize(240, 48);
        m_mainLayout->addWidget(button, 0, Qt::AlignHCenter);
        m_actionButtons.append(button);
        button->setFocusPolicy(Qt::NoFocus);
        button->setCheckable(true);
        // 按钮的选中状态跟随用户最初选择的是关机还是重启
        if ((action == UpdateModel::Reboot && UpdateModel::instance()->isReboot())
            || (action == UpdateModel::ShutDown && !UpdateModel::instance()->isReboot())) {
            button->setChecked(true);
            m_checkedButton = button;
        }

        connect(button, &QPushButton::clicked, this, [action] {
            UpdateWorker::instance()->doAction(action);
        });
    }

    // 非重启/关机按钮的情况，默认选中第一个按钮
    if (!m_checkedButton && !m_actionButtons.isEmpty()) {
        m_checkedButton = m_actionButtons.first();
        m_checkedButton->setChecked(true);
    }
}

void CheckResultWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Tab: {
        if (m_actionButtons.size() > 1) {
            int index = 0;
            if (m_checkedButton) {
                index = m_actionButtons.indexOf(m_checkedButton.data());
                if (index == m_actionButtons.length() - 1)
                    index = 0;
                else
                    index++;

                m_checkedButton->setChecked(false);
            }
            m_checkedButton = m_actionButtons.at(index);
            m_checkedButton->setChecked(true);
        }

        break;
    }
    case Qt::Key_Return:
        if(m_checkedButton)
            m_checkedButton->clicked();
        break;
    case Qt::Key_Enter:
        if (m_checkedButton)
            m_checkedButton->clicked();
        break;
    }

    QWidget::keyPressEvent(event);
}

CheckSystemWidget::CheckSystemWidget(QWidget *parent)
    : QWidget(parent)
    , m_checkProgressWidget(new CheckProgressWidget(this))
    , m_checkResultWidget(new CheckResultWidget(this))
{
    initUI();
    initConnections();
}

CheckSystemWidget::~CheckSystemWidget()
{

}

CheckSystemWidget* CheckSystemWidget::instance()
{
    static CheckSystemWidget* checkSystemWidget = nullptr;
    if (!checkSystemWidget) {
        checkSystemWidget = new CheckSystemWidget(nullptr);
    }
    return checkSystemWidget;
}

void CheckSystemWidget::initUI()
{
    auto mainLayout = new  QVBoxLayout(this);
    mainLayout->addWidget(m_checkProgressWidget);
    mainLayout->addWidget(m_checkResultWidget);

    m_checkProgressWidget->setVisible(true);
    m_checkResultWidget->setVisible(false);
}

void CheckSystemWidget::initConnections()
{
    connect(UpdateModel::instance(), &UpdateModel::checkStatusChanged, this, [this](UpdateModel::CheckStatus status) {
        if (UpdateModel::CheckSuccess == status || UpdateModel::CheckFailed == status) {
            m_checkResultWidget->showResult(UpdateModel::CheckSuccess == status);
            m_checkResultWidget->setVisible(true);
            m_checkProgressWidget->setVisible(false);
        }
    });

    connect(UpdateModel::instance(), &UpdateModel::JobProgressChanged, m_checkProgressWidget, &CheckProgressWidget::setValue);

    m_checkProgressWidget->setValue(UpdateModel::instance()->jobProgress());
}
