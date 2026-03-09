import QtQuick 2.14
import QtQuick.Window 2.14
import QtQuick.Controls 2.14
import QtGraphicalEffects 1.14
import "../../theme/Theme.js" as Theme

Window {
    id: root
    width: 196
    height: 208
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"

    // 0: 顺序播放, 1: 单曲循环, 2: 列表循环, 3: 随机播放
    property int currentMode: 2
    property bool ignoreNextFocusLoss: false
    property string playerIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/player/"

    signal modeChanged(int mode)

    Connections {
        target: Qt.application
        function onAboutToQuit() {
            root.close()
        }
    }

    onActiveChanged: {
        if (!active && !ignoreNextFocusLoss) {
            closeTimer.restart()
        }
        ignoreNextFocusLoss = false
    }

    Timer {
        id: closeTimer
        interval: 150
        onTriggered: {
            if (!root.active) {
                root.close()
            }
        }
    }

    ListModel {
        id: modeModel
        ListElement { text: "顺序播放"; iconBase: "player_mode_repeat"; mode: 0 }
        ListElement { text: "单曲循环"; iconBase: "player_mode_single"; mode: 1 }
        ListElement { text: "列表循环"; iconBase: "player_mode_repeat"; mode: 2 }
        ListElement { text: "随机播放"; iconBase: "player_mode_shuffle"; mode: 3 }
    }

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: Theme.glassStrong
        border.width: 1
        border.color: Theme.glassBorder

        layer.enabled: true
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: 5
            radius: 16
            samples: 25
            color: "#36000000"
        }

        ListView {
            anchors.fill: parent
            anchors.margins: 8
            clip: true
            spacing: 4
            interactive: false
            model: modeModel

            delegate: Rectangle {
                width: ListView.view.width
                height: 44
                radius: 9
                color: modeArea.containsMouse ? Theme.accentSoft : "transparent"
                border.width: root.currentMode === mode ? 1 : 0
                border.color: Theme.accent

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 10

                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 18
                        height: 18
                        source: {
                            if (modeArea.containsMouse) {
                                return root.playerIconPrefix + iconBase + "_hover.svg"
                            }
                            if (root.currentMode === mode) {
                                if (mode === 2) {
                                    return root.playerIconPrefix + "player_mode_repeat_active.svg"
                                }
                                return root.playerIconPrefix + iconBase + "_active.svg"
                            }
                            return root.playerIconPrefix + iconBase + "_default.svg"
                        }
                        fillMode: Image.PreserveAspectFit
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: model.text
                        font.pixelSize: 13
                        font.weight: root.currentMode === mode ? Font.DemiBold : Font.Normal
                        color: root.currentMode === mode ? Theme.accent : Theme.textPrimary
                    }

                    Item {
                        width: parent.width - 116
                        height: 1
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.currentMode === mode ? "✓" : ""
                        font.pixelSize: 14
                        color: Theme.accent
                    }
                }

                MouseArea {
                    id: modeArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.currentMode = mode
                        root.modeChanged(mode)
                        root.close()
                    }
                }
            }
        }
    }

    function setMode(mode) {
        root.currentMode = mode
    }
}
