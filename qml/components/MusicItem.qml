import QtQuick 2.14
import QtQuick.Controls 2.14
import QtGraphicalEffects 1.14

Rectangle {
    id: root
    width: 600
    height: 60
    color: "transparent"

    property string songName: "未知歌曲"
    property string artist: "未知歌手"
    property string duration: "0:00"
    property string cover: ""
    property string filePath: ""
    property bool isNet: false
    property bool isPlaying: false  // 播放状态

    signal playRequested(string songName)
    signal removeRequested(string songName)
    signal downloadRequested(string songName)

    Rectangle {
        id: background
        anchors.fill: parent
        radius: 4
        color: hoverArea.containsMouse ? "#E8F4FF" : "#FFFFFF"
        border.width: hoverArea.containsMouse ? 1 : 0
        border.color: "#4A90E2"
    }

    Row {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 15

        // 封面图片
        Rectangle {
            width: 44
            height: 44
            anchors.verticalCenter: parent.verticalCenter
            radius: 22
            color: "#E0E0E0"
            
            Image {
                id: coverImg
                anchors.fill: parent
                anchors.margins: 2
                source: root.cover !== "" ? root.cover : "qrc:/new/prefix1/icon/Music.png"
                fillMode: Image.PreserveAspectCrop
                visible: false
            }
            
            Rectangle {
                id: mask
                anchors.fill: coverImg
                radius: width / 2
                visible: false
            }
            
            OpacityMask {
                anchors.fill: coverImg
                source: coverImg
                maskSource: mask
            }
        }

        // 歌曲信息
        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4
            width: 300
            
            Text { 
                text: root.songName
                font.pixelSize: 14
                font.bold: true
                color: "#333333"
                elide: Text.ElideRight
                width: parent.width
            }
            Text { 
                text: root.artist !== "" ? root.artist : "未知艺术家"
                font.pixelSize: 11
                color: "#888888"
            }
        }

        Item { width: 50 }  // 弹性空间

        // 时长
        Text {
            text: root.duration
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 12
            color: "#666666"
            width: 50
        }

        // 操作按钮，hover 时显示
        Row {
            id: buttonRow
            spacing: 8
            anchors.verticalCenter: parent.verticalCenter
            opacity: hoverArea.containsMouse ? 1.0 : 0.0
            visible: opacity > 0
            
            Behavior on opacity { NumberAnimation { duration: 150 } }

            // 播放按钮
            Rectangle {
                width: 32
                height: 32
                radius: 16
                color: playBtnArea.containsMouse ? "#4A90E2" : "#DDDDDD"
                
                Text {
                    anchors.centerIn: parent
                    text: root.isPlaying ? "⏸" : "▶"
                    font.pixelSize: 14
                    color: playBtnArea.containsMouse ? "white" : "#333333"
                }
                
                MouseArea {
                    id: playBtnArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.playRequested(root.filePath)
                }
            }

            // 删除按钮（本地音乐）
            Rectangle {
                width: 60
                height: 28
                radius: 4
                color: removeBtnArea.containsMouse ? "#FF6B6B" : "#F0F0F0"
                visible: !root.isNet
                
                Text {
                    anchors.centerIn: parent
                    text: "删除"
                    font.pixelSize: 12
                    color: removeBtnArea.containsMouse ? "white" : "#666666"
                }
                
                MouseArea {
                    id: removeBtnArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.removeRequested(root.filePath)
                }
            }

            // 下载按钮（网络音乐）
            Rectangle {
                width: 60
                height: 28
                radius: 4
                color: downloadBtnArea.containsMouse ? "#51CF66" : "#F0F0F0"
                visible: root.isNet
                
                Text {
                    anchors.centerIn: parent
                    text: "下载"
                    font.pixelSize: 12
                    color: downloadBtnArea.containsMouse ? "white" : "#666666"
                }
                
                MouseArea {
                    id: downloadBtnArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.downloadRequested(root.filePath)
                }
            }
        }
    }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            // 点击整体项视为播放
            root.playRequested(root.songName)
        }
    }

    // 提供给 C++ 调用的方法
    function setPlayingState(playing) {
        root.isPlaying = playing
    }

    function triggerPlay() {
        root.playRequested(root.filePath)
    }
}
