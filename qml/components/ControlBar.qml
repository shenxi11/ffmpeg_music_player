import QtQuick 2.14
import QtQuick.Controls 2.14

Item {
    id: root
    width: 250
    height: 40
    
    // 属性
    property int playState: 0  // 0: Stop, 1: Play, 2: Pause
    property bool loopState: false
    property bool isUp: false
    property int volumeValue: 50
    property bool volumeVisible: false
    property bool mlistChecked: false
    property bool deskChecked: false
    
    // 信号
    signal stop()
    signal nextSong()
    signal lastSong()
    signal volumeChanged(int value)
    signal mlistToggled(bool checked)
    signal playClicked()
    signal rePlay()
    signal deskToggled(bool checked)
    signal loopStateChanged(bool loop)
    
    Row {
        anchors.centerIn: parent
        spacing: 8
        
        // 循环播放按钮
        Rectangle {
            width: 28
            height: 28
            color: "transparent"
            radius: 14
            
            Image {
                anchors.centerIn: parent
                width: 20
                height: 20
                source: root.loopState ? 
                    (root.isUp ? "qrc:/new/prefix1/icon/loop_play_w.png" : "qrc:/new/prefix1/icon/loop_play.png") :
                    (root.isUp ? "qrc:/new/prefix1/icon/random_play_w.png" : "qrc:/new/prefix1/icon/random_play.png")
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
            
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onEntered: parent.color = "#E0E0E0"
                onExited: parent.color = "transparent"
                onClicked: {
                    root.loopState = !root.loopState
                    root.loopStateChanged(root.loopState)
                }
            }
        }
        
        // 上一首按钮
        Rectangle {
            width: 28
            height: 28
            color: "transparent"
            radius: 14
            
            Image {
                anchors.centerIn: parent
                width: 20
                height: 20
                source: root.isUp ? "qrc:/new/prefix1/icon/last_song_w.png" : "qrc:/new/prefix1/icon/last_song.png"
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
            
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onEntered: parent.color = "#E0E0E0"
                onExited: parent.color = "transparent"
                onClicked: root.lastSong()
            }
        }
        
        // 播放/暂停按钮（大一点，带绿色圆形背景）
        Rectangle {
            width: 36
            height: 36
            color: "transparent"
            radius: 18
            
            Rectangle {
                anchors.centerIn: parent
                width: 32
                height: 32
                radius: 16
                color: playMouseArea.containsMouse ? "#1ED760" : "#1DB954"
                
                Image {
                    anchors.centerIn: parent
                    width: 18
                    height: 18
                    source: {
                        if (root.playState === 1) { // Playing
                            return "qrc:/new/prefix1/icon/pause_w.png"
                        } else {
                            return "qrc:/new/prefix1/icon/play_w.png"
                        }
                    }
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }
            }
            
            MouseArea {
                id: playMouseArea
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onClicked: root.playClicked()
            }
        }
        
        // 下一首按钮
        Rectangle {
            width: 28
            height: 28
            color: "transparent"
            radius: 14
            
            Image {
                anchors.centerIn: parent
                width: 20
                height: 20
                source: root.isUp ? "qrc:/new/prefix1/icon/next_song_w.png" : "qrc:/new/prefix1/icon/next_song.png"
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
            
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onEntered: parent.color = "#E0E0E0"
                onExited: parent.color = "transparent"
                onClicked: root.nextSong()
            }
        }
        
        // 音量按钮
        Rectangle {
            width: 28
            height: 28
            color: "transparent"
            radius: 14
            
            Image {
                anchors.centerIn: parent
                width: 20
                height: 20
                source: root.isUp ? "qrc:/new/prefix1/icon/volume_w.png" : "qrc:/new/prefix1/icon/volume.png"
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
            
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onEntered: parent.color = "#E0E0E0"
                onExited: parent.color = "transparent"
                onClicked: {
                    root.volumeVisible = !root.volumeVisible
                }
            }
            
            // 音量滑块
            Rectangle {
                id: volumeSliderBg
                width: 100
                height: 30
                color: "#FFFFFF"
                radius: 4
                border.color: "#E0E0E0"
                border.width: 1
                visible: root.volumeVisible
                anchors.bottom: parent.top
                anchors.bottomMargin: 5
                anchors.horizontalCenter: parent.horizontalCenter
                
                Slider {
                    id: volumeSlider
                    anchors.fill: parent
                    anchors.margins: 5
                    from: 0
                    to: 100
                    value: root.volumeValue
                    onValueChanged: {
                        root.volumeValue = value
                        root.volumeChanged(value)
                    }
                }
            }
        }
        
        // 桌面歌词按钮
        Rectangle {
            width: 28
            height: 28
            color: "transparent"
            radius: 14
            
            Image {
                anchors.centerIn: parent
                width: 20
                height: 20
                source: root.isUp ? "qrc:/new/prefix1/icon/vocabulary_rights.png" : "qrc:/new/prefix1/icon/vocabulary_rights.png"
                fillMode: Image.PreserveAspectFit
                opacity: root.deskChecked ? 1.0 : 0.6
                smooth: true
            }
            
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onEntered: parent.color = "#E0E0E0"
                onExited: parent.color = "transparent"
                onClicked: {
                    root.deskChecked = !root.deskChecked
                    root.deskToggled(root.deskChecked)
                }
            }
        }
        
        // 播放列表按钮
        Rectangle {
            width: 28
            height: 28
            color: "transparent"
            radius: 14
            
            Image {
                anchors.centerIn: parent
                width: 20
                height: 20
                source: root.isUp ? "qrc:/new/prefix1/icon/musiclist.png" : "qrc:/new/prefix1/icon/musiclist.png"
                fillMode: Image.PreserveAspectFit
                opacity: root.mlistChecked ? 1.0 : 0.6
                smooth: true
            }
            
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onEntered: parent.color = "#E0E0E0"
                onExited: parent.color = "transparent"
                onClicked: {
                    root.mlistChecked = !root.mlistChecked
                    root.mlistToggled(root.mlistChecked)
                }
            }
        }
    }
    
    // 公开给 C++ 调用的函数
    function setPlayState(state) {
        root.playState = state
    }
    
    function getPlayState() {
        return root.playState
    }
    
    function setLoopState(loop) {
        root.loopState = loop
    }
    
    function getLoopState() {
        return root.loopState
    }
    
    function setIsUp(up) {
        root.isUp = up
    }
    
    function playFinished() {
        root.rePlay()
    }
}
