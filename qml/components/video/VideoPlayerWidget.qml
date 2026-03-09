import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import VideoPlayer 1.0
import "../../theme/Theme.js" as Theme

Rectangle {
    id: root
    color: "#0F1115"
    property string playerIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/player/"

    property int pagePadding: width >= 1400 ? 28 : (width >= 1000 ? 22 : 16)
    property int actionButtonWidth: width >= 1200 ? 116 : 98
    property int actionButtonHeight: width >= 1200 ? 38 : 34
    property int controlButtonSize: width >= 1200 ? 52 : 46
    property int controlPanelHeight: width >= 1200 ? 104 : 96

    // 视频状态属性
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

    function updateProgress(progress) {
        var clamped = Math.max(0, Math.min(1.0, Number(progress)))
        progressBar.value = isNaN(clamped) ? 0 : clamped
    }

    function showError(errorMsg) {
        errorText.text = errorMsg || "播放失败"
        errorText.visible = true
        errorHideTimer.restart()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 62
            color: "#171A20"
            border.width: 1
            border.color: "#262A33"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: root.pagePadding
                anchors.rightMargin: root.pagePadding
                spacing: 12

                Text {
                    text: "视频播放"
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    color: "#F2F5FB"
                }

                Text {
                    text: videoWidth > 0
                          ? (videoWidth + " x " + videoHeight + "  ·  " + videoFPS.toFixed(1) + " FPS")
                          : "未加载视频"
                    font.pixelSize: 12
                    color: "#A4ACB9"
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    width: root.actionButtonWidth
                    height: root.actionButtonHeight
                    radius: height / 2
                    color: openVideoArea.containsMouse ? Theme.accent : "transparent"
                    border.width: 1
                    border.color: Theme.accent

                    Text {
                        anchors.centerIn: parent
                        text: "选择视频"
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        color: openVideoArea.containsMouse ? "#FFFFFF" : Theme.accent
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

            Text {
                anchors.centerIn: parent
                visible: videoWidth === 0
                text: "请选择视频文件"
                font.pixelSize: 24
                color: "#566075"
            }

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 18
                visible: errorText.visible
                radius: 10
                color: "#CC8B1F1F"
                border.width: 1
                border.color: "#E05555"
                width: Math.min(parent.width - 40, errorText.implicitWidth + 24)
                height: 38

                Text {
                    id: errorText
                    anchors.centerIn: parent
                    font.pixelSize: 13
                    color: "#FFFFFF"
                    visible: false
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
                interval: 3500
                repeat: false
                onTriggered: errorText.visible = false
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.controlPanelHeight
            color: "#171A20"
            border.width: 1
            border.color: "#262A33"

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: root.pagePadding
                anchors.rightMargin: root.pagePadding
                anchors.topMargin: 10
                anchors.bottomMargin: 10
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        text: formatTime(currentPosition)
                        font.pixelSize: 12
                        color: "#A4ACB9"
                    }

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
                            color: "#3A414E"

                            Rectangle {
                                width: progressBar.visualPosition * parent.width
                                height: parent.height
                                radius: 2
                                color: Theme.accent
                            }
                        }

                        handle: Rectangle {
                            x: progressBar.leftPadding + progressBar.visualPosition * (progressBar.availableWidth - width)
                            y: progressBar.topPadding + progressBar.availableHeight / 2 - height / 2
                            width: 14
                            height: 14
                            radius: 7
                            color: progressBar.pressed ? "#FFFFFF" : Theme.accent
                            border.width: 1
                            border.color: "#FFFFFF"
                        }
                    }

                    Text {
                        text: formatTime(videoDuration)
                        font.pixelSize: 12
                        color: "#A4ACB9"
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        width: root.controlButtonSize
                        height: root.controlButtonSize
                        radius: width / 2
                        color: playPauseArea.containsMouse ? "#FDECEC" : "transparent"
                        border.width: 1
                        border.color: Theme.accent

                        Image {
                            anchors.centerIn: parent
                            width: 22
                            height: 22
                            source: {
                                if (root.isPlaying) {
                                    return playPauseArea.containsMouse
                                            ? root.playerIconPrefix + "player_btn_pause_hover.svg"
                                            : root.playerIconPrefix + "player_btn_pause_default.svg"
                                }
                                return playPauseArea.containsMouse
                                        ? root.playerIconPrefix + "player_btn_play_hover.svg"
                                        : root.playerIconPrefix + "player_btn_play_default.svg"
                            }
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
                        width: root.controlButtonSize
                        height: root.controlButtonSize
                        radius: width / 2
                        color: stopArea.containsMouse ? "#FDECEC" : "transparent"
                        border.width: 1
                        border.color: stopArea.containsMouse ? Theme.accent : "#556070"

                        Image {
                            anchors.centerIn: parent
                            width: 20
                            height: 20
                            source: stopArea.containsMouse
                                    ? root.playerIconPrefix + "player_btn_stop_hover.svg"
                                    : root.playerIconPrefix + "player_btn_stop_default.svg"
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

                    Item { Layout.fillWidth: true }
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
