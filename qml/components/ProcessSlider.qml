import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Controls.impl 2.14
import QtQuick.Templates 2.14 as T
import QtGraphicalEffects 1.14
import QtQuick.Layouts 1.14

Item {
    id: root
    width: 1000
    height: 70
    
    // 清理：当组件销毁时，确保音量窗口和播放模式窗口也关闭
    Component.onDestruction: {
        if (volumeWindowLoader.active) {
            volumeWindowLoader.active = false
        }
        if (playModePopupLoader.active) {
            playModePopupLoader.active = false
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
    property int playMode: 2  // 0: Sequential, 1: RepeatOne, 2: RepeatAll, 3: Shuffle
    
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
    signal loopToggled(bool isLooping)  // 改参数名避免冲突
    
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
        // 设置为透明背景
        color: "transparent"
        
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
                        color: "#666666"
                        opacity: 0.9
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
                            opacity: 0.5
                            
                            Rectangle {
                                width: progressSlider.visualPosition * parent.width
                                height: parent.height
                                color: "#31C27C"  // QQ音乐绿色
                                radius: 2
                            }
                        }
                        
                        handle: Rectangle {
                            x: progressSlider.leftPadding + progressSlider.visualPosition * (progressSlider.availableWidth - width)
                            y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                            width: 12
                            height: 12
                            radius: 6
                            color: progressSlider.pressed ? "#31C27C" : "#FFFFFF"
                            border.color: "#31C27C"
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
                        color: "#666666"
                        opacity: 0.9
                    }
                }
            }
            
            // 控制栏区域
            Rectangle {
                width: parent.width
                height: 50
                color: "transparent"
                
                // 居中布局
                Item {
                    anchors.centerIn: parent
                    width: parent.width - 40
                    height: parent.height
                    
                    RowLayout {
                        anchors.fill: parent
                        spacing: 0
                        
                        // 左侧：封面+歌曲名
                        Rectangle {
                            Layout.preferredWidth: 250
                            Layout.fillHeight: true
                            color: "transparent"
                            
                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 12
                                
                                Image {
                                    id: coverImage
                                    width: 45
                                    height: 45
                                    source: root.picPath
                                    fillMode: Image.PreserveAspectCrop
                                    smooth: true
                                    layer.enabled: true
                                    layer.effect: OpacityMask {
                                        maskSource: Rectangle {
                                            width: coverImage.width
                                            height: coverImage.height
                                            radius: 4
                                        }
                                    }
                                }
                                
                                Text {
                                    width: 180
                                    height: 45
                                    text: root.songName
                                    verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 14
                                    font.weight: Font.Medium
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
                        
                        Item { Layout.fillWidth: true }  // 弹性空间
                        
                        // 中间：控制按钮组
                        Row {
                            spacing: 20
                            Layout.alignment: Qt.AlignVCenter
                            
                            // 循环按钮 - 根据模式绘制不同图标
                            Item {
                                width: 32
                                height: 32
                                
                                Canvas {
                                    id: loopCanvas
                                    anchors.fill: parent
                                    
                                    property color iconColor: root.isUp ? "#FFFFFF" : "#666666"
                                    property bool hovered: false
                                    
                                    onIconColorChanged: requestPaint()
                                    onHoveredChanged: requestPaint()
                                    
                                    Connections {
                                        target: root
                                        function onPlayModeChanged() {
                                            loopCanvas.requestPaint()
                                        }
                                    }
                                    
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        
                                        var centerX = width / 2
                                        var centerY = height / 2
                                        var size = 18
                                        
                                        ctx.strokeStyle = hovered ? "#31C27C" : iconColor
                                        ctx.lineWidth = 2
                                        ctx.lineCap = "round"
                                        ctx.lineJoin = "round"
                                        
                                        // 根据播放模式绘制不同图标
                                        if (root.playMode === 0) {
                                            // Sequential - 顺序播放箭头
                                            ctx.beginPath()
                                            ctx.moveTo(centerX - size/3, centerY - size/3)
                                            ctx.lineTo(centerX + size/3, centerY)
                                            ctx.lineTo(centerX - size/3, centerY + size/3)
                                            ctx.stroke()
                                        } else if (root.playMode === 1) {
                                            // RepeatOne - 单曲循环（循环箭头 + "1"）
                                            ctx.beginPath()
                                            ctx.arc(centerX, centerY, size/2.5, -Math.PI/4, Math.PI*5/4, false)
                                            ctx.stroke()
                                            ctx.beginPath()
                                            ctx.moveTo(centerX + size/2.5 - 2, centerY - size/2.5 + 2)
                                            ctx.lineTo(centerX + size/2.5, centerY - size/2.5)
                                            ctx.lineTo(centerX + size/2.5 + 2, centerY - size/2.5 + 2)
                                            ctx.stroke()
                                            ctx.font = "10px Arial"
                                            ctx.fillStyle = hovered ? "#31C27C" : iconColor
                                            ctx.textAlign = "center"
                                            ctx.textBaseline = "middle"
                                            ctx.fillText("1", centerX, centerY)
                                        } else if (root.playMode === 2) {
                                            // RepeatAll - 列表循环（双箭头循环）
                                            ctx.beginPath()
                                            ctx.arc(centerX, centerY, size/2, -Math.PI/4, Math.PI*5/4, false)
                                            ctx.stroke()
                                            ctx.beginPath()
                                            ctx.moveTo(centerX + size/2 - 3, centerY - size/2 + 3)
                                            ctx.lineTo(centerX + size/2, centerY - size/2)
                                            ctx.lineTo(centerX + size/2 + 3, centerY - size/2 + 3)
                                            ctx.stroke()
                                            ctx.beginPath()
                                            ctx.moveTo(centerX - size/2 - 3, centerY + size/2 - 3)
                                            ctx.lineTo(centerX - size/2, centerY + size/2)
                                            ctx.lineTo(centerX - size/2 + 3, centerY + size/2 - 3)
                                            ctx.stroke()
                                        } else {
                                            // Shuffle - 随机播放（交叉箭头）
                                            ctx.beginPath()
                                            ctx.moveTo(centerX - size/3, centerY - size/3)
                                            ctx.lineTo(centerX + size/3, centerY + size/3)
                                            ctx.moveTo(centerX + size/3, centerY - size/3)
                                            ctx.lineTo(centerX - size/3, centerY + size/3)
                                            ctx.stroke()
                                            ctx.beginPath()
                                            ctx.moveTo(centerX + size/3 - 4, centerY + size/3 - 4)
                                            ctx.lineTo(centerX + size/3, centerY + size/3)
                                            ctx.lineTo(centerX + size/3 + 4, centerY + size/3 - 4)
                                            ctx.stroke()
                                        }
                                    }
                                }
                                
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    hoverEnabled: true
                                    onEntered: loopCanvas.hovered = true
                                    onExited: loopCanvas.hovered = false
                                    onClicked: {
                                        if (playModePopupLoader.active) {
                                            if (playModePopupLoader.item) {
                                                playModePopupLoader.item.ignoreNextFocusLoss = true
                                            }
                                            playModePopupLoader.active = false
                                        } else {
                                            playModePopupLoader.active = true
                                        }
                                    }
                                }
                                
                                // Loader 用于创建独立播放模式窗口
                                Loader {
                                    id: playModePopupLoader
                                    active: false
                                    
                                    sourceComponent: Component {
                                        PlayModePopup {
                                            currentMode: root.playMode
                                            
                                            Component.onCompleted: {
                                                var buttonPos = loopCanvas.mapToGlobal(0, 0)
                                                x = buttonPos.x - (width - loopCanvas.width) / 2
                                                y = buttonPos.y - height - 10
                                                show()
                                                requestActivate()
                                            }
                                            
                                            onModeChanged: {
                                                root.playMode = mode
                                                loopCanvas.requestPaint()
                                            }
                                            
                                            onClosing: {
                                                playModePopupLoader.active = false
                                            }
                                        }
                                    }
                                }
                            }
                            
                            // 上一首 - 绘制上一曲图标
                            Item {
                                width: 32
                                height: 32
                                
                                Canvas {
                                    id: prevCanvas
                                    anchors.fill: parent
                                    property color iconColor: root.isUp ? "#FFFFFF" : "#666666"
                                    property bool hovered: false
                                    
                                    onIconColorChanged: requestPaint()
                                    onHoveredChanged: requestPaint()
                                    
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        
                                        var centerX = width / 2
                                        var centerY = height / 2
                                        
                                        ctx.fillStyle = hovered ? "#31C27C" : iconColor
                                        
                                        // 左侧竖线
                                        ctx.fillRect(centerX - 10, centerY - 8, 2, 16)
                                        
                                        // 左三角形
                                        ctx.beginPath()
                                        ctx.moveTo(centerX - 5, centerY)
                                        ctx.lineTo(centerX + 3, centerY - 8)
                                        ctx.lineTo(centerX + 3, centerY + 8)
                                        ctx.closePath()
                                        ctx.fill()
                                        
                                        // 右三角形
                                        ctx.beginPath()
                                        ctx.moveTo(centerX + 3, centerY)
                                        ctx.lineTo(centerX + 11, centerY - 8)
                                        ctx.lineTo(centerX + 11, centerY + 8)
                                        ctx.closePath()
                                        ctx.fill()
                                    }
                                }
                                
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    hoverEnabled: true
                                    onEntered: prevCanvas.hovered = true
                                    onExited: prevCanvas.hovered = false
                                    onClicked: root.lastSong()
                                }
                            }
                            
                            // 播放/暂停按钮（保持原样）
                            Rectangle {
                                width: 42
                                height: 42
                                color: "transparent"
                                radius: 21
                                
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 38
                                    height: 38
                                    radius: 19
                                    color: playMouseArea.containsMouse ? "#2ABD7C" : "#31C27C"
                                    
                                    Image {
                                        anchors.centerIn: parent
                                        width: 20
                                        height: 20
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
                            
                            // 下一首 - 绘制下一曲图标
                            Item {
                                width: 32
                                height: 32
                                
                                Canvas {
                                    id: nextCanvas
                                    anchors.fill: parent
                                    property color iconColor: root.isUp ? "#FFFFFF" : "#666666"
                                    property bool hovered: false
                                    
                                    onIconColorChanged: requestPaint()
                                    onHoveredChanged: requestPaint()
                                    
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        
                                        var centerX = width / 2
                                        var centerY = height / 2
                                        
                                        ctx.fillStyle = hovered ? "#31C27C" : iconColor
                                        
                                        // 左三角形
                                        ctx.beginPath()
                                        ctx.moveTo(centerX - 11, centerY - 8)
                                        ctx.lineTo(centerX - 11, centerY + 8)
                                        ctx.lineTo(centerX - 3, centerY)
                                        ctx.closePath()
                                        ctx.fill()
                                        
                                        // 右三角形
                                        ctx.beginPath()
                                        ctx.moveTo(centerX - 3, centerY - 8)
                                        ctx.lineTo(centerX - 3, centerY + 8)
                                        ctx.lineTo(centerX + 5, centerY)
                                        ctx.closePath()
                                        ctx.fill()
                                        
                                        // 右侧竖线
                                        ctx.fillRect(centerX + 8, centerY - 8, 2, 16)
                                    }
                                }
                                
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    hoverEnabled: true
                                    onEntered: nextCanvas.hovered = true
                                    onExited: nextCanvas.hovered = false
                                    onClicked: root.nextSong()
                                }
                            }
                            
                            // 音量按钮 - 绘制音量图标
                            Item {
                                width: 32
                                height: 32
                                
                                Canvas {
                                    id: volumeCanvas
                                    anchors.fill: parent
                                    property color iconColor: root.isUp ? "#FFFFFF" : "#666666"
                                    property bool hovered: false
                                    
                                    onIconColorChanged: requestPaint()
                                    onHoveredChanged: requestPaint()
                                    
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        
                                        var centerX = width / 2
                                        var centerY = height / 2
                                        
                                        ctx.fillStyle = hovered ? "#31C27C" : iconColor
                                        ctx.strokeStyle = hovered ? "#31C27C" : iconColor
                                        ctx.lineWidth = 2
                                        ctx.lineCap = "round"
                                        
                                        // 扬声器
                                        ctx.beginPath()
                                        ctx.moveTo(centerX - 8, centerY - 4)
                                        ctx.lineTo(centerX - 4, centerY - 4)
                                        ctx.lineTo(centerX + 2, centerY - 8)
                                        ctx.lineTo(centerX + 2, centerY + 8)
                                        ctx.lineTo(centerX - 4, centerY + 4)
                                        ctx.lineTo(centerX - 8, centerY + 4)
                                        ctx.closePath()
                                        ctx.fill()
                                        
                                        // 音波
                                        ctx.beginPath()
                                        ctx.arc(centerX + 2, centerY, 6, -Math.PI/4, Math.PI/4, false)
                                        ctx.stroke()
                                        
                                        ctx.beginPath()
                                        ctx.arc(centerX + 2, centerY, 10, -Math.PI/4, Math.PI/4, false)
                                        ctx.stroke()
                                    }
                                }
                                
                                MouseArea {
                                    id: volumeIconMouseArea
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    hoverEnabled: true
                                    onEntered: volumeCanvas.hovered = true
                                    onExited: volumeCanvas.hovered = false
                                    onClicked: {
                                        if (volumeWindowLoader.active) {
                                            if (volumeWindowLoader.item) {
                                                volumeWindowLoader.item.ignoreNextFocusLoss = true
                                            }
                                            volumeWindowLoader.active = false
                                        } else {
                                            volumeWindowLoader.active = true
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
                                                var buttonPos = volumeCanvas.mapToGlobal(0, 0)
                                                x = buttonPos.x - (width - volumeCanvas.width) / 2
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
                            }
                        }
                        
                        Item { Layout.fillWidth: true }  // 弹性空间
                        
                        // 右侧：功能按钮组
                        Row {
                            spacing: 15
                            Layout.alignment: Qt.AlignVCenter
                            
                            // 桌面歌词 - 绘制桌面窗口+歌词图标
                            Item {
                                width: 32
                                height: 32
                                
                                Canvas {
                                    id: lrcCanvas
                                    anchors.fill: parent
                                    property color iconColor: root.isUp ? "#FFFFFF" : "#666666"
                                    property bool hovered: false
                                    
                                    onIconColorChanged: requestPaint()
                                    onHoveredChanged: requestPaint()
                                    
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        
                                        var centerX = width / 2
                                        var centerY = height / 2
                                        
                                        var color = hovered ? "#31C27C" : (root.deskChecked ? "#31C27C" : iconColor)
                                        ctx.strokeStyle = color
                                        ctx.fillStyle = color
                                        ctx.lineWidth = 2
                                        ctx.lineCap = "round"
                                        ctx.lineJoin = "round"
                                        
                                        // 绘制显示器外框
                                        ctx.strokeRect(centerX - 10, centerY - 8, 20, 14)
                                        
                                        // 绘制底座
                                        ctx.beginPath()
                                        ctx.moveTo(centerX - 6, centerY + 6)
                                        ctx.lineTo(centerX - 6, centerY + 9)
                                        ctx.lineTo(centerX + 6, centerY + 9)
                                        ctx.lineTo(centerX + 6, centerY + 6)
                                        ctx.stroke()
                                        
                                        // 绘制窗口内的歌词文字（两行）
                                        ctx.lineWidth = 1.5
                                        ctx.fillRect(centerX - 7, centerY - 4, 10, 1.5)
                                        ctx.fillRect(centerX - 7, centerY + 1, 14, 1.5)
                                    }
                                }
                                
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    hoverEnabled: true
                                    onEntered: lrcCanvas.hovered = true
                                    onExited: lrcCanvas.hovered = false
                                    onClicked: {
                                        root.deskChecked = !root.deskChecked
                                        root.deskToggled(root.deskChecked)
                                        lrcCanvas.requestPaint()
                                    }
                                }
                            }
                            
                            // 播放列表 - 绘制列表图标
                            Item {
                                width: 32
                                height: 32
                                
                                Canvas {
                                    id: listCanvas
                                    anchors.fill: parent
                                    property color iconColor: root.isUp ? "#FFFFFF" : "#666666"
                                    property bool hovered: false
                                    
                                    onIconColorChanged: requestPaint()
                                    onHoveredChanged: requestPaint()
                                    
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        
                                        var centerX = width / 2
                                        var centerY = height / 2
                                        
                                        var color = hovered ? "#31C27C" : (root.mlistChecked ? "#31C27C" : iconColor)
                                        ctx.fillStyle = color
                                        
                                        // 绘制三行列表
                                        for (var i = 0; i < 3; i++) {
                                            var y = centerY - 7 + i * 7
                                            ctx.fillRect(centerX - 8, y, 16, 2)
                                        }
                                    }
                                }
                                
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    hoverEnabled: true
                                    onEntered: listCanvas.hovered = true
                                    onExited: listCanvas.hovered = false
                                    onClicked: {
                                        root.mlistChecked = !root.mlistChecked
                                        root.mlistToggled(root.mlistChecked)
                                        listCanvas.requestPaint()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
