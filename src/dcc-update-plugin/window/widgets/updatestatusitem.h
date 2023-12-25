// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once
#include "interface/namespace.h"
#include "widgets/settingsitem.h"
#include "widgets/translucentframe.h"
#include "common.h"
#include "updatemodel.h"

#include <DProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace dcc {
namespace widgets {
class NormalLabel;
}
}

namespace DCC_NAMESPACE {
namespace update {

class UpdateStatusItem : public dcc::widgets::TranslucentFrame
{
    Q_OBJECT
public:
    explicit UpdateStatusItem(dcc::update::UpdateModel *model, QFrame *parent = 0);
    void setProgressValue(int value);
    void setProgressBarVisible(bool visible);
    void setMessage(const QString &message);
    void setVersionVisible(bool state);
    void setSystemVersion(const QString &version);
    void setStatus(dcc::update::UpdatesStatus status);

signals:
    void requestCheckUpdate();

protected:
    void paintEvent(QPaintEvent *event) override;
    void showLastCheckingTime();
    void handleUpdateError();

private:
    dcc::update::UpdateModel *m_model;
    dcc::widgets::NormalLabel *m_messageLabel;
    Dtk::Widget::DProgressBar *m_progress;
    QLabel *m_labelImage;
    QLabel *m_titleLabel;
    QWidget *m_checkUpdateButtonWidget;
    QPushButton *m_checkUpdateBtn;
    QLabel *m_lastCheckTimeTip;
    dcc::update::UpdatesStatus m_status;
};

} // namespace update
} // namespace DCC_NAMESPACE
