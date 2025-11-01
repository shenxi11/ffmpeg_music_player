import QtQuick 2.14
import QtQuick.Controls 2.14
import QtGraphicalEffects 1.14

Item {
    id: root
    width: 1000
    height: 60
    
    // 清理：当组件销毁时，确保音量窗口也关闭
    Component.onDestruction: {
        if (volumeWindowLoader.active) {
            volumeWindowLoader.active = false
        }
    }
    
    // 属性
    property int currentTime: 0
    property int maxTime: 0
    property bool sliderPressed: false
    property string songName: "暂无歌曲"
    property string picPath: "qrc:/new/prefix1/icon/pian.png"
    
    // 播放控制属性（对应 ControlBar）
    property int playState: 0  // 0: Stop, 1: Play, 2: Pause
    property bool loopState: false
    property bool isUp: false
    property int volumeValue: 50
    property bool volumeVisible: false
    property bool mlistChecked: false
    property bool deskChecked: false
    
    // 信号
    signal seekTo(int seconds)
    signal sliderPressedSignal()  // 改名避免与 sliderPressed 属性冲突
    signal sliderReleasedSignal() // 改名避免冲突
    signal upClicked()
    signal playFinished()  // 播放完成信号
    signal stop()
    signal nextSong()
    signal lastSong()
    signal volumeChanged(int value)
    signal mlistToggled(bool checked)
    signal playClicked()
    signal rePlay()
    signal deskToggled(bool checked)
    signal loopToggled(bool loop)  // 改名避免与 loopState 属性的自动信号冲突
    
    // 函数
    function formatTime(seconds) {
        var mins = Math.floor(seconds / 60)
        var secs = seconds % 60
        return mins + ":" + (secs < 10 ? "0" : "") + secs
    }
    
    function setCurrentTime(seconds) {
        if (!sliderPressed) {
            root.currentTime = seconds
            // 检测播放是否完成
            if (seconds >= root.maxTime && root.maxTime > 0) {
                root.playFinished()
            }
        }
    }
    
    function setMaxTime(seconds) {
        root.maxTime = seconds
    }
    
    function setSongName(name) {
        root.songName = name
    }
    
    function setPicPath(path) {
        root.picPath = path
    }
    
    // 播放控制函数
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
    
    function handlePlayFinished() {
        console.log("handlePlayFinished called, loopState:", root.loopState)
        if (root.loopState) {
            // 循环播放 - 重新播放
            root.rePlay()
        } else {
            // 停止播放 - 仅重置 UI 状态，不触发 seek
            root.currentTime = 0  // 重置进度条到开头
            root.playState = 0    // Stop 状态
            root.stop()           // 发送停止信号
            // 注意：不调用 seekTo(0)，避免触发播放
            // 下次点击播放按钮时，会自动从头开始
        }
    }
    
    Rectangle {
        anchors.fill: parent
        // 根据展开状态切换背景色：展开时深色，收起时浅色
        color: root.isUp ? "#2C3E50" : "#FAFAFA"
        
        Column {
            anchors.fill: parent
            spacing: 0
            
            // 进度条区域
            Rectangle {
                width: parent.width
                height: 20
                color: "transparent"
                
                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 10
                    
                    Text {
                        width: 50
                        height: parent.height
                        text: root.formatTime(root.currentTime)
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignRight
                        font.pixelSize: 11
                        color: root.isUp ? "#FFFFFF" : "#666666"
                    }
                    
                    Slider {
                        id: progressSlider
                        width: parent.width - 120
                        height: parent.height
                        from: 0
                        to: root.maxTime > 0 ? root.maxTime : 100
                        value: root.currentTime
                        
                        onPressedChanged: {
                            root.sliderPressed = pressed
                            if (pressed) {
                                console.log("Slider pressed at value:", value, "maxTime:", root.maxTime)
                                root.sliderPressedSignal()
                            } else {
                                var seekSeconds = Math.floor(value)
                                console.log("Slider released at value:", value, "seeking to:", seekSeconds, "seconds")
                                root.seekTo(seekSeconds)
                                root.sliderReleasedSignal()
                            }
                        }
                        
                        // 当 currentTime 改变时更新 slider（除非用户正在拖动）
                        Connections {
                            target: root
                            function onCurrentTimeChanged() {
                                if (!progressSlider.pressed) {
                                    progressSlider.value = root.currentTime
                                }
                            }
                        }
                        
                        background: Rectangle {
                            x: progressSlider.leftPadding
                            y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                            width: progressSlider.availableWidth
                            height: 4
                            radius: 2
                            color: "#E0E0E0"
                            
                            Rectangle {
                                width: progressSlider.visualPosition * parent.width
                                height: parent.height
                                color: "#1DB954"
                                radius: 2
                            }
                        }
                        
                        handle: Rectangle {
                            x: progressSlider.leftPadding + progressSlider.visualPosition * (progressSlider.availableWidth - width)
                            y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                            width: 12
                            height: 12
                            radius: 6
                            color: progressSlider.pressed ? "#1DB954" : "#FFFFFF"
                            border.color: "#1DB954"
                            border.width: 2
                        }
                    }
                    
                    Text {
                        width: 50
                        height: parent.height
                        text: root.formatTime(root.maxTime)
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                        font.pixelSize: 11
                        color: root.isUp ? "#FFFFFF" : "#666666"
                    }
                }
            }
            
            // 控制栏区域
            Rectangle {
                width: parent.width
                height: 40
                // 添加半透明深色背景，在展开时增强对比度
                color: root.isUp ? "#80000000" : "transparent"
                
                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    spacing: 15
                    
                    // 封面+歌曲名
                    Rectangle {
                        width: 200
                        height: 40
                        color: "transparent"
                        
                        Row {
                            anchors.fill: parent
                            spacing: 8
                            
                            Image {
                                id: coverImage
                                width: 40
                                height: 40
                                source: root.picPath
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                            }
                            
                            Text {
                                width: 150
                                height: parent.height
                                text: root.songName
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 13
                                color: root.isUp ? "#FFFFFF" : "#333333"
                                elide: Text.ElideRight
                            }
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.upClicked()
                        }
                    }
                    
                    Item { width: 200; height: 1 }  // 间隔
                    
                    // 控制按钮组
                    Row {
                        spacing: 8
                        anchors.verticalCenter: parent.verticalCenter
                        
                        // 循环按钮
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
                                    (root.isUp ? "qrc:/new/prefix1/icon/loop_w.png" : "qrc:/new/prefix1/icon/loop.png") :
                                    (root.isUp ? "qrc:/new/prefix1/icon/random_play_w.png" : "qrc:/new/prefix1/icon/random_play.png")
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                hoverEnabled: true
                                onEntered: parent.color = root.isUp ? "#40FFFFFF" : "#E0E0E0"
                                onExited: parent.color = "transparent"
                                onClicked: {
                                    root.loopState = !root.loopState
                                    root.loopToggled(root.loopState)
                                }
                            }
                        }
                        
                        // 上一首
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
                                onEntered: parent.color = root.isUp ? "#40FFFFFF" : "#E0E0E0"
                                onExited: parent.color = "transparent"
                                onClicked: root.lastSong()
                            }
                        }
                        
                        // 播放/暂停
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
                                        if (root.playState === 1) {
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
                        
                        // 下一首
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
                                onEntered: parent.color = root.isUp ? "#40FFFFFF" : "#E0E0E0"
                                onExited: parent.color = "transparent"
                                onClicked: root.nextSong()
                            }
                        }
                        
                        // 音量按钮 + 独立窗口
                        Rectangle {
                            id: volumeButton
                            width: 28
                            height: 28
                            color: volumeIconMouseArea.containsMouse ? (root.isUp ? "#40FFFFFF" : "#E0E0E0") : "transparent"
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
                                id: volumeIconMouseArea
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                hoverEnabled: true
                                onClicked: {
                                    // 切换音量窗口
                                    if (volumeWindowLoader.active) {
                                        // 关闭窗口：设置忽略标志，防止焦点丢失重复触发
                                        if (volumeWindowLoader.item) {
                                            volumeWindowLoader.item.ignoreNextFocusLoss = true
                                        }
                                        volumeWindowLoader.active = false
                                    } else {
                                        // 打开窗口
                                        volumeWindowLoader.active = true
                                    }
                                }
                            }
                        }
                        
                        // Loader 用于创建独立音量窗口
                        Loader {
                            id: volumeWindowLoader
                            active: false
                            
                            sourceComponent: Component {
                                VolumeSlider {
                                    volumeValue: root.volumeValue
                                    
                                    Component.onCompleted: {
                                        // 计算全局位置
                                        var buttonPos = volumeButton.mapToGlobal(0, 0)
                                        x = buttonPos.x - (width - volumeButton.width) / 2
                                        y = buttonPos.y - height - 10
                                        show()
                                        requestActivate()
                                    }
                                    
                                    onVolumeChanged: {
                                        root.volumeValue = value
                                        root.volumeChanged(value)
                                    }
                                    
                                    onClosing: {
                                        volumeWindowLoader.active = false
                                    }
                                }
                            }
                        }
                        
                        // 桌面歌词
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
                                onEntered: parent.color = root.isUp ? "#40FFFFFF" : "#E0E0E0"
                                onExited: parent.color = "transparent"
                                onClicked: {
                                    root.deskChecked = !root.deskChecked
                                    root.deskToggled(root.deskChecked)
                                }
                            }
                        }
                        
                        // 播放列表
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
                                onEntered: parent.color = root.isUp ? "#40FFFFFF" : "#E0E0E0"
                                onExited: parent.color = "transparent"
                                onClicked: {
                                    root.mlistChecked = !root.mlistChecked
                                    root.mlistToggled(root.mlistChecked)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
