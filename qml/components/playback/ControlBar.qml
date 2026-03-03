import QtQuick 2.14
import QtQuick.Controls 2.14

Item {
    id: root
    width: 250
    height: 70
    
    // 灞炴€?
    property int playState: 0  // 0: Stop, 1: Play, 2: Pause
    property bool loopState: false
    property bool isUp: false
    property int volumeValue: 50
    property bool volumeVisible: false
    property bool mlistChecked: false
    property bool deskChecked: false
    
    // 淇″彿
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
        
        // 寰幆鎾斁鎸夐挳
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
        
        // 涓婁竴棣栨寜閽?
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
        
        // 鎾斁/鏆傚仠鎸夐挳锛堝ぇ涓€鐐癸紝甯︾豢鑹插渾褰㈣儗鏅級
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
                color: playMouseArea.containsMouse ? "#FF5757" : "#EC4141"
                
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
        
        // 涓嬩竴棣栨寜閽?
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
        
        // 闊抽噺鎸夐挳
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
            
            // 闊抽噺婊戝潡
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
        
        // 妗岄潰姝岃瘝鎸夐挳
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
        
        // 鎾斁鍒楄〃鎸夐挳
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
    
    // 鍏紑缁?C++ 璋冪敤鐨勫嚱鏁?
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

