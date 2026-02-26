import QtQuick 2.14
import QtQuick.Controls 2.14

Item {
    id: root

    property int currentLine: -1
    property bool isUp: false
    property string songTitle: ""
    property string artist: ""

    property bool draggingLyric: false
    property int dragPreviewTimeMs: -1
    property int _lastPreviewTimeMs: -1

    signal currentLrcChanged(string lyricText)
    signal lyricClicked(int lineIndex)
    signal lyricDragStarted()
    signal lyricDragPreview(int timeMs)
    signal lyricDragSeek(int timeMs)
    signal lyricDragEnded()

    ListModel {
        id: lyricModel
    }

    // 拖动时的中心定位线
    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: "#55FFFFFF"
        visible: root.draggingLyric
    }

    // 拖动预览时间
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 8
        width: 92
        height: 30
        radius: 15
        color: "#AA000000"
        visible: root.draggingLyric && root.dragPreviewTimeMs >= 0

        Text {
            anchors.centerIn: parent
            text: root.formatTimeMs(root.dragPreviewTimeMs)
            color: "#FFFFFF"
            font.pixelSize: 13
            font.bold: true
        }
    }

    ListView {
        id: lyricView
        anchors.fill: parent
        model: lyricModel
        clip: true
        interactive: root.isUp && lyricModel.count > 0
        boundsBehavior: Flickable.StopAtBounds

        highlightMoveDuration: 200
        highlightMoveVelocity: -1
        preferredHighlightBegin: height / 2 - 30
        preferredHighlightEnd: height / 2 + 30
        highlightRangeMode: ListView.ApplyRange

        // 只在用户实际拖拽时进入歌词拖动模式，避免程序自动滚动触发误跳转。
        onDraggingChanged: {
            if (lyricView.dragging && !root.draggingLyric && lyricView.interactive) {
                root.draggingLyric = true
                root.lyricDragStarted()
                root.updateDragPreview()
            } else if (!lyricView.dragging && root.draggingLyric && !lyricView.flicking) {
                root.finishDragSeek()
            }
        }

        // 用户松手后若仍在惯性滚动，等惯性结束再seek。
        onFlickingChanged: {
            if (!lyricView.flicking && root.draggingLyric && !lyricView.dragging) {
                root.finishDragSeek()
            }
        }

        onContentYChanged: {
            if (root.draggingLyric) {
                root.updateDragPreview()
            }
        }

        delegate: Item {
            id: lyricItem
            width: lyricView.width
            height: Math.max(lyricText.height + 20, 40)

            property bool isCurrent: index === root.currentLine

            TapHandler {
                onTapped: {
                    if (root.draggingLyric) {
                        return
                    }
                    if (model.timeMs !== undefined && Number(model.timeMs) >= 0) {
                        root.lyricClicked(index)
                    }
                }
            }

            Row {
                anchors.centerIn: parent
                width: parent.width - 40
                spacing: 20

                Text {
                    id: timeText
                    width: 80
                    text: model.time || ""
                    color: root.isUp ? "#CCCCCC" : "#666666"
                    font.pixelSize: lyricItem.isCurrent ? 14 : 12
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    visible: model.time && model.time !== ""

                    Behavior on font.pixelSize {
                        NumberAnimation { duration: 200 }
                    }
                }

                Text {
                    id: lyricText
                    width: parent.width - (timeText.visible ? timeText.width + parent.spacing : 0)
                    text: model.text
                    color: root.isUp ? "#FFFFFF" : "#333333"
                    font.pixelSize: lyricItem.isCurrent ? 20 : 16
                    font.bold: lyricItem.isCurrent
                    horizontalAlignment: Text.AlignLeft
                    wrapMode: Text.WordWrap

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

    Text {
        anchors.centerIn: parent
        text: "暂无歌词"
        color: root.isUp ? "#888888" : "#AAAAAA"
        font.pixelSize: 16
        visible: lyricModel.count === 0
    }

    function normalizeTimeMs(value) {
        var n = Number(value)
        if (isNaN(n) || n < 0)
            return -1
        return Math.round(n)
    }

    function formatTimeMs(timeMs) {
        var totalSec = Math.floor(Math.max(0, timeMs) / 1000)
        var mm = Math.floor(totalSec / 60)
        var ss = totalSec % 60
        return (mm < 10 ? "0" : "") + mm + ":" + (ss < 10 ? "0" : "") + ss
    }

    function nearestTimedIndex(baseIndex) {
        if (baseIndex < 0 || lyricModel.count <= 0)
            return -1

        for (var offset = 0; offset < lyricModel.count; ++offset) {
            var left = baseIndex - offset
            if (left >= 0) {
                var leftItem = lyricModel.get(left)
                if (normalizeTimeMs(leftItem.timeMs) >= 0)
                    return left
            }

            if (offset === 0)
                continue

            var right = baseIndex + offset
            if (right < lyricModel.count) {
                var rightItem = lyricModel.get(right)
                if (normalizeTimeMs(rightItem.timeMs) >= 0)
                    return right
            }
        }

        return -1
    }

    function centerTimedIndex() {
        // ListView.indexAt 使用的是 content 坐标系，Y 需要加 contentY。
        var idx = lyricView.indexAt(lyricView.contentX + lyricView.width / 2,
                                    lyricView.contentY + lyricView.height / 2)
        if (idx < 0)
            idx = lyricView.currentIndex
        return nearestTimedIndex(idx)
    }

    function updateDragPreview() {
        var idx = centerTimedIndex()
        if (idx < 0)
            return

        var item = lyricModel.get(idx)
        var timeMs = normalizeTimeMs(item.timeMs)
        if (timeMs < 0)
            return

        root.dragPreviewTimeMs = timeMs
        if (root._lastPreviewTimeMs !== timeMs) {
            root._lastPreviewTimeMs = timeMs
            root.lyricDragPreview(timeMs)
        }
    }

    function finishDragSeek() {
        var idx = centerTimedIndex()
        if (idx >= 0) {
            var item = lyricModel.get(idx)
            var timeMs = normalizeTimeMs(item.timeMs)
            if (timeMs >= 0) {
                lyricView.positionViewAtIndex(idx, ListView.Center)
                root.currentLine = idx
                root.lyricDragSeek(timeMs)
            }
        }

        root.draggingLyric = false
        root.dragPreviewTimeMs = -1
        root._lastPreviewTimeMs = -1
        root.lyricDragEnded()
    }

    function clearLyrics() {
        lyricModel.clear()
        root.currentLine = -1
        root.draggingLyric = false
        root.dragPreviewTimeMs = -1
        root._lastPreviewTimeMs = -1
    }

    function setLyrics(lyricsArray) {
        lyricModel.clear()

        for (var i = 0; i < 5; i++) {
            lyricModel.append({ "text": " ", "time": "", "timeMs": -1 })
        }

        for (var j = 0; j < lyricsArray.length; j++) {
            var item = lyricsArray[j]
            if (typeof item === "string") {
                lyricModel.append({ "text": item, "time": "", "timeMs": -1 })
            } else {
                lyricModel.append({
                    "text": item.text || "",
                    "time": item.time || "",
                    "timeMs": normalizeTimeMs(item.timeMs)
                })
            }
        }

        for (var k = 0; k < 9; k++) {
            lyricModel.append({ "text": " ", "time": "", "timeMs": -1 })
        }

        root.currentLine = 5
        lyricView.currentIndex = 5
        lyricView.positionViewAtIndex(5, ListView.Center)
    }

    function highlightLine(lineNumber) {
        if (lineNumber < 0 || lineNumber >= lyricModel.count)
            return

        if (root.draggingLyric)
            return

        root.currentLine = lineNumber
        lyricView.currentIndex = lineNumber

        var item = lyricModel.get(lineNumber)
        if (item && item.text.trim() !== "") {
            root.currentLrcChanged(item.text)
        }
    }

    function scrollToLine(lineNumber) {
        if (lineNumber < 0 || lineNumber >= lyricModel.count)
            return

        if (root.draggingLyric)
            return

        lyricView.positionViewAtIndex(lineNumber, ListView.Center)
    }

    function setIsUp(up) {
        root.isUp = up
    }

    function setSongInfo(title, artistName) {
        root.songTitle = title || ""
        root.artist = artistName || ""
    }
}
