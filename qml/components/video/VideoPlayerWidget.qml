import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import VideoPlayer 1.0
import "../../theme/Theme.js" as Theme

Rectangle {
    id: root
    color: "#0B0D11"
    property string videoIconPrefix: "qrc:/qml/assets/ai/icons/"

    property bool compactLayout: width < 960
    property bool denseLayout: width < 760
    property bool iconOnlyActions: width < 860

    property int pagePadding: width >= 1360 ? 22 : (width >= 960 ? 16 : 12)
    property int titleBarHeight: denseLayout ? 52 : 58
    property int bottomBarHeight: denseLayout ? 88 : 98
    property int primaryButtonSize: denseLayout ? 42 : 46
    property int secondaryButtonSize: denseLayout ? 34 : 36

    property int videoWidth: 0
    property int videoHeight: 0
    property double videoFPS: 0
    property int videoDuration: 0
    property bool isPlaying: false
    property int currentPosition: 0

    signal playPauseClicked()
    signal stopClicked()
    signal seekRequested(int positionMs)
    signal openVideoClicked()

    function iconSource(name) {
        return videoIconPrefix + name + ".svg"
    }

    function currentMetaText() {
        if (videoWidth <= 0 || videoHeight <= 0) {
            return "?????????"
        }
        var fpsText = videoFPS > 0 ? (videoFPS.toFixed(1) + " FPS") : ""
        var durationText = videoDuration > 0 ? formatTime(videoDuration) : ""
        var parts = [videoWidth + " ? " + videoHeight]
        if (fpsText.length > 0)
            parts.push(fpsText)
        if (durationText.length > 0)
            parts.push(durationText)
        return parts.join("  ?  ")
    }

    function updateProgress(progress) {
        var clamped = Math.max(0, Math.min(1.0, Number(progress)))
        progressBar.value = isNaN(clamped) ? 0 : clamped
    }

    function showError(errorMsg) {
        errorText.text = errorMsg || "????"
        errorToast.visible = true
        errorHideTimer.restart()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.titleBarHeight
            color: "#11151B"
            border.width: 1
            border.color: "#202631"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: root.pagePadding
                anchors.rightMargin: root.pagePadding
                spacing: 10

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1

                    Text {
                        text: videoWidth > 0 ? "????" : "?????"
                        font.pixelSize: root.denseLayout ? 14 : 15
                        font.weight: Font.DemiBold
                        color: "#F4F7FB"
                        elide: Text.ElideRight
                    }

                    Text {
                        visible: !root.denseLayout || videoWidth > 0
                        text: root.currentMetaText()
                        font.pixelSize: 11
                        color: "#7E8796"
                        elide: Text.ElideRight
                    }
                }

                Rectangle {
                    width: root.iconOnlyActions ? root.secondaryButtonSize : (root.compactLayout ? 92 : 106)
                    height: root.secondaryButtonSize
                    radius: height / 2
                    color: openVideoArea.pressed ? "#161B22" : (openVideoArea.containsMouse ? "#202733" : "#171C23")
                    border.width: 1
                    border.color: openVideoArea.containsMouse ? "#505C6F" : "#2D3440"

                    Row {
                        anchors.centerIn: parent
                        spacing: root.iconOnlyActions ? 0 : 8

                        Image {
                            width: 16
                            height: 16
                            source: root.iconSource("video-open")
                            fillMode: Image.PreserveAspectFit
                        }

                        Text {
                            visible: !root.iconOnlyActions
                            anchors.verticalCenter: parent.verticalCenter
                            text: "????"
                            font.pixelSize: 12
                            font.weight: Font.Medium
                            color: "#EAF0F8"
                        }
                    }

                    MouseArea {
                        id: openVideoArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.openVideoClicked()
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#000000"

            VideoFrameItem {
                id: videoFrameItem
                objectName: "videoFrameItem"
                anchors.fill: parent
            }

            Column {
                anchors.centerIn: parent
                spacing: 6
                visible: videoWidth === 0

                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: root.denseLayout ? 28 : 32
                    height: root.denseLayout ? 28 : 32
                    source: root.iconSource("video-play")
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.9
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "?????????"
                    font.pixelSize: root.denseLayout ? 16 : 18
                    font.weight: Font.Medium
                    color: "#EEF2F7"
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "????????????"
                    font.pixelSize: 12
                    color: "#6F7786"
                }
            }

            Rectangle {
                id: errorToast
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 16
                visible: false
                radius: 10
                color: "#DB2F353D"
                border.width: 1
                border.color: "#F46A6A"
                width: Math.min(parent.width - 32, errorText.implicitWidth + 24)
                height: 38

                Text {
                    id: errorText
                    anchors.centerIn: parent
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    color: "#FFFFFF"
                    text: ""
                    elide: Text.ElideRight
                }
            }

            BusyIndicator {
                id: loadingIndicator
                anchors.centerIn: parent
                running: false
                visible: running
            }

            Timer {
                id: errorHideTimer
                interval: 3200
                repeat: false
                onTriggered: errorToast.visible = false
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.bottomBarHeight
            color: "#12161D"
            border.width: 1
            border.color: "#202631"

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: root.pagePadding
                anchors.rightMargin: root.pagePadding
                anchors.topMargin: 10
                anchors.bottomMargin: 10
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Slider {
                        id: progressBar
                        Layout.fillWidth: true
                        from: 0
                        to: 1.0
                        value: 0

                        onPressedChanged: {
                            if (!pressed && videoDuration > 0) {
                                root.seekRequested(Math.floor(value * videoDuration))
                            }
                        }

                        background: Rectangle {
                            x: progressBar.leftPadding
                            y: progressBar.topPadding + progressBar.availableHeight / 2 - height / 2
                            width: progressBar.availableWidth
                            height: 4
                            radius: 2
                            color: "#313948"

                            Rectangle {
                                width: progressBar.visualPosition * parent.width
                                height: parent.height
                                radius: 2
                                color: "#D94747"
                            }
                        }

                        handle: Rectangle {
                            x: progressBar.leftPadding + progressBar.visualPosition * (progressBar.availableWidth - width)
                            y: progressBar.topPadding + progressBar.availableHeight / 2 - height / 2
                            width: 14
                            height: 14
                            radius: 7
                            color: progressBar.pressed ? "#FFFFFF" : "#F6F8FB"
                            border.width: 1
                            border.color: "#D94747"
                        }
                    }

                    Text {
                        text: formatTime(currentPosition) + " / " + formatTime(videoDuration)
                        font.pixelSize: 12
                        color: "#E6EBF3"
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    RowLayout {
                        spacing: 8

                        Rectangle {
                            width: root.primaryButtonSize
                            height: root.primaryButtonSize
                            radius: width / 2
                            color: playPauseArea.pressed ? "#C64040"
                                                          : (playPauseArea.containsMouse ? "#E25555" : "#D94747")
                            border.width: 1
                            border.color: playPauseArea.containsMouse ? "#F28E8E" : "#D84A4A"

                            Image {
                                anchors.centerIn: parent
                                width: root.denseLayout ? 18 : 20
                                height: root.denseLayout ? 18 : 20
                                source: root.isPlaying ? root.iconSource("video-pause") : root.iconSource("video-play")
                                fillMode: Image.PreserveAspectFit
                            }

                            MouseArea {
                                id: playPauseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.playPauseClicked()
                            }
                        }

                        Rectangle {
                            width: root.secondaryButtonSize
                            height: root.secondaryButtonSize
                            radius: width / 2
                            color: stopArea.pressed ? "#161B22" : (stopArea.containsMouse ? "#202733" : "#191E26")
                            border.width: 1
                            border.color: stopArea.containsMouse ? "#505C6F" : "#2D3440"

                            Image {
                                anchors.centerIn: parent
                                width: 14
                                height: 14
                                source: root.iconSource("video-stop")
                                fillMode: Image.PreserveAspectFit
                            }

                            MouseArea {
                                id: stopArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.stopClicked()
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Text {
                        text: root.currentMetaText()
                        visible: !root.compactLayout && videoWidth > 0
                        font.pixelSize: 11
                        color: "#7E8796"
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    function formatTime(ms) {
        var totalSeconds = Math.floor(Math.max(0, ms) / 1000)
        var hours = Math.floor(totalSeconds / 3600)
        var minutes = Math.floor((totalSeconds % 3600) / 60)
        var seconds = totalSeconds % 60

        if (hours > 0) {
            return hours + ":" +
                   (minutes < 10 ? "0" : "") + minutes + ":" +
                   (seconds < 10 ? "0" : "") + seconds
        }

        return (minutes < 10 ? "0" : "") + minutes + ":" +
               (seconds < 10 ? "0" : "") + seconds
    }
}
