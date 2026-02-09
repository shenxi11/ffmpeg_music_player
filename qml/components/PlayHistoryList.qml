import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// 播放历史列表
Rectangle {
    id: root
    color: "#f5f5f5"

    property bool isLoggedIn: false
    property string userAccount: ""
    property string currentPlayingPath: ""  // 当前播放的音乐路径

    signal playMusic(string filename)
    signal deleteHistory(var selectedPaths)
    signal loginRequested()
    signal refreshRequested()

    // 播放历史数据模型
    ListModel {
        id: historyModel
    }

    // 选中的项目
    property var selectedItems: []

    // 未登录状态界面
    Rectangle {
        anchors.fill: parent
        visible: !root.isLoggedIn
        color: "#f5f5f5"

        Column {
            anchors.centerIn: parent
            spacing: 20

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "♪"
                font.pixelSize: 60
                color: "#409EFF"
                font.bold: true
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "登录后可显示最近播放歌曲"
                font.pixelSize: 18
                color: "#333333"
                font.bold: true
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "最近播放历史支持云漫游"
                font.pixelSize: 14
                color: "#999999"
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 160
                height: 42
                text: "立即登录"

                background: Rectangle {
                    color: parent.pressed ? "#3a8ee6" : 
                           parent.hovered ? "#66b1ff" : "#409EFF"
                    radius: 4
                }

                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 15
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    root.loginRequested()
                }
            }
        }
    }

    // 已登录状态界面
    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        visible: root.isLoggedIn

        // 标题栏和操作按钮
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            spacing: 10

            Text {
                text: "播放历史"
                font.pixelSize: 20
                font.bold: true
                color: "#333333"
                Layout.fillWidth: true
            }

            Button {
                text: "刷新"
                
                background: Rectangle {
                    color: parent.pressed ? "#3a8ee6" : 
                           parent.hovered ? "#66b1ff" : "#409EFF"
                    radius: 4
                }

                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    root.refreshRequested()
                }
            }
        }

        // 音乐列表
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            spacing: 2
            clip: true

            model: historyModel

            delegate: Rectangle {
                id: itemRoot
                width: listView.width
                height: 60
                color: itemRoot.containsMouse ? "#E8F4FF" : (index % 2 === 0 ? "#FFFFFF" : "#FAFAFA")

                property bool containsMouse: false
                property bool isPlaying: root.currentPlayingPath === model.path

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    spacing: 10

                    // 序号
                    Text {
                        text: (index + 1).toString()
                        width: 50
                        font.pixelSize: 13
                        color: "#666666"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    // 封面（真实封面或默认音乐图标）
                    Rectangle {
                        width: 44
                        height: 44
                        anchors.verticalCenter: parent.verticalCenter
                        radius: 4
                        color: "#E0E0E0"

                        Image {
                            id: coverImage
                            anchors.fill: parent
                            anchors.margins: 2
                            source: {
                                var url = model.cover_art_url || ""
                                if (url && url.length > 0) {
                                    return url
                                }
                                return "qrc:/new/prefix1/icon/Music.png"
                            }
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            cache: true
                            
                            onStatusChanged: {
                                if (status === Image.Error) {
                                    console.log("[PlayHistory] Failed to load cover:", source)
                                    source = "qrc:/new/prefix1/icon/Music.png"
                                } else if (status === Image.Ready) {
                                    console.log("[PlayHistory] Cover loaded:", source)
                                }
                            }
                        }
                    }

                    // 歌曲信息列
                    Column {
                        width: 240
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 4

                        Text {
                            text: model.title || model.path.split('/').pop()
                            font.pixelSize: 14
                            font.bold: itemRoot.isPlaying
                            color: itemRoot.isPlaying ? "#4A90E2" : "#333333"
                            elide: Text.ElideRight
                            width: parent.width
                        }

                        Text {
                            text: model.artist || "未知艺术家"
                            font.pixelSize: 11
                            color: "#888888"
                        }
                    }

                    Item { width: 10 }

                    // 时长
                    Text {
                        text: model.duration || "--:--"
                        width: 80
                        font.pixelSize: 12
                        color: "#666666"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    // 播放时间
                    Text {
                        text: model.play_time || ""
                        width: 130
                        font.pixelSize: 12
                        color: "#999999"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Item { width: 10 }

                    // 操作按钮（hover时显示）
                    Row {
                        spacing: 8
                        anchors.verticalCenter: parent.verticalCenter
                        opacity: itemRoot.containsMouse ? 1.0 : 0.0
                        visible: opacity > 0

                        Behavior on opacity { NumberAnimation { duration: 150 } }

                        // 喜欢按钮
                        Rectangle {
                            width: 32
                            height: 32
                            radius: 16
                            color: favBtnArea.containsMouse ? "#ffe0e6" : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: "♡"
                                font.pixelSize: 16
                                color: favBtnArea.containsMouse ? "#ff0000" : "#999999"
                            }

                            MouseArea {
                                id: favBtnArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    // TODO: 添加到喜欢列表
                                }
                            }
                        }

                        // 播放按钮
                        Rectangle {
                            width: 32
                            height: 32
                            radius: 16
                            color: playBtnArea.containsMouse ? "#4A90E2" : "#DDDDDD"

                            Text {
                                anchors.centerIn: parent
                                text: itemRoot.isPlaying ? "⏸" : "▶"
                                font.pixelSize: 14
                                color: playBtnArea.containsMouse ? "white" : "#333333"
                            }

                            MouseArea {
                                id: playBtnArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.playMusic(model.path || "")
                                }
                            }
                        }

                        // 删除按钮
                        Rectangle {
                            width: 60
                            height: 28
                            radius: 4
                            color: deleteBtnArea.containsMouse ? "#E74C3C" : "#F0F0F0"

                            Text {
                                anchors.centerIn: parent
                                text: "删除"
                                font.pixelSize: 12
                                color: deleteBtnArea.containsMouse ? "white" : "#666666"
                            }

                            MouseArea {
                                id: deleteBtnArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.deleteHistory([model.path])
                                }
                            }
                        }
                    }
                }

                // 整个item的悬停检测
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    propagateComposedEvents: true
                    onEntered: itemRoot.containsMouse = true
                    onExited: itemRoot.containsMouse = false
                    onPressed: mouse.accepted = false
                    onReleased: mouse.accepted = false
                    onClicked: mouse.accepted = false
                    onDoubleClicked: {
                        root.playMusic(model.path || "")
                    }
                }
            }

            // 空状态
            Label {
                anchors.centerIn: parent
                text: "暂无播放历史"
                font.pixelSize: 16
                color: "#AAAAAA"
                visible: listView.count === 0
            }
        }
    }

    // 公共函数：加载播放历史
    function loadHistory(historyData) {
        historyModel.clear()
        root.selectedItems = []
        
        console.log("[PlayHistoryList] Loading", historyData.length, "items")
        
        for (var i = 0; i < historyData.length; i++) {
            var item = historyData[i]
            item.uniqueId = i  // 添加唯一ID，避免重复歌曲删除问题
            
            // 调试输出
            console.log("[PlayHistoryList] Item", i, "- title:", item.title, "cover_art_url:", item.cover_art_url)
            
            historyModel.append(item)
        }
    }

    // 清空列表
    function clearHistory() {
        historyModel.clear()
        root.selectedItems = []
    }
}
