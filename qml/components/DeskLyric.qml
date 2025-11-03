import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.14

Item {
    id: root
    width: 500
    height: 120
    
    // 属性
    property string lyricText: "♪ 暂无歌词 ♪"
    property string songName: "暂无歌曲"
    property color lyricColor: "#ffffff"
    property int lyricFontSize: 18
    property string lyricFontFamily: "Microsoft YaHei"
    property bool hovered: false
    property bool isDragging: false
    property bool isResizing: false
    property bool isPlaying: false  // 播放状态
    
    // 最小和最大尺寸
    readonly property int minWidth: 250
    readonly property int minHeight: 100
    readonly property int maxWidth: 1600
    readonly property int maxHeight: 400
    
    // 信号
    signal playClicked(int state)
    signal nextClicked()
    signal lastClicked()
    signal forwardClicked()
    signal backwardClicked()
    signal settingsClicked()
    signal closeClicked()
    
    // 移动和调整大小相关信号
    signal windowMoved(var newPos)
    signal windowResized(var newSize)
    
    // 主容器 - 完全透明
    Item {
        id: mainContainer
        anchors.fill: parent
        
        // 控制栏
        Rectangle {
            id: controlBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 35
            visible: root.hovered
            opacity: root.hovered ? 1.0 : 0.0
            
            // 渐变背景
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(0.16, 0.16, 0.16, 0.78) }
                GradientStop { position: 1.0; color: Qt.rgba(0.08, 0.08, 0.08, 0.86) }
            }
            
            radius: 15
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.2)
            
            Behavior on opacity {
                NumberAnimation { duration: 200 }
            }
            
            Row {
                anchors.centerIn: parent
                spacing: 8
                
                // 上一曲按钮
                Button {
                    width: 24
                    height: 24
                    text: "⏮"
                    font.pixelSize: 12
                    
                    background: Rectangle {
                        color: parent.pressed ? Qt.rgba(1, 1, 1, 0.3) : 
                               parent.hovered ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                        radius: 4
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: "#ffffff"
                        font: parent.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: root.lastClicked()
                }
                
                // 快退按钮
                Button {
                    width: 24
                    height: 24
                    text: "⏪"
                    font.pixelSize: 12
                    
                    background: Rectangle {
                        color: parent.pressed ? Qt.rgba(1, 1, 1, 0.3) : 
                               parent.hovered ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                        radius: 4
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: "#ffffff"
                        font: parent.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: root.backwardClicked()
                }
                
                // 播放/暂停按钮
                Button {
                    id: playButton
                    width: 28
                    height: 28
                    text: root.isPlaying ? "⏸" : "▶"  // 根据播放状态显示不同图标
                    font.pixelSize: 14
                    
                    background: Rectangle {
                        color: parent.pressed ? Qt.rgba(1, 1, 1, 0.3) : 
                               parent.hovered ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                        radius: 4
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: "#ffffff"
                        font: parent.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: {
                        console.log("Play button clicked in QML, current isPlaying:", root.isPlaying)
                        // 切换播放状态并发送信号
                        root.isPlaying = !root.isPlaying
                        console.log("New isPlaying state:", root.isPlaying)
                        root.playClicked(root.isPlaying ? 1 : 0)
                    }
                }
                
                // 快进按钮
                Button {
                    width: 24
                    height: 24
                    text: "⏩"
                    font.pixelSize: 12
                    
                    background: Rectangle {
                        color: parent.pressed ? Qt.rgba(1, 1, 1, 0.3) : 
                               parent.hovered ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                        radius: 4
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: "#ffffff"
                        font: parent.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: root.forwardClicked()
                }
                
                // 下一曲按钮
                Button {
                    width: 24
                    height: 24
                    text: "⏭"
                    font.pixelSize: 12
                    
                    background: Rectangle {
                        color: parent.pressed ? Qt.rgba(1, 1, 1, 0.3) : 
                               parent.hovered ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                        radius: 4
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: "#ffffff"
                        font: parent.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: root.nextClicked()
                }
                
                // 设置按钮
                Button {
                    width: 24
                    height: 24
                    text: "⚙"
                    font.pixelSize: 12
                    
                    background: Rectangle {
                        color: parent.pressed ? Qt.rgba(1, 1, 1, 0.3) : 
                               parent.hovered ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                        radius: 4
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: "#ffffff"
                        font: parent.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: root.settingsClicked()
                }
                
                // 关闭按钮
                Button {
                    width: 24
                    height: 24
                    text: "✕"
                    font.pixelSize: 12
                    
                    background: Rectangle {
                        color: parent.pressed ? Qt.rgba(1, 0, 0, 0.6) : 
                               parent.hovered ? Qt.rgba(1, 0, 0, 0.4) : "transparent"
                        radius: 4
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: "#ffffff"
                        font: parent.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: root.closeClicked()
                }
            }
        }
        
        // 歌词标签
        Rectangle {
            id: lyricBackground
            anchors.top: controlBar.visible ? controlBar.bottom : parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.topMargin: controlBar.visible ? 5 : 0
            anchors.margins: 10
            
            // 渐变背景
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.39) }
                GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.59) }
            }
            
            radius: 20
            border.width: 2
            border.color: Qt.rgba(1, 1, 1, 0.1)
            
            ColumnLayout {
                id: lyricColumn
                anchors.fill: parent
                anchors.margins: 25
                spacing: 10
                
                // 歌曲名标签
                Text {
                    id: songNameLabel
                    width: parent.width
                    text: root.songName || "正在播放..."
                    color: Qt.lighter(root.lyricColor, 1.2)
                    font.family: root.lyricFontFamily
                    font.pixelSize: Math.max(root.lyricFontSize - 8, 12)
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    opacity: 0.9
                }
                
                // 分隔线
                Rectangle {
                    width: parent.width * 0.8
                    height: 1
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: Qt.rgba(1, 1, 1, 0.3)
                    visible: root.songName !== ""
                }
                
                // 歌词标签
                Text {
                    id: lyricLabel
                    width: parent.width
                    Layout.fillHeight: true
                    text: root.lyricText
                    color: root.lyricColor
                    font.family: root.lyricFontFamily
                    font.pixelSize: root.lyricFontSize
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                }
            }
        }
        
        // 歌词背景阴影效果
        DropShadow {
            anchors.fill: lyricBackground
            source: lyricBackground
            radius: 16
            samples: 33
            color: Qt.rgba(0, 0, 0, 0.6)
            horizontalOffset: 0
            verticalOffset: 4
        }
        
        // 悬停背景效果
        Rectangle {
            id: hoverBackground
            anchors.fill: parent
            anchors.margins: 5
            visible: root.hovered
            opacity: root.hovered ? 1.0 : 0.0
            color: "transparent"
            
            // 渐变背景
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(0.16, 0.16, 0.16, 0.71) }
                GradientStop { position: 0.5; color: Qt.rgba(0.24, 0.24, 0.24, 0.63) }
                GradientStop { position: 1.0; color: Qt.rgba(0.08, 0.08, 0.08, 0.78) }
            }
            
            radius: 15
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.2)
            
            Behavior on opacity {
                NumberAnimation { duration: 200 }
            }
        }
        
        // 内发光效果
        Rectangle {
            anchors.fill: hoverBackground
            anchors.margins: 1
            visible: root.hovered
            color: "transparent"
            radius: 14
            border.width: 2
            border.color: Qt.rgba(0.4, 0.49, 0.92, 0.39)
            
            opacity: root.hovered ? 1.0 : 0.0
            Behavior on opacity {
                NumberAnimation { duration: 200 }
            }
        }
        
        // 拉伸手柄
        Rectangle {
            id: resizeHandle
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: 10
            height: 10
            visible: root.hovered
            color: Qt.rgba(0.4, 0.49, 0.92, 0.2)
            border.width: 2
            border.color: Qt.rgba(0.4, 0.49, 0.92, 0.59)
            
            // 拉伸手柄线条
            Canvas {
                anchors.fill: parent
                onPaint: {
                    var ctx = getContext("2d");
                    ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.78);
                    ctx.lineWidth = 1;
                    
                    for (var i = 0; i < 3; i++) {
                        var offset = i * 3 + 2;
                        ctx.beginPath();
                        ctx.moveTo(offset, parent.height - 2);
                        ctx.lineTo(parent.width - 2, offset);
                        ctx.stroke();
                    }
                }
            }
        }
        
        // 悬停检测区域（拖拽由C++处理）
        MouseArea {
            anchors.fill: parent
            anchors.rightMargin: root.hovered ? 10 : 0
            anchors.bottomMargin: root.hovered ? 10 : 0
            
            hoverEnabled: true
            acceptedButtons: Qt.NoButton  // 不接受按钮，让C++处理拖拽
            
            onEntered: {
                root.hovered = true
            }
            
            onExited: {
                if (!resizeMouseArea.containsMouse) {
                    root.hovered = false
                }
            }
        }
        
        // 拉伸鼠标区域
        MouseArea {
            id: resizeMouseArea
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: 15
            height: 15
            visible: root.hovered
            
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.SizeFDiagCursor
            
            property point startPos
            property size startSize
            
            onEntered: {
                root.hovered = true
            }
            
            onPressed: {
                if (mouse.button === Qt.LeftButton) {
                    root.isResizing = true
                    startPos = Qt.point(mouse.x, mouse.y)
                    startSize = Qt.size(root.width, root.height)
                }
            }
            
            onPositionChanged: {
                if (root.isResizing) {
                    var deltaX = mouse.x - startPos.x
                    var deltaY = mouse.y - startPos.y
                    
                    var newWidth = Math.max(root.minWidth, 
                                  Math.min(root.maxWidth, startSize.width + deltaX))
                    var newHeight = Math.max(root.minHeight, 
                                   Math.min(root.maxHeight, startSize.height + deltaY))
                    
                    root.windowResized(Qt.size(newWidth, newHeight))
                }
            }
            
            onReleased: {
                root.isResizing = false
            }
        }
    }
    
    // 公开函数
    function setLyricText(text) {
        console.log("DeskLyric.qml: setLyricText called with:", text)
        root.lyricText = text
    }
    
    function setSongName(name) {
        console.log("DeskLyric.qml: setSongName called with:", name)
        root.songName = name
    }
    
    function setPlayButtonState(isPlaying) {
        playButton.text = isPlaying ? "⏸" : "▶"
    }
    
    function setLyricStyle(color, fontSize, fontFamily) {
        root.lyricColor = color
        root.lyricFontSize = fontSize
        root.lyricFontFamily = fontFamily
    }
    
    function showSettingsDialog() {
        settingsLoader.active = true
    }
    
    // 设置对话框加载器
    Loader {
        id: settingsLoader
        active: false
        source: "qrc:/qml/components/DeskLyricSettings.qml"
        
        onLoaded: {
            if (item) {
                // 设置当前值 - 使用正确的属性名
                item.currentColor = root.lyricColor
                item.currentFontSize = root.lyricFontSize
                item.currentFontFamily = root.lyricFontFamily
                
                // 连接信号
                item.settingsChanged.connect(function(color, fontSize, fontFamily) {
                    root.setLyricStyle(color, fontSize, fontFamily)
                })
                
                item.dialogClosed.connect(function() {
                    settingsLoader.active = false
                })
                
                // 显示对话框
                item.show()
            }
        }
    }
}