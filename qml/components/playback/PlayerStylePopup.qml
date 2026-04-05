import QtQuick 2.14
import QtQuick.Window 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.14
import "../../theme/Theme.js" as Theme
import "../../theme/PlayerStyle.js" as PlayerStyle

Window {
    id: root
    width: 260
    height: 388
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"

    property int currentStyle: 0
    property bool ignoreNextFocusLoss: false

    signal styleSelected(int styleId)

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
        id: styleModel
        ListElement { name: "经典黑胶"; styleId: 0 }
        ListElement { name: "简约方形"; styleId: 1 }
        ListElement { name: "透明彩胶"; styleId: 2 }
        ListElement { name: "简约歌词"; styleId: 3 }
        ListElement { name: "歌手写真"; styleId: 4 }
    }

    Rectangle {
        anchors.fill: parent
        radius: 18
        color: "#F8FFFFFF"
        border.width: 1
        border.color: "#2E20263A"

        layer.enabled: true
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: 8
            radius: 22
            samples: 29
            color: "#2A000000"
        }

        Column {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Item {
                width: parent.width
                height: 46

                RowLayout {
                    anchors.fill: parent
                    spacing: 10

                    Rectangle {
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 26
                        width: 26
                        height: 26
                        radius: 13
                        color: Theme.accentSoft
                        border.width: 1
                        border.color: "#22D33A31"

                        Text {
                            anchors.centerIn: parent
                            text: "♪"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            color: Theme.accent
                        }
                    }

                    Column {
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 2

                        Text {
                            text: "播放样式"
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            color: Theme.textPrimary
                        }

                        Text {
                            text: "切换当前展开页视觉风格"
                            font.pixelSize: 11
                            color: Theme.textMuted
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                radius: 1
                color: Theme.divider
                opacity: 0.8
            }

            Repeater {
                model: styleModel

                delegate: Rectangle {
                    property var styleSpec: PlayerStyle.styleFor(styleId)
                    property string summary: styleId === 0 ? "黑胶与柔光背景"
                                             : styleId === 1 ? "方形封面与深色卡片"
                                             : styleId === 2 ? "通透彩胶与亮色玻璃"
                                             : styleId === 3 ? "弱化封面聚焦歌词"
                                             : "写真背景与沉浸舞台"
                    width: parent.width
                    height: 56
                    radius: 14
                    color: {
                        if (root.currentStyle === styleId) {
                            return Theme.accentSoft
                        }
                        return mouseArea.containsMouse ? "#FFF6F5" : "transparent"
                    }
                    border.width: root.currentStyle === styleId ? 1 : 0
                    border.color: root.currentStyle === styleId ? Theme.accent : "transparent"

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 12

                        Rectangle {
                            width: 58
                            height: parent.height
                            radius: 12
                            color: styleSpec.previewSecondary
                            border.width: 1
                            border.color: "#1A20263A"
                            clip: true

                            Rectangle {
                                anchors.fill: parent
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: styleSpec.previewPrimary }
                                    GradientStop { position: 1.0; color: styleSpec.previewSecondary }
                                }
                            }

                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 10
                                color: styleSpec.controlBarColor
                                opacity: 0.92
                            }

                            Rectangle {
                                visible: styleId === 0 || styleId === 2
                                width: styleId === 2 ? 24 : 22
                                height: width
                                radius: width / 2
                                x: styleId === 2 ? 26 : 28
                                y: styleId === 2 ? 6 : 7
                                color: styleSpec.previewDiscColor
                                opacity: styleId === 2 ? 0.66 : 0.96
                                border.width: 1
                                border.color: "#28FFFFFF"

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 9
                                    height: 9
                                    radius: 4.5
                                    color: styleSpec.previewPrimary
                                }
                            }

                            Rectangle {
                                visible: styleId === 1 || styleId === 4
                                width: 18
                                height: 18
                                radius: 6
                                x: styleId === 4 ? 6 : 8
                                y: 8
                                color: styleSpec.previewDiscColor
                                border.width: 1
                                border.color: "#30FFFFFF"
                            }

                            Rectangle {
                                visible: styleId === 4
                                x: 0
                                y: 0
                                width: parent.width
                                height: parent.height
                                color: "#26000000"
                            }

                            Rectangle {
                                visible: styleId === 3
                                x: 8
                                y: 10
                                width: parent.width - 16
                                height: 2
                                radius: 1
                                color: "#D8FFFFFF"
                                opacity: 0.86
                            }

                            Rectangle {
                                visible: styleId === 3
                                x: 12
                                y: 17
                                width: parent.width - 24
                                height: 2
                                radius: 1
                                color: "#8FFFFFFF"
                                opacity: 0.72
                            }

                            Rectangle {
                                visible: styleId === 3
                                x: 10
                                y: 24
                                width: parent.width - 20
                                height: 2
                                radius: 1
                                color: "#B8FFFFFF"
                                opacity: 0.78
                            }

                            Rectangle {
                                visible: styleId !== 3
                                x: styleId === 4 ? 28 : 8
                                y: styleId === 1 ? 10 : 12
                                width: styleId === 4 ? 18 : 16
                                height: 2
                                radius: 1
                                color: styleSpec.lyricsTitleColor
                                opacity: 0.95
                            }

                            Rectangle {
                                visible: styleId !== 3
                                x: styleId === 4 ? 28 : 8
                                y: styleId === 1 ? 16 : 18
                                width: styleId === 4 ? 14 : 18
                                height: 2
                                radius: 1
                                color: styleSpec.lyricsArtistColor
                                opacity: 0.82
                            }
                        }

                        Column {
                            width: 124
                            height: parent.height
                            spacing: 2

                            Item {
                                width: 1
                                height: 8
                            }

                            Text {
                                text: name
                                font.pixelSize: 12
                                font.weight: root.currentStyle === styleId ? Font.DemiBold : Font.Medium
                                color: root.currentStyle === styleId ? Theme.accent : Theme.textPrimary
                            }

                            Text {
                                text: summary
                                font.pixelSize: 10
                                color: Theme.textMuted
                            }
                        }

                        Item {
                            width: Math.max(0, parent.width - 58 - 12 - 124 - 12 - 38)
                            height: 1
                        }

                        Rectangle {
                            y: Math.round((parent.height - height) / 2)
                            width: root.currentStyle === styleId ? 38 : 18
                            height: 20
                            radius: 10
                            color: root.currentStyle === styleId ? Theme.accent : "transparent"
                            border.width: root.currentStyle === styleId ? 0 : 1
                            border.color: "#1A20263A"

                            Text {
                                anchors.centerIn: parent
                                text: root.currentStyle === styleId ? "当前" : ""
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                                color: "#FFFFFF"
                            }
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.currentStyle = styleId
                            root.styleSelected(styleId)
                            root.close()
                        }
                    }
                }
            }
        }
    }
}
