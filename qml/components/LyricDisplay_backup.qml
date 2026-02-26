import QtQuick 2.14
import QtQuick.Controls 2.14
import QtGraphicalEffects 1.14

Item {
    id: root
    
    // 灞炴€?
    property int currentLine: -1
    property bool isUp: false
    property int userScrolledIndex: -1  // 鐢ㄦ埛婊氬姩鍒扮殑琛?
    property bool isUserScrolling: false  // 鏄惁姝ｅ湪鐢ㄦ埛婊氬姩妯″紡
    
    // 淇″彿
    signal currentLrcChanged(string lyricText)
    signal lyricClicked(int lineIndex)
    signal dragStarted(int lineIndex)
    signal dragMoved(int lineIndex)
    signal dragEnded(int lineIndex)

    signal playButtonClicked(int lineIndex)  // 鎾斁鎸夐挳鐐瑰嚮淇″彿
    
    // 鎭㈠瀹氭椂鍣?
    Timer {
        id: restoreTimer
        interval: 5000  // 5绉?
        onTriggered: {
            root.isUserScrolling = false
            root.userScrolledIndex = -1
            // 鎭㈠鍒板綋鍓嶉珮浜
            lyricView.positionViewAtIndex(root.currentLine, ListView.Center)
        }
    }
    
    // 姝岃瘝妯″瀷
    ListModel {
        id: lyricModel
    }
    
    // 姝岃瘝 ListView
    ListView {
        id: lyricView
        anchors.fill: parent
        model: lyricModel
        clip: true
        
        // 绂佺敤婊氬姩鏉?
        ScrollBar.vertical: ScrollBar {
            visible: false
        }
        
        // 鍚敤浜や簰浠ユ敮鎸佹粴杞?
        interactive: true
        
        // 娣诲姞婊氳疆鏀寔
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton  // 涓嶅鐞嗙偣鍑伙紝鍙鐞嗘粴杞?
            
            property bool isWheelScrolling: false
            
            onWheel: {
                var delta = wheel.angleDelta.y
                var sensitivity = 120  // 婊氳疆鐏垫晱搴?
                
                if (Math.abs(delta) >= sensitivity) {
                    // 濡傛灉鏄涓€娆℃粴鍔?
                    if (!isWheelScrolling) {
                        isWheelScrolling = true
                    }
                    
                    var direction = delta > 0 ? -1 : 1  // 鍚戜笂婊氬姩鍑忓皯绱㈠紩锛屽悜涓嬫粴鍔ㄥ鍔犵储寮?
                    var currentIndex = lyricView.indexAt(lyricView.contentX, lyricView.contentY + lyricView.height / 2)
                    var newIndex = Math.max(5, Math.min(currentIndex + direction, lyricModel.count - 10))
                    
                    if (newIndex !== currentIndex && newIndex >= 5 && newIndex < lyricModel.count - 5) {
                        // 璁剧疆鐢ㄦ埛婊氬姩鐘舵€?
                        root.isUserScrolling = true
                        root.userScrolledIndex = newIndex
                        
                        // 婊氬姩鍒版寚瀹氫綅缃?
                        lyricView.positionViewAtIndex(newIndex, ListView.Center)
                        
                        // 閲嶆柊鍚姩鎭㈠瀹氭椂鍣?
                        restoreTimer.restart()
                    }
                    
                    // 閲嶇疆瀹氭椂鍣?
                    scrollEndTimer.restart()
                }
            }
            
            Timer {
                id: scrollEndTimer
                interval: 300  // 300ms 鏃犳粴鍔ㄥ垯璁や负缁撴潫
                onTriggered: {
                    if (parent.isWheelScrolling) {
                        parent.isWheelScrolling = false
                    }
                }
            }
        }
        
        // 褰撳墠椤瑰眳涓樉绀猴紙鍦ㄨ鍙ｄ笂1/3澶勶級
        preferredHighlightBegin: height / 3
        preferredHighlightEnd: height / 3
        highlightRangeMode: ListView.StrictlyEnforceRange
        highlightMoveDuration: 300  // 骞虫粦婊氬姩鍔ㄧ敾
        
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
                    
                    // 璁＄畻涓績琛?
                    var centerY = lyricView.height / 2
                    var centerIndex = Math.round((lyricView.contentY + centerY) / 60) // 鍋囪姣忚60鍍忕礌
                    root.updateCenterIndex(centerIndex)
                }
            }
            
            // 绠€鍗曠殑姝岃瘝鏂囨湰鏄剧ず
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
                
                // 瀛楀彿鍜岄€忔槑搴﹀姩鐢?
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
                
                // 绗竴鍒楋細鎾斁鎸夐挳
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
                        
                        // 鎾斁鍥炬爣锛堜笁瑙掑舰锛?
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
                        
                        // 缂╂斁鍔ㄧ敾
                        scale: playButtonMouseArea.pressed ? 0.85 : 1.0
                        Behavior on scale {
                            NumberAnimation { duration: 100 }
                        }
                        
                        // 閫忔槑搴﹀姩鐢?
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
                                // 绔嬪嵆闅愯棌鎸夐挳鍜屾仮澶嶇姸鎬?
                                root.isUserScrolling = false
                                root.restoreTimer.stop()
                                root.playButtonClicked(index)
                                
                                // 婊氬姩鍥炲埌褰撳墠鎾斁琛?
                                if (root.currentLine >= 0) {
                                    lyricView.positionViewAtIndex(root.currentLine, ListView.Center)
                                }
                            }
                        }
                    }
                }
                
                // 绗簩鍒楋細鏃堕棿鏄剧ず
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
                        
                        // 棰滆壊鍔ㄧ敾
                        Behavior on color {
                            ColorAnimation { duration: 200 }
                        }
                    }
                }
                
                // 绗笁鍒楋細姝岃瘝鏂囨湰
                Item {
                    width: parent.width - 50 - 65 - 20  // 鍑忓幓鍓嶄袱鍒楀搴﹀拰闂磋窛
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
                        
                        // 瀛楀彿鍔ㄧ敾
                        Behavior on font.pixelSize {
                            NumberAnimation { duration: 200 }
                        }
                        
                        // 瀛椾綋绮楃粏鍔ㄧ敾
                        Behavior on font.bold {
                            PropertyAnimation { duration: 200 }
                        }
                        
                        // 棰滆壊鍔ㄧ敾
                        Behavior on color {
                            ColorAnimation { duration: 200 }
                        }
                        
                        // 閫忔槑搴﹀熀浜庤窛绂诲綋鍓嶈鐨勮繙杩?
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
    
    // 绌哄垪琛ㄦ彁绀?
    Text {
        anchors.centerIn: parent
        text: "鏆傛棤姝岃瘝"
        color: root.isUp ? "#888888" : "#AAAAAA"
        font.pixelSize: 16
        visible: lyricModel.count === 0
    }
    
    // 鍑芥暟锛氭竻绌烘瓕璇?
    function clearLyrics() {
        lyricModel.clear()
        root.currentLine = -1
    }
    
    // 鍑芥暟锛氭坊鍔犳瓕璇嶅垪琛紙鍓嶅悗鍔犵┖琛岋級
    function setLyrics(lyricsArray) {
        lyricModel.clear()
        
        // 娣诲姞鍓嶉潰5琛岀┖琛?
        for (var i = 0; i < 5; i++) {
            lyricModel.append({ "text": " ", "time": "" })
        }
        
        // 娣诲姞姝岃瘝鍐呭
        for (var j = 0; j < lyricsArray.length; j++) {
            var item = lyricsArray[j]
            if (typeof item === 'string') {
                // 鍏煎鏃ф牸寮忥紙绾瓧绗︿覆鏁扮粍锛?
                lyricModel.append({ "text": item, "time": "" })
            } else {
                // 鏂版牸寮忥紙甯︽椂闂寸殑瀵硅薄锛?
                lyricModel.append({ 
                    "text": item.text || "",
                    "time": item.time || ""
                })
            }
        }
        
        // 娣诲姞鍚庨潰9琛岀┖琛?
        for (var k = 0; k < 9; k++) {
            lyricModel.append({ "text": " ", "time": "" })
        }
        
        // 閲嶇疆褰撳墠琛屼负绗竴琛屾瓕璇嶏紙绱㈠紩5锛?
        root.currentLine = 5
    }
    
    // 鍑芥暟锛氶珮浜寚瀹氳
    function highlightLine(lineNumber) {
        if (lineNumber < 0 || lineNumber >= lyricModel.count) {
            return
        }
        
        root.currentLine = lineNumber
        
        // 鍙湁鍦ㄩ潪鐢ㄦ埛婊氬姩鐘舵€佷笅鎵嶈嚜鍔ㄦ粴鍔ㄥ埌楂樹寒琛?
        if (!root.isUserScrolling) {
            lyricView.positionViewAtIndex(lineNumber, ListView.Center)
        }
        
        // 鍙戝皠褰撳墠姝岃瘝鏂囨湰
        var item = lyricModel.get(lineNumber)
        if (item && item.text.trim() !== "") {
            root.currentLrcChanged(item.text)
        }
    }
    
    // 鍑芥暟锛氭粴鍔ㄥ埌鎸囧畾琛?
    function scrollToLine(lineNumber) {
        if (lineNumber < 0 || lineNumber >= lyricModel.count) {
            return
        }
        lyricView.positionViewAtIndex(lineNumber, ListView.Center)
    }
    
    // 鍑芥暟锛氳缃槸鍚﹀睍寮€鐘舵€?
    function setIsUp(up) {
        root.isUp = up
    }
}

