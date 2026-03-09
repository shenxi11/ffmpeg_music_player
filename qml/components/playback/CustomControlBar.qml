import QtQuick 2.14

Item {
    id: root
    width: 1200
    height: 100
    
    signal playPauseClicked()
    signal previousClicked()
    signal nextClicked()
    signal volumeLevelChanged(real volume)
    signal seekToPosition(real position)
    signal loopModeToggled()
    signal desktopLrcToggled()
    signal listToggled()
    
    property bool isPlaying: false
    property real volume: 0.8
    property real progress: 0.0
    property int currentSeconds: 0
    property int totalSeconds: 0
    property string songName: "鏆傛棤鎾斁"
    property string artist: ""
    property string coverUrl: ""
    property int loopMode: 0
    
    Row {
        id: leftSection
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        spacing: 15
        
        Rectangle {
            width: 65
            height: 65
            radius: 8
            color: "#34495e"
            anchors.verticalCenter: parent.verticalCenter
            clip: true
            
            Image {
                id: coverImage
                anchors.fill: parent
                anchors.margins: 2
                source: root.coverUrl || "qrc:/new/prefix1/icon/netease.ico"
                fillMode: Image.PreserveAspectCrop
            }
            
            RotationAnimator {
                target: coverImage
                from: 0
                to: 360
                duration: 20000
                loops: Animation.Infinite
                running: root.isPlaying
            }
        }
        
        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4
            
            Text {
                text: root.songName
                font.family: "Microsoft YaHei"
                font.pixelSize: 15
                font.bold: true
                color: "white"
                width: 200
                elide: Text.ElideRight
            }
            
            Text {
                text: root.artist || "鏈煡鑹烘湳瀹?
                font.family: "Microsoft YaHei"
                font.pixelSize: 12
                color: "#B3FFFFFF"
                width: 200
                elide: Text.ElideRight
            }
        }
    }
    
    Column {
        id: centerSection
        anchors.centerIn: parent
        width: 600
        spacing: 12
        
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 25
            
            Rectangle {
                width: 40
                height: 40
                radius: 20
                color: prevBtn.containsMouse ? "#40FFFFFF" : "transparent"
                
                Text {
                    text: "鈴?
                    font.pixelSize: 20
                    color: "white"
                    anchors.centerIn: parent
                }
                
                MouseArea {
                    id: prevBtn
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.previousClicked()
                }
            }
            
            Rectangle {
                width: 50
                height: 50
                radius: 25
                color: playBtn.containsMouse ? "#F2FFFFFF" : "#D9FFFFFF"
                
                Text {
                    text: root.isPlaying ? "鈴? : "鈻?
                    font.pixelSize: 20
                    color: "#667eea"
                    anchors.centerIn: parent
                }
                
                MouseArea {
                    id: playBtn
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.playPauseClicked()
                }
                
                scale: playBtn.pressed ? 0.9 : 1.0
                Behavior on scale { NumberAnimation { duration: 100 } }
            }
            
            Rectangle {
                width: 40
                height: 40
                radius: 20
                color: nextBtn.containsMouse ? "#40FFFFFF" : "transparent"
                
                Text {
                    text: "鈴?
                    font.pixelSize: 20
                    color: "white"
                    anchors.centerIn: parent
                }
                
                MouseArea {
                    id: nextBtn
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.nextClicked()
                }
            }
        }
        
        Column {
            width: parent.width
            spacing: 6
            
            Item {
                width: parent.width
                height: 6
                
                Rectangle {
                    width: parent.width
                    height: 4
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 2
                    color: "#4DFFFFFF"
                }
                
                Rectangle {
                    width: parent.width * root.progress
                    height: 4
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 2
                    color: "white"
                }
                
                Rectangle {
                    width: 14
                    height: 14
                    radius: 7
                    x: Math.max(0, Math.min(parent.width - width, parent.width * root.progress - width/2))
                    anchors.verticalCenter: parent.verticalCenter
                    color: "white"
                    visible: progressMouseArea.containsMouse || progressMouseArea.pressed
                }
                
                MouseArea {
                    id: progressMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    
                    onClicked: {
                        var newProgress = mouse.x / width
                        root.seekToPosition(newProgress)
                    }
                    
                    onPositionChanged: {
                        if (pressed) {
                            var newProgress = Math.max(0, Math.min(1, mouse.x / width))
                            root.seekToPosition(newProgress)
                        }
                    }
                }
            }
            
            Row {
                width: parent.width
                
                Text {
                    text: formatTime(root.currentSeconds)
                    font.family: "Consolas"
                    font.pixelSize: 11
                    color: "#CCFFFFFF"
                }
                
                Item { width: parent.width - 100; height: 1 }
                
                Text {
                    text: formatTime(root.totalSeconds)
                    font.family: "Consolas"
                    font.pixelSize: 11
                    color: "#CCFFFFFF"
                }
            }
        }
    }
    
    Row {
        id: rightSection
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        spacing: 15
        
        Rectangle {
            width: 36
            height: 36
            radius: 18
            color: loopBtn.containsMouse ? "#40FFFFFF" : "transparent"
            anchors.verticalCenter: parent.verticalCenter
            
            Text {
                anchors.centerIn: parent
                text: root.loopMode === 0 ? "循" : root.loopMode === 1 ? "单" : "随"
                font.pixelSize: 16
            }
            
            MouseArea {
                id: loopBtn
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.loopModeToggled()
            }
        }
        
        Rectangle {
            width: 36
            height: 36
            radius: 18
            color: deskLrcBtn.containsMouse ? "#40FFFFFF" : "transparent"
            anchors.verticalCenter: parent.verticalCenter
            
            Text {
                anchors.centerIn: parent
                text: "词"
                font.family: "Microsoft YaHei"
                font.pixelSize: 14
                font.bold: true
                color: "white"
            }
            
            MouseArea {
                id: deskLrcBtn
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.desktopLrcToggled()
            }
        }
        
        Row {
            spacing: 10
            anchors.verticalCenter: parent.verticalCenter
            
            Text {
                text: root.volume > 0.5 ? "高" : root.volume > 0 ? "中" : "静"
                font.pixelSize: 16
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Item {
                width: 80
                height: 6
                anchors.verticalCenter: parent.verticalCenter
                
                Rectangle {
                    width: parent.width
                    height: 4
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 2
                    color: "#4DFFFFFF"
                }
                
                Rectangle {
                    width: parent.width * root.volume
                    height: 4
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 2
                    color: "white"
                }
                
                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    x: Math.max(0, Math.min(parent.width - width, parent.width * root.volume - width/2))
                    anchors.verticalCenter: parent.verticalCenter
                    color: "white"
                    visible: volumeMouseArea.containsMouse || volumeMouseArea.pressed
                }
                
                MouseArea {
                    id: volumeMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    
                    onClicked: {
                        var newVolume = mouse.x / width
                        root.volumeLevelChanged(newVolume)
                    }
                    
                    onPositionChanged: {
                        if (pressed) {
                            var newVolume = Math.max(0, Math.min(1, mouse.x / width))
                            root.volumeLevelChanged(newVolume)
                        }
                    }
                }
            }
        }
        
        Rectangle {
            width: 36
            height: 36
            radius: 18
            color: listBtn.containsMouse ? "#40FFFFFF" : "transparent"
            anchors.verticalCenter: parent.verticalCenter
            
            Text {
                anchors.centerIn: parent
                text: "鈽?
                font.pixelSize: 16
                color: "white"
            }
            
            MouseArea {
                id: listBtn
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.listToggled()
            }
        }
    }
    
    function formatTime(seconds) {
        var mins = Math.floor(seconds / 60)
        var secs = seconds % 60
        return mins + ":" + (secs < 10 ? "0" : "") + secs
    }
    
    function setPlayState(playing) {
        root.isPlaying = playing
    }
    
    function setProgress(current, total) {
        root.currentSeconds = current
        root.totalSeconds = total
        if (total > 0) {
            root.progress = current / total
        }
    }
    
    function setSongInfo(name, artistName, cover) {
        root.songName = name
        root.artist = artistName
        root.coverUrl = cover
    }
    
    function setVolume(vol) {
        root.volume = vol
    }
    
    function setLoopMode(mode) {
        root.loopMode = mode
    }
}

