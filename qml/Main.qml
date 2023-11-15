/*
 * Copyright (C) 2023  UBports Foundation
 * Copytight (C) 2023  Maciej Sopyło
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * screenrecorder is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.7
import Lomiri.Components 1.3
import Lomiri.Components.Pickers 1.0
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Window 2.12
import Qt.labs.settings 1.0
import GSettings 1.0
import Controller 1.0

MainView {
    id: root
    objectName: "mainView"
    applicationName: "screenrecorder.ubports"
    automaticOrientation: true

    width: units.gu(45)
    height: units.gu(75)

    QtObject {
        id: d

        function checkAppLifecycleExemption() {
            const appidList = gsettings.lifecycleExemptAppids;
            if (!appidList) {
                return false;
            }
            return appidList.includes(Qt.application.name);
        }

        function setAppLifecycleExemption() {
            if (!d.checkAppLifecycleExemption()) {
                const appidList = gsettings.lifecycleExemptAppids;
                const newList = appidList.slice();
                newList.push(Qt.application.name);
                gsettings.lifecycleExemptAppids = newList;
            }
        }

        function unsetAppLifecycleExemption() {
            if (d.checkAppLifecycleExemption()) {
                const appidList = gsettings.lifecycleExemptAppids;
                const index = appidList.indexOf(Qt.application.name);
                const newList = appidList.slice();
                if (index > -1) {
                    newList.splice(index, 1);
                }
                gsettings.lifecycleExemptAppids = newList;
            }
        }

        function startRecording() {
            indicator.running = true;
            d.setAppLifecycleExemption();
            Controller.start(Screen.width, Screen.height, 1.0/*resolution.checkedButton.value*/, fps.checkedButton.value);
        }

        function stopRecording() {
            indicator.running = false;
            Controller.stop();
            d.unsetAppLifecycleExemption();
        }
    }

    // cleanup in case it crashes
    Component.onCompleted: d.unsetAppLifecycleExemption()
    Component.onDestruction: d.unsetAppLifecycleExemption()

    GSettings {
        id: gsettings
        schema.id: "com.canonical.qtmir"
    }

    Page {
        anchors.fill: parent

        header: PageHeader {
            id: header
            title: i18n.tr("Screen recorder")
        }

        ColumnLayout {
            spacing: units.gu(2)

            anchors {
                margins: units.gu(2)
                fill: parent
            }

            Item {
                Layout.fillHeight: true
            }

            Label {
                text: i18n.tr("The recording indicator will not show up until you restart your device or Lomiri once.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Label {
                text: i18n.tr("Resolution scale (not working atm):")
                Layout.fillWidth: true
            }

            ButtonGroup {
                id: resolution
                buttons: resolutionRow.children
            }

            Row {
                id: resolutionRow

                RadioButton {
                    property real value: 1.0
                    checked: true
                    enabled: false
                    text: "100%"
                }

                RadioButton {
                    property real value: 0.75
                    enabled: false
                    text: "75%"
                }

                RadioButton {
                    property real value: 0.5
                    enabled: false
                    text: "50%"
                }

                RadioButton {
                    property real value: 0.25
                    enabled: false
                    text: "25%"
                }
            }

            Label {
                text: i18n.tr("Framerate:")
                Layout.fillWidth: true
            }

            ButtonGroup {
                id: fps
                buttons: fpsRow.children
            }

            Row {
                id: fpsRow

                RadioButton {
                    property int value: 30
                    checked: true
                    text: "30"
                }

                RadioButton {
                    property int value: 60
                    text: "60"
                }
            }

            BusyIndicator {
                id: indicator
                running: false
                visible: running
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: units.gu(1)

                Button {
                    Layout.alignment: Qt.AlignHCenter

                    // probably expose recording status from ScreenRecorder?
                    enabled: !indicator.running
                    text: i18n.tr("Start recording")
                    onClicked: d.startRecording()
                }

                Button {
                    Layout.alignment: Qt.AlignHCenter

                    enabled: indicator.running
                    text: i18n.tr("Stop recording")
                    onClicked: d.stopRecording()
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }

    Connections {
        target: UriHandler

        onOpened: {
            if (!uris.length) {
                return;
            }

            // the only thing we want to handle is stopping the recording
            d.stopRecording();
        }
    }
}
