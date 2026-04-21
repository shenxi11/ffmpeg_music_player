import QtGraphicalEffects 1.14
import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Dialogs 1.3
import QtQuick.Layouts 1.14

Item {
    id: root
    width: 760
    height: 190

    property string currentLyricText: "\u2764 \u6682\u65e0\u6b4c\u8bcd \u2764"
    property string nextLyricText: ""
    property string songName: "\u6682\u65e0\u6b4c\u66f2"
    property color playedLyricColor: "#ded93e"
    property color unplayedLyricColor: "#4ca6e8"
    property color strokeColor: "#000000"
    property int lyricFontSize: 57
    property string lyricFontFamily: "Microsoft YaHei"
    property bool hovered: false
    property bool isResizing: false
    property bool isPlaying: false
    property bool isLocked: false
    property bool settingsMenuOpen: false
    property bool colorPaletteExpanded: false
    property string activeColorRole: "played"
    property string lineMode: "double"
    property string alignment: "split"
    property int lyricOpacity: 100
    property bool lyricBold: false
    property real currentLineProgress: 0.0

    readonly property bool singleLineMode: lineMode === "single"
    readonly property bool anyHoverZoneActive: bodyHoverArea.containsMouse
                                              || topToolbarHover.hovered
                                              || settingsPopupHover.hovered
                                              || unlockChipHover.hovered
                                              || resizeMouseArea.containsMouse
    readonly property int minWidth: 360
    readonly property int minHeight: 132
    readonly property int maxWidth: 1800
    readonly property int maxHeight: 420
    readonly property int autoWidthThreshold: 24
    readonly property int splitOffset: singleLineMode ? 0 : 96

    signal playClicked(int state)
    signal nextClicked()
    signal lastClicked()
    signal forwardClicked()
    signal backwardClicked()
    signal settingsClicked()
    signal closeClicked()
    signal windowMoved(var newPos)
    signal windowResized(var newSize)
    signal lyricStyleChanged(var style)

    function normalizedText(text, fallbackText) {
        var trimmed = (text || "").trim()
        return trimmed.length > 0 ? trimmed : fallbackText
    }

    function textAlignmentForLine(lineIndex) {
        if (alignment === "left") {
            return Text.AlignLeft
        }
        if (alignment === "right") {
            return Text.AlignRight
        }
        if (alignment === "split") {
            return lineIndex === 0 ? Text.AlignLeft : Text.AlignRight
        }
        return Text.AlignHCenter
    }

    function lineWidthFor(lineIndex) {
        if (singleLineMode || alignment !== "split") {
            return lyricColumn.width
        }
        return lyricColumn.width - splitOffset
    }

    function lineXFor(lineIndex) {
        if (singleLineMode || alignment !== "split") {
            return 0
        }
        return lineIndex === 0 ? 0 : splitOffset
    }

    function emitStylePayload() {
        var payload = {
            color: playedLyricColor,
            playedColor: playedLyricColor,
            unplayedColor: unplayedLyricColor,
            strokeColor: strokeColor,
            fontSize: lyricFontSize,
            fontFamily: lyricFontFamily,
            lineMode: lineMode,
            alignment: alignment,
            opacity: lyricOpacity,
            bold: lyricBold
        }
        lyricStyleChanged(payload)
        scheduleAutoResize()
    }

    function stepFontSize(delta) {
        var nextSize = Math.max(24, Math.min(64, lyricFontSize + delta))
        if (nextSize === lyricFontSize) {
            return
        }
        lyricFontSize = nextSize
        emitStylePayload()
    }

    function toggleLineMode() {
        lineMode = singleLineMode ? "double" : "single"
        emitStylePayload()
    }

    function openColorPicker(role) {
        activeColorRole = role
        if (role === "played") {
            colorDialog.color = playedLyricColor
        } else if (role === "unplayed") {
            colorDialog.color = unplayedLyricColor
        } else {
            colorDialog.color = strokeColor
        }
        colorDialog.open()
    }

    function applySelectedColor(color) {
        if (activeColorRole === "played") {
            playedLyricColor = color
        } else if (activeColorRole === "unplayed") {
            unplayedLyricColor = color
        } else {
            strokeColor = color
        }
        emitStylePayload()
    }

    function setLyricLines(currentText, nextText) {
        var nextCurrent = normalizedText(currentText, "\u2764 \u6682\u65e0\u6b4c\u8bcd \u2764")
        if (currentLyricText !== nextCurrent) {
            currentLineProgress = 0.0
        }
        currentLyricText = nextCurrent
        nextLyricText = (nextText || "").trim()
        scheduleAutoResize()
    }

    function setLyricText(text) {
        setLyricLines(text, "")
    }

    function setSongName(name) {
        songName = normalizedText(name, "\u6682\u65e0\u6b4c\u66f2")
    }

    function setPlayButtonState(playing) {
        isPlaying = playing
    }

    function setCurrentLineProgress(progress) {
        currentLineProgress = Math.max(0.0, Math.min(1.0, progress))
    }

    function setLyricStyle(style) {
        if (style.hasOwnProperty("color") && ("" + style.color).length > 0) {
            playedLyricColor = style.color
        }
        if (style.hasOwnProperty("playedColor") && ("" + style.playedColor).length > 0) {
            playedLyricColor = style.playedColor
        }
        if (style.hasOwnProperty("unplayedColor") && ("" + style.unplayedColor).length > 0) {
            unplayedLyricColor = style.unplayedColor
        }
        if (style.hasOwnProperty("strokeColor") && ("" + style.strokeColor).length > 0) {
            strokeColor = style.strokeColor
        }
        if (style.hasOwnProperty("fontSize")) {
            lyricFontSize = Math.max(24, Math.min(64, style.fontSize))
        }
        if (style.hasOwnProperty("fontFamily") && (style.fontFamily || "").trim().length > 0) {
            lyricFontFamily = style.fontFamily
        }
        if (style.hasOwnProperty("lineMode") && (style.lineMode || "").trim().length > 0) {
            lineMode = style.lineMode
        }
        if (style.hasOwnProperty("alignment") && (style.alignment || "").trim().length > 0) {
            alignment = style.alignment
        }
        if (style.hasOwnProperty("opacity")) {
            lyricOpacity = Math.max(0, Math.min(100, style.opacity))
        }
        if (style.hasOwnProperty("bold")) {
            lyricBold = !!style.bold
        }
        scheduleAutoResize()
    }

    function showSettingsDialog() {
        refreshHoverState()
        settingsMenuOpen = true
    }

    function closeTransientPanels() {
        settingsMenuOpen = false
        colorPaletteExpanded = false
    }

    function refreshHoverState() {
        if (hoverExitTimer.running) {
            hoverExitTimer.stop()
        }
        hovered = true
    }

    function scheduleHoverExit() {
        if (anyHoverZoneActive) {
            refreshHoverState()
            return
        }
        if (!hoverExitTimer.running) {
            hoverExitTimer.start()
        } else {
            hoverExitTimer.restart()
        }
    }

    function scheduleAutoResize() {
        if (!autoResizeTimer.running) {
            autoResizeTimer.start()
        } else {
            autoResizeTimer.restart()
        }
    }

    function recalcPreferredSize() {
        var maxTextWidth = Math.max(primaryMetrics.width, singleLineMode ? 0 : secondaryMetrics.width)
        var targetWidth = Math.ceil(maxTextWidth + 168 + (alignment === "split" && !singleLineMode ? splitOffset : 32))
        targetWidth = Math.max(minWidth, Math.min(maxWidth, targetWidth))

        var lineCount = singleLineMode ? 1 : 2
        var lineHeight = lyricFontSize + 28
        var targetHeight = Math.ceil(lineHeight * lineCount + 72)
        targetHeight = Math.max(minHeight, Math.min(maxHeight, targetHeight))

        if (Math.abs(targetWidth - root.width) >= autoWidthThreshold ||
                Math.abs(targetHeight - root.height) >= 12) {
            windowResized(Qt.size(targetWidth, targetHeight))
        }
    }

    TextMetrics {
        id: primaryMetrics
        text: root.currentLyricText
        font.family: root.lyricFontFamily
        font.pixelSize: root.lyricFontSize
        font.bold: root.lyricBold
    }

    TextMetrics {
        id: secondaryMetrics
        text: root.nextLyricText
        font.family: root.lyricFontFamily
        font.pixelSize: root.lyricFontSize
        font.bold: root.lyricBold
    }

    Timer {
        id: autoResizeTimer
        interval: 80
        repeat: false
        onTriggered: root.recalcPreferredSize()
    }

    Timer {
        id: hoverExitTimer
        interval: 90
        repeat: false
        onTriggered: {
            if (!root.anyHoverZoneActive) {
                root.hovered = false
                root.closeTransientPanels()
            }
        }
    }

    onCurrentLyricTextChanged: scheduleAutoResize()
    onNextLyricTextChanged: scheduleAutoResize()
    onLyricFontSizeChanged: scheduleAutoResize()
    onLyricFontFamilyChanged: scheduleAutoResize()
    onLineModeChanged: scheduleAutoResize()
    onAlignmentChanged: scheduleAutoResize()

    Rectangle {
        id: hoverSurface
        anchors.fill: parent
        color: root.hovered && !root.isLocked ? Qt.rgba(0.39, 0.39, 0.39, 0.84) : "transparent"
        radius: 0
        border.width: root.hovered && !root.isLocked ? 1 : 0
        border.color: Qt.rgba(1, 1, 1, 0.12)
        antialiasing: true

        Behavior on color {
            ColorAnimation { duration: 180 }
        }
    }

    DropShadow {
        anchors.fill: hoverSurface
        source: hoverSurface
        radius: 22
        samples: 41
        color: root.hovered && !root.isLocked ? Qt.rgba(0, 0, 0, 0.24) : "transparent"
        horizontalOffset: 0
        verticalOffset: 8
    }

    MouseArea {
        id: dragSurface
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        hoverEnabled: false
        propagateComposedEvents: true
        z: 1

        property point lastPoint

        onPressed: {
            if (root.settingsMenuOpen) {
                mouse.accepted = false
                return
            }
            lastPoint = Qt.point(mouse.x, mouse.y)
        }

        onPositionChanged: {
            if (!(mouse.buttons & Qt.LeftButton) || root.isResizing || root.settingsMenuOpen) {
                return
            }
            var delta = Qt.point(mouse.x - lastPoint.x, mouse.y - lastPoint.y)
            root.windowMoved(delta)
        }
    }

    MouseArea {
        id: bodyHoverArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        onEntered: root.refreshHoverState()
        onExited: root.scheduleHoverExit()
    }

    Item {
        id: topToolbar
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: toolbarRow.implicitWidth + 24
        height: 34
        z: 30
        visible: !root.isLocked
        opacity: root.hovered ? 1.0 : 0.0

        Behavior on opacity {
            NumberAnimation { duration: 140 }
        }

        HoverHandler {
            id: topToolbarHover
            onHoveredChanged: {
                if (hovered) {
                    root.refreshHoverState()
                } else {
                    root.scheduleHoverExit()
                }
            }
        }

        Row {
            id: toolbarRow
            anchors.centerIn: parent
            spacing: 4

            Button {
                width: 28
                height: 28
                text: "\u8bcd"
                onClicked: {}
                ToolTip.visible: hovered
                ToolTip.text: "\u8bcd\u56fe"
                background: Rectangle {
                    radius: height / 2
                    color: parent.down ? Qt.rgba(1, 1, 1, 0.20)
                                       : parent.hovered ? Qt.rgba(1, 1, 1, 0.14)
                                                        : "transparent"
                }
                contentItem: Text {
                    text: parent.text
                    color: "#f4f4f4"
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Button {
                width: 28
                height: 28
                text: "\u23ee"
                onClicked: root.lastClicked()
                ToolTip.visible: hovered
                ToolTip.text: "\u4e0a\u4e00\u9996"
                background: Rectangle {
                    radius: height / 2
                    color: parent.down ? Qt.rgba(1, 1, 1, 0.20)
                                       : parent.hovered ? Qt.rgba(1, 1, 1, 0.14)
                                                        : "transparent"
                }
                contentItem: Text {
                    text: parent.text
                    color: "#f4f4f4"
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Button {
                width: 32
                height: 28
                text: root.isPlaying ? "\u23f8" : "\u25b6"
                onClicked: {
                    root.isPlaying = !root.isPlaying
                    root.playClicked(root.isPlaying ? 1 : 0)
                }
                ToolTip.visible: hovered
                ToolTip.text: "\u64ad\u653e/\u6682\u505c"
                background: Rectangle {
                    radius: height / 2
                    color: parent.down ? Qt.rgba(1, 1, 1, 0.22)
                                       : parent.hovered ? Qt.rgba(1, 1, 1, 0.16)
                                                        : "transparent"
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.pixelSize: parent.text === "\u25b6" ? 13 : 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Button {
                width: 28
                height: 28
                text: "\u23ed"
                onClicked: root.nextClicked()
                ToolTip.visible: hovered
                ToolTip.text: "\u4e0b\u4e00\u9996"
                background: Rectangle {
                    radius: height / 2
                    color: parent.down ? Qt.rgba(1, 1, 1, 0.20)
                                       : parent.hovered ? Qt.rgba(1, 1, 1, 0.14)
                                                        : "transparent"
                }
                contentItem: Text {
                    text: parent.text
                    color: "#f4f4f4"
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Rectangle {
                width: 1
                height: 14
                radius: 0
                anchors.verticalCenter: parent.verticalCenter
                color: Qt.rgba(1, 1, 1, 0.24)
            }

            Button {
                width: 28
                height: 28
                text: "\u8bd1"
                enabled: false
                ToolTip.visible: hovered
                ToolTip.text: "\u7ffb\u8bd1/\u97f3\u8bd1"
                background: Rectangle {
                    radius: height / 2
                    color: parent.hovered ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
                }
                contentItem: Text {
                    text: parent.text
                    color: "#9f9f9f"
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Button {
                width: 28
                height: 28
                text: "\u9501"
                onClicked: {
                    root.isLocked = true
                    root.settingsMenuOpen = false
                    root.colorPaletteExpanded = false
                }
                ToolTip.visible: hovered
                ToolTip.text: "\u9501\u5b9a"
                background: Rectangle {
                    radius: height / 2
                    color: parent.down ? Qt.rgba(1, 1, 1, 0.20)
                                       : parent.hovered ? Qt.rgba(1, 1, 1, 0.14)
                                                        : "transparent"
                }
                contentItem: Text {
                    text: parent.text
                    color: "#f4f4f4"
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Item {
                width: 32
                height: 28

                Button {
                    id: settingsButton
                    anchors.fill: parent
                    text: "\u8bbe"
                    onClicked: {
                        root.settingsMenuOpen = !root.settingsMenuOpen
                        if (!root.settingsMenuOpen) {
                            root.colorPaletteExpanded = false
                        }
                    }
                    ToolTip.visible: hovered
                    ToolTip.text: "\u8bbe\u7f6e"
                    background: Rectangle {
                        radius: height / 2
                        color: parent.down || root.settingsMenuOpen ? Qt.rgba(1, 1, 1, 0.20)
                                                                    : parent.hovered ? Qt.rgba(1, 1, 1, 0.14)
                                                                                     : "transparent"
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "#ffffff"
                        font.pixelSize: 12
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Rectangle {
                    id: settingsPopup
                    visible: root.settingsMenuOpen
                    anchors.top: parent.bottom
                    anchors.topMargin: 8
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 278
                    height: settingsColumn.implicitHeight + 12
                    radius: 0
                    color: "#ffffff"
                    border.width: 1
                    border.color: "#e6e6e6"
                    z: 120

                    HoverHandler {
                        id: settingsPopupHover
                        onHoveredChanged: {
                            if (hovered) {
                                root.refreshHoverState()
                            } else {
                                root.scheduleHoverExit()
                            }
                        }
                    }

                    Column {
                        id: settingsColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 6
                        spacing: 0

                        Rectangle {
                            width: parent.width
                            height: 38
                            color: "transparent"

                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 12
                                spacing: 10

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "\u5b57\u4f53\u5927\u5c0f"
                                    color: "#4b5563"
                                    font.pixelSize: 13
                                }

                                Item { width: 1; height: 1 }

                                Rectangle {
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 54
                                    height: 22
                                    radius: 0
                                    color: "#ffffff"
                                    border.width: 1
                                    border.color: "#d5d5d5"

                                    Row {
                                        anchors.fill: parent
                                        spacing: 0

                                        Button {
                                            width: 27
                                            height: parent.height
                                            text: "-"
                                            onClicked: root.stepFontSize(-3)
                                            background: Rectangle {
                                                color: parent.down ? "#f0f0f0"
                                                                   : parent.hovered ? "#f8f8f8"
                                                                                    : "transparent"
                                            }
                                            contentItem: Text {
                                                text: parent.text
                                                color: "#6b7280"
                                                font.pixelSize: 14
                                                font.bold: true
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                        }

                                        Rectangle {
                                            width: 1
                                            height: parent.height
                                            color: "#e0e0e0"
                                        }

                                        Button {
                                            width: 26
                                            height: parent.height
                                            text: "+"
                                            onClicked: root.stepFontSize(3)
                                            background: Rectangle {
                                                color: parent.down ? "#f0f0f0"
                                                                   : parent.hovered ? "#f8f8f8"
                                                                                    : "transparent"
                                            }
                                            contentItem: Text {
                                                text: parent.text
                                                color: "#6b7280"
                                                font.pixelSize: 14
                                                font.bold: true
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: root.colorPaletteExpanded ? 94 : 38
                            color: "transparent"

                            Column {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 12
                                anchors.topMargin: 10
                                spacing: 10

                                Row {
                                    width: parent.width
                                    spacing: 10

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "\u5b57\u4f53\u989c\u8272"
                                        color: "#4b5563"
                                        font.pixelSize: 13
                                    }

                                    Item { width: 1; height: 1 }

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: root.colorPaletteExpanded ? "\u2303" : "\u203a"
                                        color: "#9ca3af"
                                        font.pixelSize: root.colorPaletteExpanded ? 10 : 16
                                        font.bold: true
                                    }
                                }

                                Flow {
                                    visible: root.colorPaletteExpanded
                                    width: parent.width - 12
                                    spacing: 12

                                    Repeater {
                                        model: [
                                            { "role": "played", "label": "\u5df2\u64ad" },
                                            { "role": "unplayed", "label": "\u672a\u64ad" },
                                            { "role": "stroke", "label": "\u63cf\u8fb9" }
                                        ]

                                        delegate: Row {
                                            spacing: 6

                                            Text {
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: modelData.label
                                                color: "#6b7280"
                                                font.pixelSize: 12
                                            }

                                            Rectangle {
                                                width: 16
                                                height: 16
                                                radius: 0
                                                border.width: 1
                                                border.color: "#d7d7d7"
                                                color: modelData.role === "played"
                                                       ? root.playedLyricColor
                                                       : modelData.role === "unplayed"
                                                         ? root.unplayedLyricColor
                                                         : root.strokeColor

                                                MouseArea {
                                                    anchors.fill: parent
                                                    cursorShape: Qt.PointingHandCursor
                                                    onClicked: root.openColorPicker(modelData.role)
                                                }
                                            }
                                        }
                                    }
                                }

                                MouseArea {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    height: 22
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.colorPaletteExpanded = !root.colorPaletteExpanded
                                }
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: 1
                            color: "#f0f0f0"
                        }

                        Repeater {
                            model: [
                                { "label": "\u6c14\u6ce1\u6362\u80a4", "enabled": false },
                                { "label": "\u6b4c\u8bcd\u64cd\u4f5c", "enabled": false },
                                { "label": "\u7ffb\u8bd1\u6b4c\u8bcd", "enabled": false },
                                { "label": "\u97f3\u8bd1\u6b4c\u8bcd", "enabled": false },
                                { "label": "\u65e5\u8bed\u5047\u540d\u673a\u5668\u6807\u6ce8", "enabled": false },
                                { "label": "\u663e\u793a\u7e41\u4f53\u6b4c\u8bcd", "enabled": false },
                                { "label": "\u7ad6\u6392\u663e\u793a", "enabled": false }
                            ]

                            delegate: Rectangle {
                                width: parent.width
                                height: 34
                                color: "transparent"

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                    anchors.leftMargin: 40
                                    text: modelData.label
                                    color: modelData.enabled ? "#4b5563" : "#a7a7a7"
                                    font.pixelSize: 13
                                }
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: 34
                            color: lineToggleMouse.containsMouse ? "#f6f6f6" : "transparent"

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 40
                                text: root.singleLineMode ? "\u5207\u6362\u53cc\u884c" : "\u5207\u6362\u5355\u884c"
                                color: "#4b5563"
                                font.pixelSize: 13
                            }

                            MouseArea {
                                id: lineToggleMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.toggleLineMode()
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: 34
                            color: "transparent"

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 40
                                text: "\u66f4\u591a\u8bbe\u7f6e"
                                color: "#a7a7a7"
                                font.pixelSize: 13
                            }
                        }
                    }
                }
            }

            Button {
                width: 28
                height: 28
                text: "\u00d7"
                onClicked: root.closeClicked()
                ToolTip.visible: hovered
                ToolTip.text: "\u5173\u95ed"
                background: Rectangle {
                    radius: height / 2
                    color: parent.down ? Qt.rgba(0.93, 0.25, 0.25, 0.42)
                                       : parent.hovered ? Qt.rgba(0.93, 0.25, 0.25, 0.26)
                                                        : "transparent"
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.pixelSize: 14
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    Rectangle {
        id: unlockChip
        visible: root.isLocked
        opacity: root.isLocked && root.hovered ? 1.0 : 0.0
        anchors.top: parent.top
        anchors.topMargin: 2
        anchors.horizontalCenter: parent.horizontalCenter
        radius: height / 2
        color: Qt.rgba(0.62, 0.62, 0.62, 0.88)
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.18)
        width: unlockLabel.implicitWidth + 22
        height: 26
        z: 60

        Behavior on opacity {
            NumberAnimation { duration: 120 }
        }

        HoverHandler {
            id: unlockChipHover
            onHoveredChanged: {
                if (hovered) {
                    root.refreshHoverState()
                } else {
                    root.scheduleHoverExit()
                }
            }
        }

        Text {
            id: unlockLabel
            anchors.centerIn: parent
            text: "\u89e3\u9501"
            color: "#ffffff"
            font.pixelSize: 12
            font.bold: true
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.isLocked = false
        }
    }

    Item {
        id: lyricColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 32
        anchors.rightMargin: 32
        height: root.singleLineMode ? currentLineText.height : currentLineText.height + nextLineText.height + 10

        Item {
            id: currentLineItem
            x: root.lineXFor(0)
            width: root.lineWidthFor(0)
            height: currentLineBaseText.height

            property real highlightRevealWidth: {
                if (root.currentLineProgress <= 0 || !currentLineBaseText.text || currentLineBaseText.text.trim().length === 0)
                    return 0
                var paintedWidth = currentLineBaseText.paintedWidth > 0 ? currentLineBaseText.paintedWidth : currentLineBaseText.width
                var leftInset = 0
                if (currentLineBaseText.horizontalAlignment === Text.AlignHCenter)
                    leftInset = Math.max(0, (currentLineBaseText.width - paintedWidth) / 2)
                else if (currentLineBaseText.horizontalAlignment === Text.AlignRight)
                    leftInset = Math.max(0, currentLineBaseText.width - paintedWidth)
                return Math.min(currentLineBaseText.width, leftInset + paintedWidth * root.currentLineProgress)
            }

            DropShadow {
                anchors.fill: currentLineBaseText
                source: currentLineBaseText
                radius: 18
                samples: 33
                color: Qt.rgba(0, 0, 0, 0.44)
                horizontalOffset: 4
                verticalOffset: 5
            }

            Text {
                id: currentLineBaseText
                width: parent.width
                text: root.currentLyricText
                color: root.unplayedLyricColor
                opacity: root.lyricOpacity / 100.0
                font.family: root.lyricFontFamily
                font.pixelSize: root.lyricFontSize
                font.weight: root.lyricBold ? Font.Black : Font.DemiBold
                style: Text.Outline
                styleColor: root.strokeColor
                wrapMode: Text.NoWrap
                elide: Text.ElideRight
                horizontalAlignment: root.textAlignmentForLine(0)
                verticalAlignment: Text.AlignVCenter
                renderType: Text.NativeRendering
            }

            Item {
                x: currentLineBaseText.x
                y: currentLineBaseText.y
                width: currentLineItem.highlightRevealWidth
                height: currentLineBaseText.height
                clip: true
                visible: width > 0

                Text {
                    id: currentLineText
                    width: currentLineBaseText.width
                    height: currentLineBaseText.height
                    text: currentLineBaseText.text
                    color: root.playedLyricColor
                    opacity: root.lyricOpacity / 100.0
                    font.family: currentLineBaseText.font.family
                    font.pixelSize: currentLineBaseText.font.pixelSize
                    font.weight: currentLineBaseText.font.weight
                    style: Text.Outline
                    styleColor: root.strokeColor
                    wrapMode: currentLineBaseText.wrapMode
                    elide: currentLineBaseText.elide
                    horizontalAlignment: currentLineBaseText.horizontalAlignment
                    verticalAlignment: currentLineBaseText.verticalAlignment
                    renderType: Text.NativeRendering
                }
            }
        }

        Item {
            id: nextLineItem
            visible: !root.singleLineMode && root.nextLyricText.length > 0
            y: currentLineItem.height + 10
            x: root.lineXFor(1)
            width: root.lineWidthFor(1)
            height: nextLineText.height

            DropShadow {
                anchors.fill: nextLineText
                source: nextLineText
                radius: 18
                samples: 33
                color: Qt.rgba(0, 0, 0, 0.40)
                horizontalOffset: 4
                verticalOffset: 5
            }

            Text {
                id: nextLineText
                width: parent.width
                text: root.nextLyricText
                color: root.unplayedLyricColor
                opacity: root.lyricOpacity / 100.0
                font.family: root.lyricFontFamily
                font.pixelSize: root.lyricFontSize
                font.weight: root.lyricBold ? Font.Black : Font.DemiBold
                style: Text.Outline
                styleColor: root.strokeColor
                wrapMode: Text.NoWrap
                elide: Text.ElideRight
                horizontalAlignment: root.textAlignmentForLine(1)
                verticalAlignment: Text.AlignVCenter
                renderType: Text.NativeRendering
            }
        }
    }

    Rectangle {
        id: resizeHandle
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 5
        anchors.bottomMargin: 5
        width: 10
        height: 10
        radius: 0
        visible: root.hovered
        color: Qt.rgba(0.48, 0.56, 0.94, 0.20)
        border.width: 1
        border.color: Qt.rgba(0.48, 0.56, 0.94, 0.55)
    }

    MouseArea {
        id: resizeMouseArea
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 18
        height: 18
        visible: root.hovered
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeFDiagCursor

        property point startPos
        property size startSize

        onEntered: root.refreshHoverState()
        onExited: root.scheduleHoverExit()

        onPressed: {
            if (mouse.button === Qt.LeftButton) {
                root.isResizing = true
                startPos = Qt.point(mouse.x, mouse.y)
                startSize = Qt.size(root.width, root.height)
            }
        }

        onPositionChanged: {
            if (!root.isResizing) {
                return
            }

            var deltaX = mouse.x - startPos.x
            var deltaY = mouse.y - startPos.y
            var newWidth = Math.max(root.minWidth, Math.min(root.maxWidth, startSize.width + deltaX))
            var newHeight = Math.max(root.minHeight, Math.min(root.maxHeight, startSize.height + deltaY))
            root.windowResized(Qt.size(newWidth, newHeight))
        }

        onReleased: root.isResizing = false
    }

    ColorDialog {
        id: colorDialog
        title: "\u684c\u9762\u6b4c\u8bcd\u989c\u8272"
        onAccepted: root.applySelectedColor(color)
    }

    Component.onCompleted: scheduleAutoResize()
}
