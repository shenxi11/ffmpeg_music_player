import QtQuick 2.14
import QtQuick.Controls 2.14
import QtGraphicalEffects 1.14

Rectangle {
    id: root
    color: "#F7F9FC"
    
    // 对外暴露的属性
    property int currentSelectedIndex: -1
    
    // 信号
    signal videoSelected(string videoPath, string videoName)
    signal refreshRequested()
    
    // 视频列表数据模型
    ListModel {
        id: videoListModel
    }
    
    // 顶部标题栏
    Rectangle {
        id: topBar
        width: parent.width
        height: 60
        color: "white"
        anchors.top: parent.top
        
        Rectangle {
            width: parent.width
            height: 1
            color: "#E0E0E0"
            anchors.bottom: parent.bottom
        }
        
        Row {
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            spacing: 15
            
            Text {
                text: "📹 在线视频"
                font.pixelSize: 18
                font.bold: true
                color: "#333333"
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: "(" + videoListModel.count + " 个视频)"
                font.pixelSize: 14
                color: "#999999"
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        
        // 刷新按钮
        Rectangle {
            id: refreshButton
            width: 80
            height: 32
            color: refreshMouseArea.containsMouse ? "#E8F5E9" : "#F1F3F5"
            radius: 16
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            border.width: 1
            border.color: refreshMouseArea.containsMouse ? "#4CAF50" : "#E0E0E0"
            
            Row {
                anchors.centerIn: parent
                spacing: 5
                
                Text {
                    text: "🔄"
                    font.pixelSize: 14
                    anchors.verticalCenter: parent.verticalCenter
                }
                
                Text {
                    text: "刷新"
                    font.pixelSize: 13
                    color: "#4CAF50"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            
            MouseArea {
                id: refreshMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.refreshRequested()
                }
            }
        }
    }
    
    // 视频列表
    ListView {
        id: videoListView
        anchors.top: topBar.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
        spacing: 8
        clip: true
        
        model: videoListModel
        
        delegate: Rectangle {
            id: videoItem
            width: videoListView.width
            height: 80
            color: videoMouseArea.containsMouse ? "#FFFFFF" : "#F8F9FA"
            radius: 8
            border.width: 1
            border.color: videoMouseArea.containsMouse ? "#4CAF50" : "#E8E8E8"
            
            // 添加阴影效果
            layer.enabled: videoMouseArea.containsMouse
            layer.effect: DropShadow {
                horizontalOffset: 0
                verticalOffset: 2
                radius: 8
                samples: 16
                color: "#20000000"
            }
            
            Row {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 15
                
                // 视频图标
                Rectangle {
                    width: 50
                    height: 50
                    color: "#E3F2FD"
                    radius: 8
                    anchors.verticalCenter: parent.verticalCenter
                    
                    Text {
                        text: "🎬"
                        font.pixelSize: 28
                        anchors.centerIn: parent
                    }
                }
                
                // 视频信息
                Column {
                    spacing: 8
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 80
                    
                    Text {
                        text: model.name
                        font.pixelSize: 16
                        font.bold: true
                        color: "#333333"
                        elide: Text.ElideRight
                        width: parent.width
                    }
                    
                    Row {
                        spacing: 15
                        
                        Row {
                            spacing: 5
                            
                            Text {
                                text: "📄"
                                font.pixelSize: 12
                            }
                            
                            Text {
                                text: model.path
                                font.pixelSize: 13
                                color: "#666666"
                            }
                        }
                        
                        Row {
                            spacing: 5
                            
                            Text {
                                text: "💾"
                                font.pixelSize: 12
                            }
                            
                            Text {
                                text: formatFileSize(model.size)
                                font.pixelSize: 13
                                color: "#666666"
                            }
                        }
                    }
                }
            }
            
            MouseArea {
                id: videoMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.currentSelectedIndex = index
                    root.videoSelected(model.path, model.name)
                }
            }
        }
        
        // 滚动条
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }
    }
    
    // 空状态提示
    Item {
        anchors.centerIn: parent
        visible: videoListModel.count === 0
        
        Column {
            anchors.centerIn: parent
            spacing: 15
            
            Text {
                text: "📹"
                font.pixelSize: 64
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            Text {
                text: "暂无视频"
                font.pixelSize: 18
                color: "#999999"
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            Text {
                text: "点击右上角刷新按钮加载视频列表"
                font.pixelSize: 14
                color: "#BBBBBB"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
    
    // 公开方法
    function addVideo(name, path, size) {
        videoListModel.append({
            "name": name,
            "path": path,
            "size": size
        })
    }
    
    function clearAll() {
        videoListModel.clear()
    }
    
    function getCount() {
        return videoListModel.count
    }
    
    function formatFileSize(bytes) {
        if (bytes < 1024) {
            return bytes + " B"
        } else if (bytes < 1024 * 1024) {
            return (bytes / 1024).toFixed(2) + " KB"
        } else if (bytes < 1024 * 1024 * 1024) {
            return (bytes / (1024 * 1024)).toFixed(2) + " MB"
        } else {
            return (bytes / (1024 * 1024 * 1024)).toFixed(2) + " GB"
        }
    }
}
