import QtQuick 2.14
import QtQuick.Controls 2.14
import "../../theme/Theme.js" as Theme
import "../../theme/PlayerStyle.js" as PlayerStyle

Item {
    id: root

    property int currentLine: -1
    property real currentLineProgress: 0.0
    property bool isUp: false
    property bool showSongInfo: true
    property string songTitle: ""
    property string artist: ""
    property int playerPageStyle: 0
    property var styleSpec: PlayerStyle.styleFor(playerPageStyle)
    property bool similarPanelCollapsed: false

    property bool draggingLyric: false
    property int dragPreviewTimeMs: -1
    property int _lastPreviewTimeMs: -1
    property int _centerRetryLeft: 0
    property real centerYOffset: 0

    // 根据视图高度平滑放大歌词字号，保证全屏可读性。
    property real lyricFontScale: {
        var h = lyricView ? lyricView.height : height
        var extra = Math.max(0, h - 520)
        return Math.min(1.55, 1.0 + extra / 620.0)
    }
    property real lyricBaseWidth: {
        if (!root.isUp)
            return 760
        if (root.styleSpec.lyricBaseWidth)
            return root.styleSpec.lyricBaseWidth
        if (root.playerPageStyle === 3)
            return 980
        if (root.playerPageStyle === 4)
            return 860
        return 900
    }
    property int panelPadding: {
        if (!root.isUp)
            return 0
        if (root.styleSpec.panelPadding)
            return root.styleSpec.panelPadding
        if (root.playerPageStyle === 3)
            return 24
        if (root.playerPageStyle === 4)
            return 18
        return 20
    }

    signal currentLrcChanged(string lyricText)
    signal lyricClicked(int lineIndex)
    signal lyricDragStarted()
    signal lyricDragPreview(int timeMs)
    signal lyricDragSeek(int timeMs)
    signal lyricDragEnded()
    signal similarPlayRequested(var item)

    onLyricFontScaleChanged: scheduleCenterCurrentLine()
    onCenterYOffsetChanged: scheduleCenterCurrentLine()

    ListModel {
        id: lyricModel
    }

    ListModel {
        id: similarModel
    }

    Rectangle {
        id: rootBackdrop
        anchors.fill: parent
        color: "transparent"

        gradient: Gradient {
            GradientStop { position: 0.0; color: root.isUp ? (root.styleSpec.backdropStartColor || "transparent") : "transparent" }
            GradientStop { position: 1.0; color: root.isUp ? (root.styleSpec.backdropEndColor || "transparent") : "transparent" }
        }
    }

    Rectangle {
        id: decorOrbPrimary
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: root.isUp ? 28 : 0
        anchors.topMargin: root.isUp ? 36 : 0
        width: root.isUp ? Math.round(280 * root.styleSpec.decorScale) : 0
        height: root.isUp ? Math.round(280 * root.styleSpec.decorScale) : 0
        radius: width / 2
        visible: root.isUp && root.playerPageStyle !== 3 && root.styleSpec.enableDecor !== false
        color: root.styleSpec.previewPrimary
        opacity: root.playerPageStyle === 4 ? root.styleSpec.decorOpacity * 0.75 : root.styleSpec.decorOpacity
        rotation: root.styleSpec.decorRotation

        SequentialAnimation on anchors.leftMargin {
            loops: Animation.Infinite
            running: root.isUp && root.playerPageStyle !== 3 && root.styleSpec.enableDecor !== false
            NumberAnimation { to: 28 + root.styleSpec.decorDriftDistance; duration: 5400; easing.type: Easing.InOutSine }
            NumberAnimation { to: 28; duration: 5400; easing.type: Easing.InOutSine }
        }

        SequentialAnimation on anchors.topMargin {
            loops: Animation.Infinite
            running: root.isUp && root.playerPageStyle !== 3 && root.styleSpec.enableDecor !== false
            NumberAnimation { to: 36 + root.styleSpec.decorDriftDistance * 0.4; duration: 4200; easing.type: Easing.InOutSine }
            NumberAnimation { to: 36; duration: 4200; easing.type: Easing.InOutSine }
        }
    }

    Rectangle {
        id: decorOrbSecondary
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: root.isUp ? 220 : 0
        anchors.bottomMargin: root.isUp ? 90 : 0
        width: root.isUp ? 220 : 0
        height: root.isUp ? 220 : 0
        radius: width / 2
        visible: root.isUp && root.styleSpec.enableDecor !== false
        color: root.styleSpec.previewSecondary
        opacity: root.playerPageStyle === 2 ? root.styleSpec.decorOpacity * 0.55 : root.styleSpec.decorOpacity * 0.38

        SequentialAnimation on anchors.rightMargin {
            loops: Animation.Infinite
            running: root.isUp && root.styleSpec.enableDecor !== false
            NumberAnimation { to: 220 + root.styleSpec.decorDriftDistance * 0.6; duration: 6200; easing.type: Easing.InOutSine }
            NumberAnimation { to: 220; duration: 6200; easing.type: Easing.InOutSine }
        }
    }

    Rectangle {
        id: ambientGlow
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: root.isUp ? 72 : 0
        width: root.isUp ? Math.min(parent.width * 0.52, 520) : 0
        height: root.isUp ? 160 : 0
        radius: height / 2
        visible: root.isUp && root.styleSpec.enableDecor !== false
        color: root.styleSpec.previewPrimary
        opacity: root.styleSpec.glowOpacity

        SequentialAnimation on opacity {
            loops: Animation.Infinite
            running: root.isUp && root.styleSpec.enableDecor !== false
            NumberAnimation { to: root.styleSpec.glowOpacity + 0.05; duration: 3600; easing.type: Easing.InOutSine }
            NumberAnimation { to: root.styleSpec.glowOpacity; duration: 3600; easing.type: Easing.InOutSine }
        }
    }

    Item {
        id: lyricsPanel
        anchors.top: parent.top
        anchors.topMargin: root.playerPageStyle === 4 ? 8 : 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.playerPageStyle === 4 ? 6 : 0
        anchors.left: parent.left
        anchors.right: similarPanel.visible ? similarPanel.left : parent.right
        anchors.rightMargin: similarPanel.visible ? (root.playerPageStyle === 4 ? 20 : 18) : 0
        clip: true

        Rectangle {
            anchors.fill: parent
            radius: root.styleSpec.panelRadius
            color: "transparent"
            border.width: root.isUp ? 1 : 0
            border.color: root.isUp ? root.styleSpec.lyricsBorderColor : "transparent"

            gradient: Gradient {
                GradientStop { position: 0.0; color: root.isUp ? root.styleSpec.lyricsPanelColor : "transparent" }
                GradientStop { position: 1.0; color: root.isUp ? root.styleSpec.lyricsPanelSecondaryColor : "transparent" }
            }
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: root.styleSpec.panelInsetMargin || 2
            radius: Math.max(0, root.styleSpec.panelRadius - (root.styleSpec.panelInsetMargin || 2))
            visible: root.isUp
            color: root.styleSpec.lyricsPanelInsetColor || "transparent"
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 1
            height: root.isUp ? (root.styleSpec.panelHighlightHeight || 36) : 0
            radius: Math.max(0, root.styleSpec.panelRadius - 2)
            visible: root.isUp
            color: "transparent"

            gradient: Gradient {
                GradientStop { position: 0.0; color: root.styleSpec.lyricsPanelHighlightColor || "transparent" }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        Column {
            id: songInfoBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: root.styleSpec.songInfoTopMargin || (root.panelPadding - 2)
            anchors.leftMargin: root.panelPadding
            anchors.rightMargin: root.panelPadding
            spacing: artistText.visible ? 4 : 0
            visible: root.isUp && root.showSongInfo

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.songTitle || ""
                color: root.styleSpec.lyricsTitleColor
                font.pixelSize: root.styleSpec.titleFontSize || 22
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                width: Math.min(parent.width - 30, 760)
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                id: artistText
                anchors.horizontalCenter: parent.horizontalCenter
                text: {
                    var titleText = (root.songTitle || "").toString().trim()
                    var artistName = (root.artist || "").toString().trim()
                    if (!artistName || artistName.length === 0) {
                        return ""
                    }
                    if (titleText.length > 0 && artistName === titleText) {
                        return ""
                    }
                    return artistName
                }
                visible: text.length > 0
                color: root.styleSpec.lyricsArtistColor
                font.pixelSize: root.styleSpec.artistFontSize || 13
                elide: Text.ElideRight
                width: Math.min(parent.width - 30, 760)
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: root.centerYOffset
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: root.styleSpec.dragLineColor
            visible: root.draggingLyric
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: root.isUp ? 62 : 8
            width: 96
            height: 30
            radius: 15
            color: root.styleSpec.dragBubbleColor
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
            anchors.top: songInfoBar.visible ? songInfoBar.bottom : parent.top
            anchors.topMargin: songInfoBar.visible ? (root.styleSpec.songInfoBottomGap || 16) : 0
            anchors.left: parent.left
            anchors.leftMargin: root.panelPadding
            anchors.right: parent.right
            anchors.rightMargin: root.panelPadding
            anchors.bottom: parent.bottom
            anchors.bottomMargin: root.panelPadding
            model: lyricModel
            clip: true
            interactive: root.isUp && lyricModel.count > 0
            boundsBehavior: Flickable.StopAtBounds

            highlightMoveDuration: 200
            highlightMoveVelocity: -1
            preferredHighlightBegin: height / 2
            preferredHighlightEnd: height / 2
            highlightRangeMode: ListView.StrictlyEnforceRange

            onHeightChanged: root.scheduleCenterCurrentLine()
            onWidthChanged: root.scheduleCenterCurrentLine()

            Behavior on contentY {
                enabled: !lyricView.dragging && !lyricView.flicking && !root.draggingLyric
                NumberAnimation {
                    duration: 130
                    easing.type: Easing.OutCubic
                }
            }

            onDraggingChanged: {
                if (lyricView.dragging && !root.draggingLyric && lyricView.interactive) {
                    root.draggingLyric = true
                    root.lyricDragStarted()
                    root.updateDragPreview()
                } else if (!lyricView.dragging && root.draggingLyric && !lyricView.flicking) {
                    root.finishDragSeek()
                }
            }

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
                property real linePadding: root.isUp
                                           ? (24 + Math.max(0, (root.lyricFontScale - 1.0) * 24))
                                           : 20
                height: Math.max(lyricText.height + linePadding, root.isUp ? 54 : 42)

                property bool isCurrent: index === root.currentLine
                property real highlightProgress: isCurrent
                                                 ? Math.max(0, Math.min(1, root.currentLineProgress))
                                                 : 0.0
                property real highlightRevealWidth: {
                    if (highlightProgress <= 0 || !lyricText.text || lyricText.text.trim().length === 0)
                        return 0
                    var paintedWidth = lyricText.paintedWidth > 0 ? lyricText.paintedWidth : lyricText.width
                    var leftInset = lyricText.horizontalAlignment === Text.AlignHCenter
                                    ? Math.max(0, (lyricText.width - paintedWidth) / 2)
                                    : 0
                    return Math.min(lyricText.width, leftInset + paintedWidth * highlightProgress)
                }

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
                    width: Math.min(parent.width - 8, root.isUp ? root.lyricBaseWidth : 760)
                    spacing: root.styleSpec.lyricRowSpacing || 16

                    Text {
                        id: timeText
                        width: root.styleSpec.timeColumnWidth || 82
                        text: model.time || ""
                        color: root.isUp ? root.styleSpec.lyricsTimeColor : "#7F8AA2"
                        font.pixelSize: Math.round((lyricItem.isCurrent ? 14 : 12) * root.lyricFontScale)
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                        visible: model.time && model.time !== ""

                        Behavior on font.pixelSize {
                            NumberAnimation { duration: 180 }
                        }
                    }

                    Item {
                        width: parent.width - (timeText.visible ? timeText.width + parent.spacing : 0)
                        height: lyricText.height

                        Text {
                            id: lyricText
                            width: parent.width
                            text: model.text
                            color: root.isUp ? root.styleSpec.lyricsLineColor : "#2A3242"
                            font.pixelSize: Math.round(((lyricItem.isCurrent
                                                            ? (root.styleSpec.currentLyricFontSize || 21)
                                                            : (root.styleSpec.normalLyricFontSize || 16)))
                                                        * root.lyricFontScale)
                            font.bold: lyricItem.isCurrent
                            horizontalAlignment: timeText.visible ? Text.AlignLeft : Text.AlignHCenter
                            wrapMode: Text.WordWrap

                            Behavior on font.pixelSize {
                                NumberAnimation { duration: 180 }
                            }

                            Behavior on opacity {
                                NumberAnimation { duration: 180 }
                            }

                            opacity: {
                                if (lyricItem.isCurrent) return 1.0
                                var distance = Math.abs(index - root.currentLine)
                                if (distance <= 1) return root.styleSpec.nearOpacity || 0.66
                                if (distance <= 2) return root.styleSpec.midOpacity || 0.48
                                return root.styleSpec.farOpacity || 0.34
                            }
                        }

                        Item {
                            x: lyricText.x
                            y: lyricText.y
                            width: lyricItem.highlightRevealWidth
                            height: lyricText.height
                            clip: true
                            visible: lyricItem.isCurrent && width > 0

                            Text {
                                width: lyricText.width
                                height: lyricText.height
                                text: lyricText.text
                                color: Theme.accent
                                font.pixelSize: lyricText.font.pixelSize
                                font.bold: lyricText.font.bold
                                horizontalAlignment: lyricText.horizontalAlignment
                                wrapMode: lyricText.wrapMode
                            }
                        }
                    }
                }
            }
        }

        Text {
            anchors.centerIn: parent
            text: "暂无歌词"
            color: root.isUp ? root.styleSpec.lyricsEmptyColor : "#A6B0C0"
            font.pixelSize: Math.round(16 * root.lyricFontScale)
            visible: lyricModel.count === 0
        }
    }

    Rectangle {
        id: panelGapBridge
        visible: root.isUp && similarPanel.visible && root.styleSpec.bridgePanels !== false
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: lyricsPanel.right
        anchors.right: similarPanel.left
        color: root.styleSpec.panelGapColor || "transparent"
        radius: Math.max(0, root.styleSpec.panelRadius - 6)
    }

    Rectangle {
        id: similarPanel
        anchors.top: parent.top
        anchors.topMargin: root.playerPageStyle === 4 ? 8 : 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.playerPageStyle === 4 ? 6 : 0
        anchors.right: parent.right
        width: root.playerPageStyle === 4
               ? Math.min(264, Math.max(208, parent.width * 0.22))
               : Math.min(320, Math.max(220, parent.width * 0.30))
        visible: root.isUp && root.styleSpec.showSimilarPanel && !root.similarPanelCollapsed
        color: root.styleSpec.similarPanelColor
        radius: root.styleSpec.panelRadius
        border.color: root.styleSpec.lyricsBorderColor
        border.width: 1

        Item {
            anchors.fill: parent
            anchors.margins: 12

            Text {
                id: similarTitle
                text: "相似推荐"
                color: root.styleSpec.lyricsTitleColor
                font.pixelSize: 16
                font.bold: true
                anchors.top: parent.top
                anchors.left: parent.left
            }

            Rectangle {
                id: similarCloseButton
                width: 24
                height: 24
                radius: 12
                anchors.verticalCenter: similarTitle.verticalCenter
                anchors.right: parent.right
                color: closeMouse.containsMouse ? root.styleSpec.similarHoverColor : "transparent"
                border.width: 1
                border.color: closeMouse.containsMouse ? root.styleSpec.similarBorderColor : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "×"
                    color: root.styleSpec.lyricsArtistColor
                    font.pixelSize: 16
                    font.bold: true
                }

                MouseArea {
                    id: closeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.similarPanelCollapsed = true
                }
            }

            Text {
                id: similarSubtitle
                text: "当前歌曲相关内容"
                color: root.styleSpec.lyricsArtistColor
                font.pixelSize: 12
                anchors.top: similarTitle.bottom
                anchors.topMargin: 6
                anchors.left: parent.left
            }
            ListView {
                id: similarList
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: similarSubtitle.bottom
                anchors.topMargin: root.playerPageStyle === 4 ? 12 : 10
                anchors.bottom: parent.bottom
                clip: true
                spacing: 6
                model: similarModel

                delegate: Rectangle {
                    width: similarList.width
                    height: 58
                    radius: 8
                    color: rowMouse.containsMouse ? root.styleSpec.similarHoverColor : root.styleSpec.similarItemColor
                    border.color: rowMouse.containsMouse ? root.styleSpec.similarBorderColor : "transparent"

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 8

                        Rectangle {
                            width: 40
                            height: 40
                            radius: 4
                            anchors.verticalCenter: parent.verticalCenter
                            color: "#2F3743"

                            Image {
                                anchors.fill: parent
                                source: model.cover_art_url || "qrc:/qml/assets/ai/icons/default-music-cover.svg"
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                cache: true
                            }
                        }

                        Column {
                            width: parent.width - 120
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 2

                            Text {
                                width: parent.width
                                text: model.title || "未知歌曲"
                                color: root.styleSpec.lyricsTitleColor
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }

                            Text {
                                width: parent.width
                                text: model.artist || "未知艺术家"
                                color: root.styleSpec.lyricsArtistColor
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }

                        Text {
                            text: model.duration || ""
                            anchors.verticalCenter: parent.verticalCenter
                            color: root.styleSpec.lyricsTimeColor
                            font.pixelSize: 11
                        }
                    }

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.similarPlayRequested({
                                "song_id": model.song_id || "",
                                "path": model.path || "",
                                "play_path": model.play_path || model.stream_url || model.path || "",
                                "stream_url": model.stream_url || "",
                                "title": model.title || "",
                                "artist": model.artist || "",
                                "cover_art_url": model.cover_art_url || "",
                                "duration": model.duration || "0:00",
                                "duration_sec": model.duration_sec || 0,
                                "request_id": model.request_id || "",
                                "model_version": model.model_version || "",
                                "scene": model.scene || "detail"
                            })
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    width: 8
                }

                Text {
                    anchors.centerIn: parent
                    visible: similarModel.count === 0
                    text: "暂无相似推荐"
                    color: root.styleSpec.lyricsEmptyColor
                    font.pixelSize: 13
                }
            }
        }
    }

    Rectangle {
        id: similarRestoreButton
        visible: root.isUp && root.styleSpec.showSimilarPanel && root.similarPanelCollapsed
        width: 38
        height: 92
        radius: 19
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        color: root.styleSpec.similarPanelColor
        border.width: 1
        border.color: root.styleSpec.lyricsBorderColor

        Text {
            anchors.centerIn: parent
            text: "推荐"
            color: root.styleSpec.lyricsTitleColor
            font.pixelSize: 13
            font.bold: true
            rotation: -90
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.similarPanelCollapsed = false
        }
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
        var markerY = lyricView.contentY + lyricView.height / 2 + root.centerYOffset
        var idx = lyricView.indexAt(lyricView.contentX + lyricView.width / 2,
                                    markerY)
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
        root.currentLineProgress = 0.0
        root.dragPreviewTimeMs = -1
        root._lastPreviewTimeMs = -1
        root.lyricDragEnded()
    }

    function clearLyrics() {
        lyricModel.clear()
        root.currentLine = -1
        root.currentLineProgress = 0.0
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
        root.currentLineProgress = 0.0
        lyricView.currentIndex = 5
        lyricView.positionViewAtIndex(5, ListView.Center)
        scheduleCenterCurrentLine()
    }

    function highlightLine(lineNumber) {
        if (lineNumber < 0 || lineNumber >= lyricModel.count)
            return

        if (root.draggingLyric)
            return

        if (lineNumber !== root.currentLine) {
            root.currentLineProgress = 0.0
        }
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

        if (lineNumber === lyricView.currentIndex)
            return

        lyricView.positionViewAtIndex(lineNumber, ListView.Center)
    }

    function setIsUp(up) {
        root.isUp = up
        if (up) {
            scheduleCenterCurrentLine()
        }
    }

    function scheduleCenterCurrentLine() {
        if (!root.isUp || root.draggingLyric || root.currentLine < 0 || lyricModel.count <= 0) {
            return
        }
        root._centerRetryLeft = 2
        centerTimer.restart()
    }

    function forceCenterCurrentLine() {
        if (!root.isUp || root.draggingLyric || root.currentLine < 0 || root.currentLine >= lyricModel.count) {
            return
        }
        lyricView.positionViewAtIndex(root.currentLine, ListView.Center)
        if (Math.abs(root.centerYOffset) > 0.1) {
            var targetContentY = lyricView.contentY - root.centerYOffset
            var maxContentY = Math.max(0, lyricView.contentHeight - lyricView.height)
            if (targetContentY < 0) {
                targetContentY = 0
            } else if (targetContentY > maxContentY) {
                targetContentY = maxContentY
            }
            lyricView.contentY = targetContentY
        }
    }

    Timer {
        id: centerTimer
        interval: 16
        repeat: false
        onTriggered: {
            root.forceCenterCurrentLine()
            root._centerRetryLeft = root._centerRetryLeft - 1
            if (root._centerRetryLeft > 0) {
                centerTimer.restart()
            }
        }
    }

    function setSongInfo(title, artistName) {
        root.songTitle = title || ""
        root.artist = artistName || ""
    }

    function clearSimilarSongs() {
        similarModel.clear()
        similarPanelCollapsed = false
    }

    function setSimilarSongs(items) {
        similarModel.clear()
        similarPanelCollapsed = false
        if (!items || items.length === 0) {
            return
        }
        for (var i = 0; i < items.length; i++) {
            var item = items[i]
            similarModel.append({
                "song_id": item.song_id || item.path || "",
                "path": item.path || "",
                "play_path": item.play_path || item.stream_url || item.path || "",
                "stream_url": item.stream_url || "",
                "title": item.title || "",
                "artist": item.artist || "",
                "cover_art_url": item.cover_art_url || "",
                "duration": item.duration || "",
                "duration_sec": item.duration_sec || 0,
                "request_id": item.request_id || "",
                "model_version": item.model_version || "",
                "scene": item.scene || "detail"
            })
        }
    }
}
