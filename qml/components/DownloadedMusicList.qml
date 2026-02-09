import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// 已下载音乐列表
Rectangle {
    id: root
    color: "#f5f5f5"

    signal playMusic(string filename)
    signal deleteMusic(string filename)
    signal addToFavorite(string path, string title, string artist, string duration)
    
    // 当可见性改变时刷新
    onVisibleChanged: {
        if (visible) {
            console.log("[DownloadedMusicList] Became visible, refreshing completed tasks...")
            if (typeof downloadTaskModel !== 'undefined') {
                downloadTaskModel.refresh(true)
            }
        }
    }

    Component.onCompleted: {
        console.log("[DownloadedMusicList] Component completed")
        // 刷新已下载的任务
        if (typeof downloadTaskModel !== 'undefined') {
            console.log("[DownloadedMusicList] Initial refresh")
            downloadTaskModel.refresh(true)
        }
    }
    
    // 添加连接监听模型变化
    Connections {
        target: typeof downloadTaskModel !== 'undefined' ? downloadTaskModel : null
        function onRowsInserted() {
            console.log("[DownloadedMusicList] Rows inserted, count:", listView.count)
        }
        function onModelReset() {
            console.log("[DownloadedMusicList] Model reset, count:", listView.count)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        // 标题栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "#ffffff"
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 15
                anchors.rightMargin: 15
                spacing: 10

                Text {
                    text: "文件名"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.preferredWidth: 300
                }

                Text {
                    text: "大小"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.preferredWidth: 120
                }

                Text {
                    text: "下载时间"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.preferredWidth: 150
                }

                Text {
                    text: "操作"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.fillWidth: true
                }
            }
        }

        // 音乐列表
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8
            clip: true

            model: typeof downloadTaskModel !== 'undefined' ? downloadTaskModel : null

            delegate: Rectangle {
                width: listView.width
                height: 60
                color: itemArea.containsMouse || playBtnArea.containsMouse || deleteBtnArea.containsMouse ? "#f0f0f0" : "#ffffff"
                radius: 4

                MouseArea {
                    id: itemArea
                    anchors.fill: parent
                    hoverEnabled: true
                    propagateComposedEvents: true
                    onDoubleClicked: {
                        root.playMusic(model.savePath || "")
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    // 专辑图片
                    Rectangle {
                        Layout.preferredWidth: 44
                        Layout.preferredHeight: 44
                        radius: 4
                        color: "#E0E0E0"
                        
                        Image {
                            anchors.fill: parent
                            anchors.margins: 2
                            source: model.coverUrl || "qrc:/new/prefix1/icon/Music.png"
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                        }
                    }

                    // 文件名
                    Text {
                        Layout.preferredWidth: 240
                        text: model.filename || ""
                        font.pixelSize: 14
                        font.bold: model.isPlaying
                        color: model.isPlaying ? "#4A90E2" : "#333333"
                        elide: Text.ElideMiddle
                    }

                    // 文件大小
                    Text {
                        Layout.preferredWidth: 120
                        text: formatSize(model.totalSize || 0)
                        font.pixelSize: 13
                        color: "#666666"
                    }

                    // 下载时间（使用创建时间）
                    Text {
                        Layout.preferredWidth: 150
                        text: Qt.formatDateTime(new Date(), "yyyy-MM-dd hh:mm")
                        font.pixelSize: 13
                        color: "#666666"
                    }

                    // 操作按钮
                    Row {
                        Layout.fillWidth: true
                        spacing: 10
                        opacity: itemArea.containsMouse || playBtnArea.containsMouse || favBtnArea.containsMouse || deleteBtnArea.containsMouse ? 1.0 : 0.0
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
                                propagateComposedEvents: false
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.addToFavorite(
                                        model.savePath || "",
                                        model.songName || "",
                                        model.artist || "",
                                        model.duration || ""
                                    )
                                }
                            }
                        }

                        // 播放按钮（圆形图标样式）
                        Rectangle {
                            width: 32
                            height: 32
                            radius: 16
                            color: playBtnArea.containsMouse ? "#4A90E2" : "#DDDDDD"

                            Text {
                                anchors.centerIn: parent
                                text: model.isPlaying ? "⏸" : "▶"
                                font.pixelSize: 14
                                color: playBtnArea.containsMouse ? "white" : "#333333"
                            }

                            MouseArea {
                                id: playBtnArea
                                anchors.fill: parent
                                hoverEnabled: true
                                propagateComposedEvents: false
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.playMusic(model.savePath || "")
                                }
                                onEntered: {
                                    // 阻止事件传播到itemArea
                                }
                                onExited: {
                                    // 阻止事件传播到itemArea
                                }
                            }
                        }

                        // 删除按钮（圆形图标样式）
                        Rectangle {
                            width: 60
                            height: 28
                            radius: 4
                            color: deleteBtnArea.containsMouse ? "#F56C6C" : "#F0F0F0"

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
                                propagateComposedEvents: false
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    // 先发送删除信号
                                    root.deleteMusic(model.savePath || "")
                                    // 从模型中移除任务
                                    if (typeof downloadTaskModel !== 'undefined') {
                                        downloadTaskModel.removeTask(model.taskId || "")
                                    }
                                }
                                onEntered: {
                                    // 阻止事件传播到itemArea
                                }
                                onExited: {
                                    // 阻止事件传播到itemArea
                                }
                            }
                        }
                    }
                }
            }

            // 空状态提示
            Text {
                anchors.centerIn: parent
                text: "暂无已下载的音乐"
                font.pixelSize: 16
                color: "#999999"
                visible: listView.count === 0
            }
        }
    }

    // 格式化文件大小
    function formatSize(bytes) {
        if (bytes < 1024) return bytes + " B"
        if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(2) + " KB"
        if (bytes < 1024 * 1024 * 1024) return (bytes / (1024 * 1024)).toFixed(2) + " MB"
        return (bytes / (1024 * 1024 * 1024)).toFixed(2) + " GB"
    }
}
