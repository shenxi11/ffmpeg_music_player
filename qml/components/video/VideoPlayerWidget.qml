import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import VideoPlayer 1.0

Rectangle {
    id: root
    color: "#1E1E1E"  // 娣辫壊鑳屾櫙
    
    // 灞炴€?
    property int videoWidth: 0
    property int videoHeight: 0
    property double videoFPS: 0
    property int videoDuration: 0
    property bool isPlaying: false
    property int currentPosition: 0
    
    // 淇″彿
    signal playPauseClicked()
    signal stopClicked()
    signal seekRequested(int positionMs)
    signal openVideoClicked()
    
    // 鏇存柊杩涘害
    function updateProgress(progress) {
        progressBar.value = progress
    }
    
    // 鏄剧ず閿欒淇℃伅
    function showError(errorMsg) {
        errorText.text = errorMsg
        errorText.visible = true
    }
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // 鏍囬鏍?
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#2D2D2D"
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                
                Text {
                    text: "瑙嗛鎾斁"
                    font.pixelSize: 18
                    font.bold: true
                    color: "#FFFFFF"
                }
                
                Item { Layout.fillWidth: true }
                
                // 閫夋嫨瑙嗛鎸夐挳
                Rectangle {
                    width: 100
                    height: 36
                    radius: 18
                    color: openVideoArea.containsMouse ? "#EC4141" : "#3A3A3A"
                    
                    Text {
                        anchors.centerIn: parent
                        text: "馃搧 閫夋嫨瑙嗛"
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
                
                // 瑙嗛淇℃伅
                Text {
                    text: videoWidth > 0 ? videoWidth + "x" + videoHeight + " @ " + videoFPS.toFixed(1) + " FPS" : "鏈姞杞借棰?
                    font.pixelSize: 12
                    color: "#AAAAAA"
                }
            }
        }
        
        // 瑙嗛鏄剧ず鍖哄煙
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#000000"
            
            // 瑙嗛鐢婚潰 - 浣跨敤鑷畾涔夌殑VideoFrameItem
            VideoFrameItem {
                id: videoFrameItem
                objectName: "videoFrameItem"
                anchors.fill: parent
            }
            
            // 鍗犱綅绗︽枃鏈?
            Text {
                anchors.centerIn: parent
                text: videoWidth === 0 ? "璇烽€夋嫨瑙嗛鏂囦欢" : ""
                font.pixelSize: 24
                color: "#666666"
                visible: videoWidth === 0
            }
            
            // 閿欒鎻愮ず
            Text {
                id: errorText
                anchors.centerIn: parent
                font.pixelSize: 16
                color: "#E74C3C"
                visible: false
            }
            
            // 鍔犺浇鎸囩ず鍣紙鍙€夛級
            BusyIndicator {
                id: loadingIndicator
                anchors.centerIn: parent
                running: false
                visible: running
            }
        }
        
        // 鎺у埗鏍?
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            color: "#2D2D2D"
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                
                // 杩涘害鏉?
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
                                color: "#EC4141"
                                radius: 2
                            }
                        }
                        
                        handle: Rectangle {
                            x: progressBar.leftPadding + progressBar.visualPosition * (progressBar.availableWidth - width)
                            y: progressBar.topPadding + progressBar.availableHeight / 2 - height / 2
                            implicitWidth: 16
                            implicitHeight: 16
                            radius: 8
                            color: progressBar.pressed ? "#FFFFFF" : "#EC4141"
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
                
                // 鎺у埗鎸夐挳
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 15
                    
                    Item { Layout.fillWidth: true }
                    
                    // 鎾斁/鏆傚仠鎸夐挳
                    Rectangle {
                        width: 50
                        height: 50
                        radius: 25
                        color: playPauseArea.containsMouse ? "#EC4141" : "#3A3A3A"
                        
                        Text {
                            anchors.centerIn: parent
                            text: root.isPlaying ? "鈴? : "鈻?
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
                    
                    // 鍋滄鎸夐挳
                    Rectangle {
                        width: 50
                        height: 50
                        radius: 25
                        color: stopArea.containsMouse ? "#E74C3C" : "#3A3A3A"
                        
                        Text {
                            anchors.centerIn: parent
                            text: "鈴?
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
    
    // 鏃堕棿鏍煎紡鍖栧嚱鏁?
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

