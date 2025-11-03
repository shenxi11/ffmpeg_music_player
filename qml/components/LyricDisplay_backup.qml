import QtQuick 2.14
import QtQuick.Controls 2.14
import QtGraphicalEffects 1.14

Item {
    id: root
    
    // 属性
    property int currentLine: -1
    property bool isUp: false
    property int userScrolledIndex: -1  // 用户滚动到的行
    property bool isUserScrolling: false  // 是否正在用户滚动模式
    
    // 信号
    signal currentLrcChanged(string lyricText)
    signal lyricClicked(int lineIndex)
    signal dragStarted(int lineIndex)
    signal dragMoved(int lineIndex)
    signal dragEnded(int lineIndex)

    signal playButtonClicked(int lineIndex)  // 播放按钮点击信号
    
    // 恢复定时器
    Timer {
        id: restoreTimer
        interval: 5000  // 5秒
        onTriggered: {
            root.isUserScrolling = false
            root.userScrolledIndex = -1
            // 恢复到当前高亮行
            lyricView.positionViewAtIndex(root.currentLine, ListView.Center)
        }
    }
    
    // 歌词模型
    ListModel {
        id: lyricModel
    }
    
    // 歌词 ListView
    ListView {
        id: lyricView
        anchors.fill: parent
        model: lyricModel
        clip: true
        
        // 禁用滚动条
        ScrollBar.vertical: ScrollBar {
            visible: false
        }
        
        // 启用交互以支持滚轮
        interactive: true
        
        // 添加滚轮支持
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton  // 不处理点击，只处理滚轮
            
            property bool isWheelScrolling: false
            
            onWheel: {
                var delta = wheel.angleDelta.y
                var sensitivity = 120  // 滚轮灵敏度
                
                if (Math.abs(delta) >= sensitivity) {
                    // 如果是第一次滚动
                    if (!isWheelScrolling) {
                        isWheelScrolling = true
                    }
                    
                    var direction = delta > 0 ? -1 : 1  // 向上滚动减少索引，向下滚动增加索引
                    var currentIndex = lyricView.indexAt(lyricView.contentX, lyricView.contentY + lyricView.height / 2)
                    var newIndex = Math.max(5, Math.min(currentIndex + direction, lyricModel.count - 10))
                    
                    if (newIndex !== currentIndex && newIndex >= 5 && newIndex < lyricModel.count - 5) {
                        // 设置用户滚动状态
                        root.isUserScrolling = true
                        root.userScrolledIndex = newIndex
                        
                        // 滚动到指定位置
                        lyricView.positionViewAtIndex(newIndex, ListView.Center)
                        
                        // 重新启动恢复定时器
                        restoreTimer.restart()
                    }
                    
                    // 重置定时器
                    scrollEndTimer.restart()
                }
            }
            
            Timer {
                id: scrollEndTimer
                interval: 300  // 300ms 无滚动则认为结束
                onTriggered: {
                    if (parent.isWheelScrolling) {
                        parent.isWheelScrolling = false
                    }
                }
            }
        }
        
        // 当前项居中显示（在视口上1/3处）
        preferredHighlightBegin: height / 3
        preferredHighlightEnd: height / 3
        highlightRangeMode: ListView.StrictlyEnforceRange
        highlightMoveDuration: 300  // 平滑滚动动画
        
        delegate: Item {
            id: lyricItem
            width: lyricView.width
            height: Math.max(lyricText.height + 20, 40)
            
            property bool isCurrent: index === root.currentLine
            
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    console.log("Lyric clicked at index:", index)
                    root.lyricClicked(index)
                }
                
                onWheel: {
                    root.userScrolling = true
                    if (restoreTimer) {
                        restoreTimer.restart()
                    }
                    
                    // 计算中心行
                    var centerY = lyricView.height / 2
                    var centerIndex = Math.round((lyricView.contentY + centerY) / 60) // 假设每行60像素
                    root.updateCenterIndex(centerIndex)
                }
            }
            
            // 简单的歌词文本显示
            Text {
                id: lyricText
                anchors.centerIn: parent
                width: parent.width - 40
                text: model.text
                color: root.isUp ? "#FFFFFF" : "#333333"
                font.pixelSize: lyricItem.isCurrent ? 20 : 16
                font.bold: lyricItem.isCurrent
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                
                // 字号和透明度动画
                Behavior on font.pixelSize {
                    NumberAnimation { duration: 200 }
                }
                
                Behavior on opacity {
                    NumberAnimation { duration: 200 }
                }
                
                opacity: {
                    if (lyricItem.isCurrent) return 1.0
                    var distance = Math.abs(index - root.currentLine)
                    if (distance <= 1) return 0.6
                    if (distance <= 2) return 0.4
                    return 0.3
                }
            }
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                
                // 第一列：播放按钮
                Item {
                    width: 50
                    height: parent.height
                    
                    Rectangle {
                        id: playButton
                        anchors.centerIn: parent
                        width: 32
                        height: 32
                        radius: 16
                        color: playButtonMouseArea.containsMouse ? Qt.rgba(0.1, 0.7, 0.3, 0.9) : Qt.rgba(0.1, 0.6, 0.3, 0.8)
                        border.color: Qt.rgba(1, 1, 1, 0.6)
                        border.width: 1.5
                        visible: root.isUserScrolling && model.time && model.time !== ""
                        
                        // 播放图标（三角形）
                        Canvas {
                            anchors.centerIn: parent
                            width: 12
                            height: 12
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.fillStyle = "#FFFFFF"
                                ctx.beginPath()
                                ctx.moveTo(3, 1)
                                ctx.lineTo(11, 6)
                                ctx.lineTo(3, 11)
                                ctx.closePath()
                                ctx.fill()
                            }
                        }
                        
                        // 缩放动画
                        scale: playButtonMouseArea.pressed ? 0.85 : 1.0
                        Behavior on scale {
                            NumberAnimation { duration: 100 }
                        }
                        
                        // 透明度动画
                        Behavior on opacity {
                            NumberAnimation { duration: 200 }
                        }
                        
                        MouseArea {
                            id: playButtonMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                console.log("Play button clicked for line:", index, "time:", model.time)
                                // 立即隐藏按钮和恢复状态
                                root.isUserScrolling = false
                                root.restoreTimer.stop()
                                root.playButtonClicked(index)
                                
                                // 滚动回到当前播放行
                                if (root.currentLine >= 0) {
                                    lyricView.positionViewAtIndex(root.currentLine, ListView.Center)
                                }
                            }
                        }
                    }
                }
                
                // 第二列：时间显示
                Item {
                    width: 65
                    height: parent.height
                    
                    Text {
                        id: timeText
                        anchors.centerIn: parent
                        text: model.time || ""
                        color: {
                            if (root.isUserScrolling && model.time && model.time !== "") {
                                return root.isUp ? "#FFFFFF" : "#333333"
                            } else {
                                return root.isUp ? "#AAAAAA" : "#888888"
                            }
                        }
                        font.pixelSize: 11
                        font.family: "Consolas, Monaco, monospace"
                        horizontalAlignment: Text.AlignCenter
                        visible: model.time && model.time !== ""
                        
                        // 颜色动画
                        Behavior on color {
                            ColorAnimation { duration: 200 }
                        }
                    }
                }
                
                // 第三列：歌词文本
                Item {
                    width: parent.width - 50 - 65 - 20  // 减去前两列宽度和间距
                    height: parent.height
                    
                    Text {
                        id: lyricText
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                        text: model.text
                        color: {
                            if (lyricItem.isCurrent) {
                                return root.isUp ? "#FFFFFF" : "#333333"
                            } else {
                                return root.isUp ? "#BBBBBB" : "#777777"
                            }
                        }
                        font.pixelSize: lyricItem.isCurrent ? 18 : 16
                        font.bold: lyricItem.isCurrent
                        horizontalAlignment: Text.AlignLeft
                        wrapMode: Text.WordWrap
                        
                        // 字号动画
                        Behavior on font.pixelSize {
                            NumberAnimation { duration: 200 }
                        }
                        
                        // 字体粗细动画
                        Behavior on font.bold {
                            PropertyAnimation { duration: 200 }
                        }
                        
                        // 颜色动画
                        Behavior on color {
                            ColorAnimation { duration: 200 }
                        }
                        
                        // 透明度基于距离当前行的远近
                        opacity: {
                            if (lyricItem.isCurrent) return 1.0
                            var distance = Math.abs(index - root.currentLine)
                            if (distance <= 1) return 0.8
                            if (distance <= 2) return 0.6
                            return 0.4
                        }
                        
                        Behavior on opacity {
                            NumberAnimation { duration: 200 }
                        }
                    }
                }
            }
        }
    }
    
    // 空列表提示
    Text {
        anchors.centerIn: parent
        text: "暂无歌词"
        color: root.isUp ? "#888888" : "#AAAAAA"
        font.pixelSize: 16
        visible: lyricModel.count === 0
    }
    
    // 函数：清空歌词
    function clearLyrics() {
        lyricModel.clear()
        root.currentLine = -1
    }
    
    // 函数：添加歌词列表（前后加空行）
    function setLyrics(lyricsArray) {
        lyricModel.clear()
        
        // 添加前面5行空行
        for (var i = 0; i < 5; i++) {
            lyricModel.append({ "text": " ", "time": "" })
        }
        
        // 添加歌词内容
        for (var j = 0; j < lyricsArray.length; j++) {
            var item = lyricsArray[j]
            if (typeof item === 'string') {
                // 兼容旧格式（纯字符串数组）
                lyricModel.append({ "text": item, "time": "" })
            } else {
                // 新格式（带时间的对象）
                lyricModel.append({ 
                    "text": item.text || "",
                    "time": item.time || ""
                })
            }
        }
        
        // 添加后面9行空行
        for (var k = 0; k < 9; k++) {
            lyricModel.append({ "text": " ", "time": "" })
        }
        
        // 重置当前行为第一行歌词（索引5）
        root.currentLine = 5
    }
    
    // 函数：高亮指定行
    function highlightLine(lineNumber) {
        if (lineNumber < 0 || lineNumber >= lyricModel.count) {
            return
        }
        
        root.currentLine = lineNumber
        
        // 只有在非用户滚动状态下才自动滚动到高亮行
        if (!root.isUserScrolling) {
            lyricView.positionViewAtIndex(lineNumber, ListView.Center)
        }
        
        // 发射当前歌词文本
        var item = lyricModel.get(lineNumber)
        if (item && item.text.trim() !== "") {
            root.currentLrcChanged(item.text)
        }
    }
    
    // 函数：滚动到指定行
    function scrollToLine(lineNumber) {
        if (lineNumber < 0 || lineNumber >= lyricModel.count) {
            return
        }
        lyricView.positionViewAtIndex(lineNumber, ListView.Center)
    }
    
    // 函数：设置是否展开状态
    function setIsUp(up) {
        root.isUp = up
    }
}
