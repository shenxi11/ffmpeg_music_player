import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// 正在下载列表
Rectangle {
    id: root
    color: "#f5f5f5"
    
    // 当可见性改变时刷新
    onVisibleChanged: {
        if (visible) {
            console.log("[DownloadingList] Became visible, refreshing active tasks...")
            if (typeof downloadTaskModel !== 'undefined') {
                downloadTaskModel.refresh(false)  // false = 活跃任务
            }
        }
    }

    Component.onCompleted: {
        console.log("[DownloadingList] Component completed")
        // 刷新正在下载的任务
        if (typeof downloadTaskModel !== 'undefined') {
            console.log("[DownloadingList] Initial refresh")
            downloadTaskModel.refresh(false)
        }
    }
    
    // 添加连接监听模型变化
    Connections {
        target: typeof downloadTaskModel !== 'undefined' ? downloadTaskModel : null
        function onRowsInserted() {
            console.log("[DownloadingList] Rows inserted, count:", listView.count)
        }
        function onModelReset() {
            console.log("[DownloadingList] Model reset, count:", listView.count)
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
                    Layout.preferredWidth: 250
                }

                Text {
                    text: "进度"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.preferredWidth: 200
                }

                Text {
                    text: "大小"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.preferredWidth: 100
                }

                Text {
                    text: "状态"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.preferredWidth: 80
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

        // 下载任务列表
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8
            clip: true

            model: typeof downloadTaskModel !== 'undefined' ? downloadTaskModel : null

            delegate: Rectangle {
                width: listView.width
                height: 80
                color: "#ffffff"
                radius: 4

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    // 文件名
                    Column {
                        Layout.preferredWidth: 250
                        spacing: 4

                        Text {
                            text: model.filename || ""
                            font.pixelSize: 14
                            color: "#333333"
                            elide: Text.ElideMiddle
                            width: 250
                        }

                        Text {
                            text: model.errorMsg || ""
                            font.pixelSize: 12
                            color: "#ff0000"
                            visible: model.state === 4  // Failed
                            width: 250
                            elide: Text.ElideRight
                        }
                    }

                    // 进度条
                    Column {
                        Layout.preferredWidth: 200
                        spacing: 4

                        ProgressBar {
                            width: 200
                            height: 20
                            from: 0
                            to: 100
                            value: model.progress || 0

                            background: Rectangle {
                                implicitWidth: 200
                                implicitHeight: 20
                                color: "#e0e0e0"
                                radius: 3
                            }

                            contentItem: Item {
                                implicitWidth: 200
                                implicitHeight: 18

                                Rectangle {
                                    width: parent.width * (parent.parent.visualPosition)
                                    height: parent.height
                                    radius: 2
                                    color: "#409EFF"
                                }
                            }
                        }

                        Text {
                            text: (model.progress || 0) + "%"
                            font.pixelSize: 12
                            color: "#666666"
                        }
                    }

                    // 文件大小
                    Text {
                        Layout.preferredWidth: 100
                        text: formatSize(model.downloadedSize || 0) + " / " + formatSize(model.totalSize || 0)
                        font.pixelSize: 12
                        color: "#666666"
                    }

                    // 状态
                    Text {
                        Layout.preferredWidth: 80
                        text: model.stateText || ""
                        font.pixelSize: 12
                        color: getStateColor(model.state || 0)
                        font.bold: true
                    }

                    // 操作按钮
                    Row {
                        Layout.fillWidth: true
                        spacing: 10

                        // 暂停/恢复按钮
                        Button {
                            text: model.state === 1 ? "暂停" : "恢复"  // 1=Downloading, 2=Paused
                            visible: model.state === 1 || model.state === 2
                            height: 28
                            
                            background: Rectangle {
                                color: parent.hovered ? "#66b1ff" : "#409EFF"
                                radius: 4
                            }

                            contentItem: Text {
                                text: parent.text
                                font.pixelSize: 12
                                color: "#ffffff"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            onClicked: {
                                if (model.state === 1) {  // Downloading
                                    if (typeof downloadTaskModel !== 'undefined') {
                                        downloadTaskModel.pauseTask(model.taskId)
                                    }
                                } else {  // Paused
                                    if (typeof downloadTaskModel !== 'undefined') {
                                        downloadTaskModel.resumeTask(model.taskId)
                                    }
                                }
                            }
                        }

                        // 取消按钮
                        Button {
                            text: "取消"
                            visible: model.state !== 3 && model.state !== 5  // 不是Completed和Cancelled
                            height: 28

                            background: Rectangle {
                                color: parent.hovered ? "#f78989" : "#f56c6c"
                                radius: 4
                            }

                            contentItem: Text {
                                text: parent.text
                                font.pixelSize: 12
                                color: "#ffffff"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            onClicked: {
                                if (typeof downloadTaskModel !== 'undefined') {
                                    downloadTaskModel.cancelTask(model.taskId)
                                }
                            }
                        }

                        // 重试按钮（失败时显示）
                        Button {
                            text: "重试"
                            visible: model.state === 4  // Failed
                            height: 28

                            background: Rectangle {
                                color: parent.hovered ? "#85ce61" : "#67c23a"
                                radius: 4
                            }

                            contentItem: Text {
                                text: parent.text
                                font.pixelSize: 12
                                color: "#ffffff"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            onClicked: {
                                if (typeof downloadTaskModel !== 'undefined') {
                                    downloadTaskModel.resumeTask(model.taskId)
                                }
                            }
                        }
                    }
                }
            }

            // 空状态提示
            Text {
                anchors.centerIn: parent
                text: "暂无下载任务"
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

    // 获取状态颜色
    function getStateColor(state) {
        switch(state) {
            case 0: return "#909399"  // Waiting
            case 1: return "#409EFF"  // Downloading
            case 2: return "#e6a23c"  // Paused
            case 3: return "#67c23a"  // Completed
            case 4: return "#f56c6c"  // Failed
            case 5: return "#909399"  // Cancelled
            default: return "#909399"
        }
    }
}
