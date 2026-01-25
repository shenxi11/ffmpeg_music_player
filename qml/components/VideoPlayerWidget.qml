import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import VideoPlayer 1.0

Rectangle {
    id: root
    color: "#1E1E1E"  // 深色背景
    
    // 属性
    property int videoWidth: 0
    property int videoHeight: 0
    property double videoFPS: 0
    property int videoDuration: 0
    property bool isPlaying: false
    property int currentPosition: 0
    
    // 信号
    signal playPauseClicked()
    signal stopClicked()
    signal seekRequested(int positionMs)
    signal openVideoClicked()
    
    // 更新进度
    function updateProgress(progress) {
        progressBar.value = progress
    }
    
    // 显示错误信息
    function showError(errorMsg) {
        errorText.text = errorMsg
        errorText.visible = true
    }
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // 标题栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#2D2D2D"
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                
                Text {
                    text: "视频播放"
                    font.pixelSize: 18
                    font.bold: true
                    color: "#FFFFFF"
                }
                
                Item { Layout.fillWidth: true }
                
                // 选择视频按钮
                Rectangle {
                    width: 100
                    height: 36
                    radius: 18
                    color: openVideoArea.containsMouse ? "#31C27C" : "#3A3A3A"
                    
                    Text {
                        anchors.centerIn: parent
                        text: "📁 选择视频"
                        font.pixelSize: 13
                        color: "#FFFFFF"
                    }
                    
                    MouseArea {
                        id: openVideoArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.openVideoClicked()
                    }
                }
                
                Item { width: 20 }
                
                // 视频信息
                Text {
                    text: videoWidth > 0 ? videoWidth + "x" + videoHeight + " @ " + videoFPS.toFixed(1) + " FPS" : "未加载视频"
                    font.pixelSize: 12
                    color: "#AAAAAA"
                }
            }
        }
        
        // 视频显示区域
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#000000"
            
            // 视频画面 - 使用自定义的VideoFrameItem
            VideoFrameItem {
                id: videoFrameItem
                objectName: "videoFrameItem"
                anchors.fill: parent
            }
            
            // 占位符文本
            Text {
                anchors.centerIn: parent
                text: videoWidth === 0 ? "请选择视频文件" : ""
                font.pixelSize: 24
                color: "#666666"
                visible: videoWidth === 0
            }
            
            // 错误提示
            Text {
                id: errorText
                anchors.centerIn: parent
                font.pixelSize: 16
                color: "#E74C3C"
                visible: false
            }
            
            // 加载指示器（可选）
            BusyIndicator {
                id: loadingIndicator
                anchors.centerIn: parent
                running: false
                visible: running
            }
        }
        
        // 控制栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            color: "#2D2D2D"
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                
                // 进度条
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    
                    Text {
                        text: formatTime(currentPosition)
                        font.pixelSize: 12
                        color: "#AAAAAA"
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
                            implicitWidth: 200
                            implicitHeight: 4
                            width: progressBar.availableWidth
                            height: implicitHeight
                            radius: 2
                            color: "#4A4A4A"
                            
                            Rectangle {
                                width: progressBar.visualPosition * parent.width
                                height: parent.height
                                color: "#31C27C"
                                radius: 2
                            }
                        }
                        
                        handle: Rectangle {
                            x: progressBar.leftPadding + progressBar.visualPosition * (progressBar.availableWidth - width)
                            y: progressBar.topPadding + progressBar.availableHeight / 2 - height / 2
                            implicitWidth: 16
                            implicitHeight: 16
                            radius: 8
                            color: progressBar.pressed ? "#FFFFFF" : "#31C27C"
                            border.color: "#FFFFFF"
                            border.width: 2
                        }
                    }
                    
                    Text {
                        text: formatTime(videoDuration)
                        font.pixelSize: 12
                        color: "#AAAAAA"
                    }
                }
                
                // 控制按钮
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 15
                    
                    Item { Layout.fillWidth: true }
                    
                    // 播放/暂停按钮
                    Rectangle {
                        width: 50
                        height: 50
                        radius: 25
                        color: playPauseArea.containsMouse ? "#31C27C" : "#3A3A3A"
                        
                        Text {
                            anchors.centerIn: parent
                            text: root.isPlaying ? "⏸" : "▶"
                            font.pixelSize: 24
                            color: "#FFFFFF"
                        }
                        
                        MouseArea {
                            id: playPauseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.playPauseClicked()
                        }
                    }
                    
                    // 停止按钮
                    Rectangle {
                        width: 50
                        height: 50
                        radius: 25
                        color: stopArea.containsMouse ? "#E74C3C" : "#3A3A3A"
                        
                        Text {
                            anchors.centerIn: parent
                            text: "⏹"
                            font.pixelSize: 24
                            color: "#FFFFFF"
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
    
    // 时间格式化函数
    function formatTime(ms) {
        var totalSeconds = Math.floor(ms / 1000)
        var hours = Math.floor(totalSeconds / 3600)
        var minutes = Math.floor((totalSeconds % 3600) / 60)
        var seconds = totalSeconds % 60
        
        if (hours > 0) {
            return hours + ":" + 
                   (minutes < 10 ? "0" : "") + minutes + ":" + 
                   (seconds < 10 ? "0" : "") + seconds
        } else {
            return (minutes < 10 ? "0" : "") + minutes + ":" + 
                   (seconds < 10 ? "0" : "") + seconds
        }
    }
}
