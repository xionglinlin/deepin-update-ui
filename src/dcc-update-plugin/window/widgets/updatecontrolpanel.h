// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DOWNLOADCONTROLPANEL_H
#define DOWNLOADCONTROLPANEL_H

#include "common.h"
#include "updatemodel.h"
#include "singlecontentwidget.h"

#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QStackedLayout>
#include <QWidget>

#include <DCommandLinkButton>
#include <DFloatingButton>
#include <DIconButton>
#include <DLabel>
#include <DLineEdit>
#include <DProgressBar>
#include <DSpinner>
#include <DSysInfo>
#include <DTextEdit>
#include <DTipLabel>

namespace dcc {
namespace update {

enum ButtonStatus {
    invalid,
    start,
    pause,
    retry
};

// DCommandLinkButton在sizeHint中加了一些margin,影响布局
class NoMarginDCommandLinkButton : public Dtk::Widget::DCommandLinkButton
{
    Q_OBJECT
public:
    using Dtk::Widget::DCommandLinkButton::DCommandLinkButton;

    QSize sizeHint() const override
    {
        QString text = this->text();
        return fontMetrics().size(0, text);
    }
};

class UpdateControlPanel : public QWidget {
    Q_OBJECT
public:
    explicit UpdateControlPanel(QWidget* parent = nullptr);
    ~UpdateControlPanel();

    void initUi();
    void initConnect();
    void setButtonStatus(const ButtonStatus& value);
    void setModel(UpdateModel* model);
    void setUpdateStatus(UpdatesStatus status);
    void setDownloadSize(qlonglong downloadSize);
    int updateTypes() const;
    void setControlPanelType(ControlPanelType type);
    ControlPanelType controlPanelType() const { return m_controlPanelType; }
    bool getCheckBoxEnabledState() const;

Q_SIGNALS:
    void StartDownload();
    void PauseDownload();
    void requestSetUpdateItemCheckBoxEnabled(bool);

public Q_SLOTS:
    void onCtrlButtonClicked();
    void setProgressValue(double progress);
    void setButtonIcon(ButtonStatus status);
    void refreshDownloadSize();
    void onUpdateStatusChanged(ControlPanelType, UpdatesStatus);
    void onCheckUpdateModeChanged(int checkUpdateMode);
    void onUpgradeProgressChanged(double value);

private:
    void setProgressText(const QString& text, const QString& toolTip = "");
    void updateWidgets();
    QString getElidedText(QWidget* widget, QString data, Qt::TextElideMode mode = Qt::ElideRight, int width = 100, int flags = 0, int line = 0);
    bool allowToSetEnabled() const;
    void updateCurrentWidgetEnabledState();
    void onUpdateButtonClicked();
    void onBatteryStatusChanged(bool isOK);
    void showText(int type, const QString &text);
    void updateBackupProgress();
    QPair<int, QString> getText();
    void updateText();
    void startSpinner();
    void setInstallProgressBeginValue(bool success);

private:
    QLabel *m_successIcon;
    Dtk::Widget::DLabel* m_updateTipsLab;
    Dtk::Widget::DLabel* m_updateSizeLab;
    Dtk::Widget::DLabel* m_progressLabel;
    NoMarginDCommandLinkButton* m_showLogButton;
    SingleContentWidget *m_tipLabelWidget;
    QSpacerItem* m_progressLabelRightSpacer;
    Dtk::Widget::DLabel* m_errorTipsLabel;
    Dtk::Widget::DLabel* m_infoTipsLabel;
    SingleContentWidget* m_controlWidget;
    Dtk::Widget::DIconButton* m_startButton;
    Dtk::Widget::DIconButton* m_stopButton;
    Dtk::Widget::DProgressBar* m_progress;
    QWidget* m_downloadingWidget;
    QPushButton* m_downloadBtn;
    QPushButton* m_retryBtn;
    QPushButton* m_updateBtn;
    QPushButton* m_continueUpgrade;
    QPushButton* m_rebootButton;
    QPushButton* m_reBackupButton;
    Dtk::Widget::DSpinner* m_spinner;
    QWidget* m_spinnerWidget;
    ButtonStatus m_buttonStatus;
    UpdatesStatus m_updateStatus;
    qlonglong m_downloadSize;
    UpdateModel* m_model;
    ControlPanelType m_controlPanelType;
    // 安装和备份共用一个进度条，如果没有备份的话从安装进度从0开始，备份了的话接着备份完成时的进度往后面走
    static int InstallProgressBeginValue;
};

}
}

#endif // DOWNLOADCONTROLPANEL_H
