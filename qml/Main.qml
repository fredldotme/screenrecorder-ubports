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

import QtQuick 2.15
import Lomiri.Components 1.3
import Lomiri.Components.Pickers 1.0
import Lomiri.Components.Themes 1.3
import Lomiri.Content 1.1
import QtQuick.Controls 2.5 as QQC
import QtQuick.Layouts 1.3
import QtQuick.Window 2.12
import QtGraphicalEffects 1.12
import QtMultimedia 5.15
import Qt.labs.settings 1.0
import GSettings 1.0
import LomiriScreenRecordController 1.0

MainView {
    id: root
    objectName: "mainView"
    applicationName: "screenrecorder.ubports"
    automaticOrientation: true

    width: units.gu(45)
    height: units.gu(75)

    Screen.orientationUpdateMask: Qt.LandscapeOrientation |
                                  Qt.PortraitOrientation |
                                  Qt.InvertedLandscapeOrientation |
                                  Qt.InvertedPortraitOrientation

    QtObject {
        id: d

        property bool pendingDelayedRecording: false

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
            recordingButton.recording = true;
            d.setAppLifecycleExemption();
            Controller.start(1.0/*resolution.checkedButton.value*/, 60/*fps.checkedButton.value*/, false /*microphoneInput*/);
        }

        function startDelayedRecording() {
            pendingDelayedRecording = true;
            d.setAppLifecycleExemption();
        }

        function cancelDelayedRecording() {
            pendingDelayedRecording = false;
            d.unsetAppLifecycleExemption();
        }

        function stopRecording() {
            recordingButton.recording = false;
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

    Connections {
        target: Qt.application
        onStateChanged: {
            if (target.state == Qt.ApplicationInactive) {
                if (d.pendingDelayedRecording && !recordingButton.recording) {
                    d.startRecording();
                    d.pendingDelayedRecording = false;
                }
            }
        }
    }

    Page {
        id: mainPage
        anchors.fill: parent
        visible: true

        header: PageHeader {
            id: header
            title: i18n.tr("Screen recorder")
            StyleHints {
                foregroundColor: "white"
                backgroundColor: "#213d3d"
                dividerColor: "transparent"
            }
        }

        RadialGradient {
            anchors.fill: parent
            angle: 45
            horizontalOffset: 0 - (parent.width / 3)
            verticalOffset: parent.height - (parent.height / 3)
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#2b6f6f" }
                GradientStop { position: 0.88; color: "#213d3d" }
            }
        }

        ColumnLayout {
            spacing: units.gu(2)

            anchors.centerIn: parent

            width: parent.width - (units.gu(2) * 2)
            height: implicitHeight

            Behavior on height {
                LomiriNumberAnimation {}
            }

            Behavior on y {
                LomiriNumberAnimation {}
            }

            Label {
                text: d.pendingDelayedRecording ?
                          i18n.tr("Tap to cancel recording")
                        : !recordingButton.recording ?
                              i18n.tr("Tap to record the screen") :
                              i18n.tr("Tap to stop recording")
                font.pixelSize: units.gu(3.5)
                wrapMode: Text.WordWrap
                color: "white"
                Layout.fillWidth: true
                horizontalAlignment: Label.AlignHCenter
            }

            Label {
                text: i18n.tr("Recording will start once the app is in the background")
                readonly property bool visibility: d.pendingDelayedRecording
                opacity: visibility ? 1.0 : 0.0
                visible: opacity > 0.0
                height: visibility ? implicitHeight : 0
                font.pixelSize: units.gu(2.5)
                wrapMode: Text.WordWrap
                color: "white"
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter

                Behavior on opacity {
                    LomiriNumberAnimation {}
                }
                Behavior on height {
                    LomiriNumberAnimation {}
                }
            }

            /*
            Label {
                text: i18n.tr("Resolution scale (not working atm):")
                color: "white"
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
                color: "white"
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
*/

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: units.gu(1)

                Rectangle {
                    id: recordingButton
                    color: recordingButtonMouseArea.pressed ? "#77FF0000" : !recording ? "#99FF0000" :  "#FFFF0000"
                    width: units.gu(16)
                    height: width
                    radius: width / 2
                    border.color: "darkred"
                    border.width: units.gu(0.5)
                    Behavior on color { ColorAnimation { duration: 100 } }

                    property bool recording : false

                    QQC.BusyIndicator {
                        id: indicator
                        running: recordingButton.recording || d.pendingDelayedRecording
                        visible: running
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        id: recordingButtonMouseArea

                        anchors.fill: parent
                        onClicked: {
                            if (d.pendingDelayedRecording)
                                d.cancelDelayedRecording()
                            else
                                if (!recordingButton.recording)
                                    d.startDelayedRecording()
                                else
                                    d.stopRecording()
                        }
                    }
                }
            }
        }

        Connections {
            target: Controller

            onFileSaved: {
                cutPage.show(path)
            }
        }
    }

    Page {
        id: cutPage
        anchors.fill: parent
        visible: opacity > 0.0
        opacity: 0.0

        onVisibleChanged: {
            if (!visible) {
                video.stop();
                cutPage.videoPath = "";
                Controller.cleanSpace();
            }
        }

        property string videoPath : ""

        Behavior on opacity {
            LomiriNumberAnimation {}
        }

        function show(path) {
            videoPath = path
            opacity = 1.0;
        }

        function hide() {
            opacity = 0.0;
        }

        header: PageHeader {
            id: cutPageHeader
            title: i18n.tr("Edit video")
            StyleHints {
                foregroundColor: "white"
                backgroundColor: "#213d3d"
                dividerColor: "transparent"
            }
        }

        RadialGradient {
            anchors.fill: parent
            angle: 45
            horizontalOffset: 0 - (parent.width / 3)
            verticalOffset: parent.height - (parent.height / 3)
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#2b6f6f" }
                GradientStop { position: 0.88; color: "#213d3d" }
            }
        }

        ColumnLayout {
            anchors.fill: parent

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: cutPageHeader.height + units.gu(3)
                Layout.leftMargin: units.gu(3)
                Layout.rightMargin: units.gu(3)
                Layout.fillWidth: true
                Layout.preferredHeight: {
                    if (root.width > root.height)
                        return (width * (root.width / root.height)) -
                                cutPageHeader.height - units.gu(6)
                    else
                        return (width * (root.height / root.width)) -
                                cutPageHeader.height - units.gu(6)
                }
                radius: units.gu(1)
                color: "black"

                Video {
                    id: video
                    anchors.fill: parent
                    anchors.margins: units.gu(1)
                    source: cutPage.videoPath !== "" ?
                                "file://" + cutPage.videoPath :
                                ""
                    autoLoad: true
                    autoPlay: true
                    loops: MediaPlayer.Infinite
                    notifyInterval: 1

                    onSourceChanged: {
                        if (cutPage.videoPath === "")
                            return;

                        videoRange.from = 0
                        videoRange.first.value = 0
                    }

                    onDurationChanged: {
                        if (cutPage.videoPath === "")
                            return

                        if (duration <= 0)
                            return

                        videoRange.to = video.duration
                        videoRange.second.value = video.duration
                    }

                    onPositionChanged: {
                        if (videoRange.second.value - 250 < position) {
                            console.log("Triggering seek to: " + videoRange.first.value)
                            video.seek(videoRange.first.value)
                        }
                    }

                    onPlaybackStateChanged: {
                        console.log("PlaybackState: " + playbackState)
                    }
                }
            }

            QQC.RangeSlider {
                id: videoRange
                Layout.alignment: Qt.AlignHCenter
                Layout.leftMargin: units.gu(3)
                Layout.rightMargin: units.gu(3)
                Layout.fillWidth: true

                stepSize: 1000.0
                snapMode: QQC.RangeSlider.SnapAlways

                first.onMoved: {
                    if (cutPage.videoPath === "")
                        return

                    video.seek(videoRange.first.value)
                }

                second.onMoved: {
                    if (cutPage.videoPath === "")
                        return

                    console.log("Set end of playback to: " + videoRange.second.value)
                    video.seek(Math.max(videoRange.first.value,
                                        videoRange.second.value - 2000))
                }
            }

            Component.onCompleted: console.log("Ratio: " + (root.width / root.height))

            RowLayout {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                Layout.bottomMargin: units.gu(2)
                spacing: units.gu(1)

                Button {
                    color: theme.palette.normal.negative
                    text: i18n.tr("Delete")
                    onClicked: {
                        cutPage.hide()
                    }
                }
                Button {
                    color: theme.palette.normal.positive
                    text: i18n.tr("Save")
                    onClicked: {
                        if (videoRange.first.value > 0 ||
                                videoRange.second.value < video.duration - 500) {
                            console.log("Saving cut video")
                            Controller.cutVideo(cutPage.videoPath,
                                                videoRange.first.value,
                                                videoRange.second.value)
                            return;
                        }

                        console.log("Saving uncut video")
                        picker.targetUrl = "file://" + cutPage.videoPath
                        picker.visible = true
                    }
                }

                Connections {
                    target: Controller
                    function onEditedFileSaved(file) {
                        picker.targetUrl = "file://" + file
                        picker.visible = true
                    }
                }
            }

            QQC.BusyIndicator {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                running: Controller.editing
                visible: running
            }
        }
    }

    ContentPeerPicker {
        id: picker
        anchors.fill: parent
        visible: false
        showTitle: true
        contentType: ContentType.Videos
        handler: ContentHandler.Destination

        property var activeTransfer : null
        property string targetUrl : ""

        ContentItem {
            id: contentItem
        }

        onCancelPressed: {
            picker.visible = false
        }

        onPeerSelected: {
            peer.selectionType = ContentTransfer.Single
            picker.activeTransfer = peer.request()
            picker.activeTransfer.stateChanged.connect(function() {
                if (picker.activeTransfer.state === ContentTransfer.InProgress) {
                    console.log("In progress");
                    contentItem.url = picker.targetUrl
                    contentItem.text = "recording_" + Date.now() + ".mp4"
                    console.log("Transfering: " + contentItem.url + " " + contentItem.text)
                    picker.activeTransfer.items = new Array
                    picker.activeTransfer.items.push(contentItem)
                    picker.activeTransfer.state = ContentTransfer.Charged;
                }
                if (picker.activeTransfer.state === ContentTransfer.Charged) {
                    console.log("Charged");
                    picker.activeTransfer = null
                }
            })
            picker.visible = false
            cutPage.hide()
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
