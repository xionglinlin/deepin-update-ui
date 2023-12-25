#include "updatemodesettingitem.h"
#include "widgets/settingsgroup.h"
#include "window/dconfigwatcher.h"
#include "window/gsettingwatcher.h"
#include "window/utils.h"

#include <DFontSizeManager>

#include <QVBoxLayout>

using namespace dcc;
using namespace dcc::widgets;
using namespace dcc::update;
using namespace DCC_NAMESPACE::update;
using namespace DCC_NAMESPACE;
DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

UpdateModeSettingItem::UpdateModeSettingItem(UpdateModel* model, QWidget* parent)
    : QWidget { parent }
    , m_model(model)
    , m_title(new DLabel(this))
    , m_systemUpdateSwitch(nullptr)
    , m_secureUpdateSwitch(nullptr)
    , m_unknownUpdateSwitch(nullptr)
{
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_systemUpdateSwitch = new SwitchWidget(tr("System Updates"), this);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_secureUpdateSwitch = new SwitchWidget(tr("Security Updates"), this);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_unknownUpdateSwitch = new SwitchWidget(tr("Third-party Updates"), this);
    initUI();
    initConnections();
}

UpdateModeSettingItem::~UpdateModeSettingItem()
{
    GSettingWatcher::instance()->erase("updateSystemUpdate", m_systemUpdateSwitch);
    GSettingWatcher::instance()->erase("updateSecureUpdate", m_secureUpdateSwitch);
}

void UpdateModeSettingItem::initUI()
{
    TranslucentFrame* contentWidget = new TranslucentFrame(this); // 添加一层半透明框架

    m_systemUpdateSwitch->setChecked(m_model->updateMode() & SystemUpdate);
    m_secureUpdateSwitch->setChecked(m_model->updateMode() & SecurityUpdate);
    m_unknownUpdateSwitch->setChecked(m_model->updateMode() & UnknownUpdate);

    m_title->setText(tr("Update content"));
    m_title->setAlignment(Qt::AlignLeft);
    DFontSizeManager::instance()->bind(m_title, DFontSizeManager::T5, QFont::DemiBold);

    SettingsGroup* updatesGrp = new SettingsGroup(contentWidget, SettingsGroup::GroupBackground);
    updatesGrp->appendItem(m_systemUpdateSwitch);
    updatesGrp->appendItem(m_secureUpdateSwitch);
    updatesGrp->appendItem(m_unknownUpdateSwitch);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setMargin(0);
    contentLayout->addWidget(updatesGrp);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setMargin(0);
    mainLayout->addWidget(m_title);
    mainLayout->addSpacing(8);
    mainLayout->addWidget(contentWidget);

    if (IsCommunitySystem) {
        m_secureUpdateSwitch->hide();
    }
}

void UpdateModeSettingItem::initConnections()
{
    connect(m_systemUpdateSwitch, &SwitchWidget::checkedChanged, this, [this](bool checked) {
        Q_EMIT updateModeEnableStateChanged(SystemUpdate, checked);
    });
    connect(m_secureUpdateSwitch, &SwitchWidget::checkedChanged, this, [this](bool checked) {
        Q_EMIT updateModeEnableStateChanged(SecurityUpdate, checked);
    });
    connect(m_unknownUpdateSwitch, &SwitchWidget::checkedChanged, this, [this](bool checked) {
        Q_EMIT updateModeEnableStateChanged(UnknownUpdate, checked);
    });
    connect(m_model, &UpdateModel::updateModeChanged, this, [this](quint64 updateMode) {
        m_systemUpdateSwitch->setChecked(updateMode & SystemUpdate);
        m_secureUpdateSwitch->setChecked(updateMode & SecurityUpdate);
        m_unknownUpdateSwitch->setChecked(updateMode & UnknownUpdate);
    });

    connect(m_model, &UpdateModel::updateStatusChanged, this, &UpdateModeSettingItem::updateWidgetEnableState);
    connect(m_model, &UpdateModel::controlTypeChanged, this, &UpdateModeSettingItem::updateWidgetEnableState);

    GSettingWatcher::instance()->bind("updateSystemUpdate", m_systemUpdateSwitch);
    GSettingWatcher::instance()->bind("updateSecureUpdate", m_secureUpdateSwitch);
    // FIXME 修改DConfigWatcher::bind方式的时候较大概率崩溃，故自己实现了绑定的功能
    DConfig* dConfig = DConfigWatcher::instance()->getModulesConfig(DConfigWatcher::update);
    auto setWidgetState = [](QWidget* w, const QString& setting) {
        if ("Enabled" == setting)
            w->setEnabled(true);
        else if ("Disabled" == setting)
            w->setEnabled(false);

        w->setVisible("Hidden" != setting);
    };
    connect(dConfig, &DConfig::valueChanged, this, [this, dConfig, setWidgetState](const QString& key) {
        const QString& setting = dConfig->value(key).toString();
        QWidget* w = nullptr;
        if (key == "updateSafety") {
            w = m_secureUpdateSwitch;
        } else if (key == "updateThirdPartySource" && DCC_NAMESPACE::IsProfessionalSystem) {
            w = m_unknownUpdateSwitch;
        } else {
            return;
        }

        setWidgetState(w, setting);
    });
    setWidgetState(m_secureUpdateSwitch, dConfig->value("updateSafety").toString());
    if (DCC_NAMESPACE::IsProfessionalSystem) {
        setWidgetState(m_unknownUpdateSwitch, dConfig->value("updateThirdPartySource").toString());
    }
    updateWidgetEnableState();
}

void UpdateModeSettingItem::setButtonsEnabled(bool enabled)
{
    m_systemUpdateSwitch->switchButton()->setEnabled(enabled);
    m_secureUpdateSwitch->switchButton()->setEnabled(enabled);
    m_unknownUpdateSwitch->switchButton()->setEnabled(enabled);
}

void UpdateModeSettingItem::updateWidgetEnableState()
{
    const auto statuses = m_model->allUpdateStatus();
    static auto disabledStatuses = { DownloadWaiting, Downloading, DownloadPaused, Upgrading, BackingUp };
    for (auto status : disabledStatuses) {
        if (statuses.contains(status)) {
            setButtonsEnabled(false);
            return;
        }
    }
    setButtonsEnabled(true);
}
