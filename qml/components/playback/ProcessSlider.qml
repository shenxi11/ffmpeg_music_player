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
    
    // 娓呯悊锛氬綋缁勪欢閿€姣佹椂锛岀‘淇濋煶閲忕獥鍙ｅ拰鎾斁妯″紡绐楀彛涔熷叧闂?
    Component.onDestruction: {
        if (volumeWindowLoader.active) {
            volumeWindowLoader.active = false
        }
        if (playModePopupLoader.active) {
            playModePopupLoader.active = false
        }
    }
    
    // 灞炴€?
    property int currentTime: 0
    property int maxTime: 0
    property bool sliderPressed: false
    property bool seekPending: false
    property int seekTargetTime: 0
    property double seekPendingSinceMs: 0
    property string songName: "\u6682\u65e0\u6b4c\u66f2"
    property string picPath: "qrc:/new/prefix1/icon/pian.png"
    
    // 鎾斁鎺у埗灞炴€э紙瀵瑰簲 ControlBar锛?
    property int playState: 0  // 0: Stop, 1: Play, 2: Pause
    property bool loopState: false
    property bool isUp: false
    property int volumeValue: 50
    property bool volumeVisible: false
    property bool mlistChecked: false
    property bool deskChecked: false
    property int playMode: 2  // 0: Sequential, 1: RepeatOne, 2: RepeatAll, 3: Shuffle
    // 是否已触发本地完成通知（已弃用，播放完成改由后端状态驱动）
    property bool finishNotified: false
    
    // 淇″彿
    signal seekTo(int seconds)
    signal sliderPressedSignal()  // 鏀瑰悕閬垮厤涓?sliderPressed 灞炴€у啿绐?
    signal sliderReleasedSignal() // 鏀瑰悕閬垮厤鍐茬獊
    signal upClicked()
    signal playFinished()  // 鎾斁瀹屾垚淇″彿
    signal stop()
    signal nextSong()
    signal lastSong()
    signal volumeChanged(int value)
    signal mlistToggled(bool checked)
    signal playClicked()
    signal rePlay()
    signal deskToggled(bool checked)
    signal loopToggled(bool isLooping)  // 鏀瑰弬鏁板悕閬垮厤鍐茬獊
    
    // 鍑芥暟
    function formatTime(seconds) {
        var mins = Math.floor(seconds / 60)
        var secs = seconds % 60
        return mins + ":" + (secs < 10 ? "0" : "") + secs
    }
    
    function setCurrentTime(seconds) {
        if (!sliderPressed) {
            if (seekPending) {
                var nowMs = Date.now()
                var drift = Math.abs(seconds - seekTargetTime)
                var reachedTarget = drift <= 1
                var backendTimelineDiff = drift >= 2
                var pendingTimeout = (nowMs - seekPendingSinceMs) >= 2500
                if (reachedTarget || backendTimelineDiff || pendingTimeout) {
                    seekPending = false
                } else {
                    root.currentTime = seekTargetTime
                    return
                }
            }

            if (seconds > root.maxTime) {
                // 元数据时长偏短时按档位扩容（每次至少+30s），避免进度条长期贴在末尾。
                var chunk = 30
                var target = Math.ceil(seconds / chunk) * chunk
                if (target <= root.maxTime) {
                    target = root.maxTime + chunk
                }
                root.maxTime = target
            }
            root.currentTime = seconds
        }
    }
    
    function setMaxTime(seconds) {
        root.maxTime = seconds
    }

    function setSeekPending(seconds) {
        root.seekTargetTime = Math.max(0, Math.floor(seconds))
        root.seekPendingSinceMs = Date.now()
        root.seekPending = true
        root.currentTime = root.seekTargetTime
        if (root.seekTargetTime > root.maxTime) {
            root.maxTime = root.seekTargetTime
        }
    }

    function clearSeekPending() {
        root.seekPending = false
    }
    
    function setSongName(name) {
        root.songName = name
    }
    
    function setPicPath(path) {
        root.picPath = path
    }

    function setVolumeValue(value) {
        var normalized = Math.max(0, Math.min(100, Math.round(value)))
        root.volumeValue = normalized
    }
    
    // 鎾斁鎺у埗鍑芥暟
    function setPlayState(state) {
        console.log("[ProcessSlider.qml] setPlayState called, state:", state, "old playState:", root.playState)
        root.playState = state
        if (state === 0) {
            root.seekPending = false
        }
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
            // 寰幆鎾斁 - 閲嶆柊鎾斁
            root.rePlay()
        } else {
            // 鍋滄鎾斁 - 浠呴噸缃?UI 鐘舵€侊紝涓嶈Е鍙?seek
            root.currentTime = 0  // 閲嶇疆杩涘害鏉″埌寮€澶?
            root.playState = 0    // Stop 鐘舵€?
            root.stop()           // 鍙戦€佸仠姝俊鍙?
            // 娉ㄦ剰锛氫笉璋冪敤 seekTo(0)锛岄伩鍏嶈Е鍙戞挱鏀?
            // 涓嬫鐐瑰嚮鎾斁鎸夐挳鏃讹紝浼氳嚜鍔ㄤ粠澶村紑濮?
        }
    }
    
    Rectangle {
        anchors.fill: parent
        // 璁剧疆涓洪€忔槑鑳屾櫙
        color: "transparent"
        
        Column {
            anchors.fill: parent
            spacing: 0
            
            // 杩涘害鏉″尯鍩?
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
                                root.setSeekPending(seekSeconds)
                                root.seekTo(seekSeconds)
                                root.sliderReleasedSignal()
                            }
                        }
                        
                        // 褰?currentTime 鏀瑰彉鏃舵洿鏂?slider锛堥櫎闈炵敤鎴锋鍦ㄦ嫋鍔級
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
                                color: "#EC4141"  // QQ闊充箰缁胯壊
                                radius: 2
                            }
                        }
                        
                        handle: Rectangle {
                            x: progressSlider.leftPadding + progressSlider.visualPosition * (progressSlider.availableWidth - width)
                            y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                            width: 12
                            height: 12
                            radius: 6
                            color: progressSlider.pressed ? "#EC4141" : "#FFFFFF"
                            border.color: "#EC4141"
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
            
            // 鎺у埗鏍忓尯鍩?
            Rectangle {
                width: parent.width
                height: 50
                color: "transparent"
                
                // 灞呬腑甯冨眬
                Item {
                    anchors.centerIn: parent
                    width: parent.width - 40
                    height: parent.height
                    
                    RowLayout {
                        anchors.fill: parent
                        spacing: 0
                        
                        // 宸︿晶锛氬皝闈?姝屾洸鍚?
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
                        
                        Item { Layout.fillWidth: true }  // 寮规€х┖闂?
                        
                        // 涓棿锛氭帶鍒舵寜閽粍
                        Row {
                            spacing: 20
                            Layout.alignment: Qt.AlignVCenter
                            
                            // 寰幆鎸夐挳 - 鏍规嵁妯″紡缁樺埗涓嶅悓鍥炬爣
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
                                        
                                        ctx.strokeStyle = hovered ? "#EC4141" : iconColor
                                        ctx.lineWidth = 2
                                        ctx.lineCap = "round"
                                        ctx.lineJoin = "round"
                                        
                                        // 鏍规嵁鎾斁妯″紡缁樺埗涓嶅悓鍥炬爣
                                        if (root.playMode === 0) {
                                            // Sequential - 椤哄簭鎾斁绠ご
                                            ctx.beginPath()
                                            ctx.moveTo(centerX - size/3, centerY - size/3)
                                            ctx.lineTo(centerX + size/3, centerY)
                                            ctx.lineTo(centerX - size/3, centerY + size/3)
                                            ctx.stroke()
                                        } else if (root.playMode === 1) {
                                            // RepeatOne - 鍗曟洸寰幆锛堝惊鐜澶?+ "1"锛?
                                            ctx.beginPath()
                                            ctx.arc(centerX, centerY, size/2.5, -Math.PI/4, Math.PI*5/4, false)
                                            ctx.stroke()
                                            ctx.beginPath()
                                            ctx.moveTo(centerX + size/2.5 - 2, centerY - size/2.5 + 2)
                                            ctx.lineTo(centerX + size/2.5, centerY - size/2.5)
                                            ctx.lineTo(centerX + size/2.5 + 2, centerY - size/2.5 + 2)
                                            ctx.stroke()
                                            ctx.font = "10px Arial"
                                            ctx.fillStyle = hovered ? "#EC4141" : iconColor
                                            ctx.textAlign = "center"
                                            ctx.textBaseline = "middle"
                                            ctx.fillText("1", centerX, centerY)
                                        } else if (root.playMode === 2) {
                                            // RepeatAll - 鍒楄〃寰幆锛堝弻绠ご寰幆锛?
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
                                            // Shuffle - 闅忔満鎾斁锛堜氦鍙夌澶达級
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
                                
                                // Loader 鐢ㄤ簬鍒涘缓鐙珛鎾斁妯″紡绐楀彛
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
                            
                            // 涓婁竴棣?- 缁樺埗涓婁竴鏇插浘鏍?
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
                                        
                                        ctx.fillStyle = hovered ? "#EC4141" : iconColor
                                        
                                        // 宸︿晶绔栫嚎
                                        ctx.fillRect(centerX - 10, centerY - 8, 2, 16)
                                        
                                        // 宸︿笁瑙掑舰
                                        ctx.beginPath()
                                        ctx.moveTo(centerX - 5, centerY)
                                        ctx.lineTo(centerX + 3, centerY - 8)
                                        ctx.lineTo(centerX + 3, centerY + 8)
                                        ctx.closePath()
                                        ctx.fill()
                                        
                                        // 鍙充笁瑙掑舰
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
                            
                            // 鎾斁/鏆傚仠鎸夐挳锛堜繚鎸佸師鏍凤級
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
                                    color: playMouseArea.containsMouse ? "#FF5757" : "#EC4141"
                                    
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
                            
                            // 涓嬩竴棣?- 缁樺埗涓嬩竴鏇插浘鏍?
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
                                        
                                        ctx.fillStyle = hovered ? "#EC4141" : iconColor
                                        
                                        // 宸︿笁瑙掑舰
                                        ctx.beginPath()
                                        ctx.moveTo(centerX - 11, centerY - 8)
                                        ctx.lineTo(centerX - 11, centerY + 8)
                                        ctx.lineTo(centerX - 3, centerY)
                                        ctx.closePath()
                                        ctx.fill()
                                        
                                        // 鍙充笁瑙掑舰
                                        ctx.beginPath()
                                        ctx.moveTo(centerX - 3, centerY - 8)
                                        ctx.lineTo(centerX - 3, centerY + 8)
                                        ctx.lineTo(centerX + 5, centerY)
                                        ctx.closePath()
                                        ctx.fill()
                                        
                                        // 鍙充晶绔栫嚎
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
                            
                            // 闊抽噺鎸夐挳 - 缁樺埗闊抽噺鍥炬爣
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
                                        
                                        ctx.fillStyle = hovered ? "#EC4141" : iconColor
                                        ctx.strokeStyle = hovered ? "#EC4141" : iconColor
                                        ctx.lineWidth = 2
                                        ctx.lineCap = "round"
                                        
                                        // 鎵０鍣?
                                        ctx.beginPath()
                                        ctx.moveTo(centerX - 8, centerY - 4)
                                        ctx.lineTo(centerX - 4, centerY - 4)
                                        ctx.lineTo(centerX + 2, centerY - 8)
                                        ctx.lineTo(centerX + 2, centerY + 8)
                                        ctx.lineTo(centerX - 4, centerY + 4)
                                        ctx.lineTo(centerX - 8, centerY + 4)
                                        ctx.closePath()
                                        ctx.fill()
                                        
                                        // 闊虫尝
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
                                
                                // Loader 鐢ㄤ簬鍒涘缓鐙珛闊抽噺绐楀彛
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
                        
                        Item { Layout.fillWidth: true }  // 寮规€х┖闂?
                        
                        // 鍙充晶锛氬姛鑳芥寜閽粍
                        Row {
                            spacing: 15
                            Layout.alignment: Qt.AlignVCenter
                            
                            // 妗岄潰姝岃瘝 - 缁樺埗妗岄潰绐楀彛+姝岃瘝鍥炬爣
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
                                        
                                        var color = hovered ? "#EC4141" : (root.deskChecked ? "#EC4141" : iconColor)
                                        ctx.strokeStyle = color
                                        ctx.fillStyle = color
                                        ctx.lineWidth = 2
                                        ctx.lineCap = "round"
                                        ctx.lineJoin = "round"
                                        
                                        // 缁樺埗鏄剧ず鍣ㄥ妗?
                                        ctx.strokeRect(centerX - 10, centerY - 8, 20, 14)
                                        
                                        // 缁樺埗搴曞骇
                                        ctx.beginPath()
                                        ctx.moveTo(centerX - 6, centerY + 6)
                                        ctx.lineTo(centerX - 6, centerY + 9)
                                        ctx.lineTo(centerX + 6, centerY + 9)
                                        ctx.lineTo(centerX + 6, centerY + 6)
                                        ctx.stroke()
                                        
                                        // 缁樺埗绐楀彛鍐呯殑姝岃瘝鏂囧瓧锛堜袱琛岋級
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
                            
                            // 鎾斁鍒楄〃 - 缁樺埗鍒楄〃鍥炬爣
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
                                        
                                        var color = hovered ? "#EC4141" : (root.mlistChecked ? "#EC4141" : iconColor)
                                        ctx.fillStyle = color
                                        
                                        // 缁樺埗涓夎鍒楄〃
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

