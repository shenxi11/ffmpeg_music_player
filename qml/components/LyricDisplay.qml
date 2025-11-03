import QtQuick 2.14
import QtQuick.Controls 2.14

Item {
    id: root
    
    // 属性
    property int currentLine: -1
    property bool isUp: false
    
    // 信号
    signal currentLrcChanged(string lyricText)
    signal lyricClicked(int lineIndex)
    
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
        interactive: false
        
        // 高亮样式
        highlightMoveDuration: 200
        highlightMoveVelocity: -1
        preferredHighlightBegin: height / 2 - 30
        preferredHighlightEnd: height / 2 + 30
        highlightRangeMode: ListView.ApplyRange
        
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
                cursorShape: model.time && model.time !== "" ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
            
            Row {
                anchors.centerIn: parent
                width: parent.width - 40
                spacing: 20
                
                // 时间显示
                Text {
                    id: timeText
                    width: 80
                    text: model.time || ""
                    color: root.isUp ? "#CCCCCC" : "#666666"
                    font.pixelSize: lyricItem.isCurrent ? 14 : 12
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    visible: model.time && model.time !== ""
                    
                    // 字号动画
                    Behavior on font.pixelSize {
                        NumberAnimation { duration: 200 }
                    }
                }
                
                // 歌词文本
                Text {
                    id: lyricText
                    width: parent.width - (timeText.visible ? timeText.width + parent.spacing : 0)
                    text: model.text
                    color: root.isUp ? "#FFFFFF" : "#333333"
                    font.pixelSize: lyricItem.isCurrent ? 20 : 16
                    font.bold: lyricItem.isCurrent
                    horizontalAlignment: Text.AlignLeft
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
        
        // 滚动到当前行（ListView 会自动处理 preferredHighlightBegin）
        lyricView.currentIndex = lineNumber
        
        // 发射当前歌词文本
        var item = lyricModel.get(lineNumber)
        if (item && item.text.trim() !== "") {
            console.log("LyricDisplay.qml: emitting currentLrcChanged with:", item.text)
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