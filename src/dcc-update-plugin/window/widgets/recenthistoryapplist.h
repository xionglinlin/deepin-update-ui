// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once
#include "interface/namespace.h"
#include "widgets/contentwidget.h"

namespace DCC_NAMESPACE {
namespace update {

class RecentHistoryAppList : public dcc::ContentWidget
{
    Q_OBJECT
public:
    explicit RecentHistoryAppList(QWidget *parent = 0);

    void setContentWidget(QWidget *widget);
};

} // namespace update
} // namespace DCC_NAMESPACE
