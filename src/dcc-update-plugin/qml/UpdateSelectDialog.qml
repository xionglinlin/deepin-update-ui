// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.5

import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style as DS
import org.deepin.dcc 1.0

D.DialogWindow {
    id: root
    width: 360
    height: 130
    icon: "preferences-system"
    modality: Qt.WindowModal

    signal silentBtnClicked()
    signal upgradeRebootBtnClicked()
    signal upgradeShutdownBtnClicked()

    ColumnLayout {
        anchors.fill: parent
        Label {
            Layout.alignment: Qt.AlignHCenter
            wrapMode: Text.WordWrap
            text: qsTr("The updates have been already downloaded. What do you want to do?")
        }

        Item {
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 0
            spacing: 10

            D.Button {
                text: qsTr("Silent Installation")
                Layout.fillWidth: true
                onClicked: {
                    root.silentBtnClicked()
                }
            }

            D.Button {
                text: qsTr("Update and Reboot")
                Layout.fillWidth: true
                onClicked: {
                    root.upgradeRebootBtnClicked()
                }
            }

            D.Button {
                text: qsTr("Update and Shut Down")
                textColor: D.Palette {
                    normal {
                        common: D.DTK.makeColor(D.Color.Highlight)
                    }
                    normalDark: normal
                }
                Layout.fillWidth: true
                onClicked: {
                    root.upgradeShutdownBtnClicked()
                }
            }
        }
    }
}
