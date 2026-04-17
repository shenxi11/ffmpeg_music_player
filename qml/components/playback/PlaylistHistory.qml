import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.14

// 播放列表面板：数据来源于真实播放队列快照，而不是本地 append 历史。
Rectangle {
    id: root
    width: 360
    height: 720
    color: "transparent"

    signal playRequested(string filePath)
    signal removeRequested(string filePath)
    signal clearAllRequested()
    signal pauseToggled()

    property string currentPlayingPath: ""
    property bool isPaused: false
    property string uiFontFamily: "Microsoft YaHei UI"
    property color accentColor: "#EC4141"
    property color panelColor: "#FFFFFF"
    property color lineColor: "#EEF1F5"
    property bool menuVisible: false
    property real menuAnchorX: 0
    property real menuAnchorY: 0
    property var menuPayload: ({})
    property string pendingActionKind: ""
    property string pendingActionPath: ""

    function looksUnreadable(value) {
        if (value === undefined || value === null)
            return true
        var text = String(value).trim()
        if (text.length === 0)
            return true
        if (/^[\?？\s]+$/.test(text))
            return true
        return text.indexOf("\uFFFD") >= 0
    }

    function baseNameFromPath(path) {
        if (!path)
            return ""
        var value = String(path)
        var qPos = value.indexOf("?")
        if (qPos >= 0)
            value = value.substring(0, qPos)
        var slash = value.lastIndexOf("/")
        var name = slash >= 0 ? value.substring(slash + 1) : value
        var dot = name.lastIndexOf(".")
        if (dot > 0)
            name = name.substring(0, dot)
        try {
            name = decodeURIComponent(name)
        } catch (e) {
        }
        return name
    }

    function normalizeText(value, fallbackText) {
        if (looksUnreadable(value))
            return fallbackText
        return String(value)
    }

    function displayTitle(item) {
        var title = normalizeText(item.title, "")
        if (title.length > 0)
            return title
        var fromPath = normalizeText(baseNameFromPath(item.filePath), "")
        return fromPath.length > 0 ? fromPath : "未知歌曲"
    }

    function displayArtist(item) {
        return normalizeText(item.artist, "未知艺术家")
    }

    function isCurrentEntry(item) {
        return !!item && (!!item.isCurrent || item.filePath === currentPlayingPath)
    }

    function stateBadgeText(item) {
        if (!isCurrentEntry(item))
            return ""
        return isPaused ? "已暂停" : "正在播放"
    }

    function stateBadgeColor(item) {
        if (!isCurrentEntry(item))
            return "transparent"
        return isPaused ? "#FFF5E8" : "#FFF1F1"
    }

    function stateBadgeTextColor(item) {
        if (!isCurrentEntry(item))
            return "#7B8597"
        return isPaused ? "#C27A2C" : accentColor
    }

    function menuPayloadFrom(item) {
        return {
            filePath: item && item.filePath ? String(item.filePath) : "",
            title: item && item.title ? String(item.title) : "",
            artist: item && item.artist ? String(item.artist) : "",
            cover: item && item.cover ? String(item.cover) : "",
            isCurrent: !!(item && item.isCurrent)
        }
    }

    function queueHintText() {
        if (playlistModel.count <= 0)
            return "播放顺序将跟随你接下来添加的歌曲"
        return "播放顺序跟随当前队列，可随时切换或移除"
    }

    function openMenu(item, anchorX, anchorY) {
        menuPayload = menuPayloadFrom(item)
        menuAnchorX = anchorX
        menuAnchorY = anchorY
        menuVisible = true
    }

    function closeMenu() {
        menuVisible = false
        menuPayload = ({})
    }

    function scheduleAction(actionKind, filePath) {
        pendingActionKind = actionKind || ""
        pendingActionPath = filePath || ""
        deferredActionTimer.restart()
    }

    function triggerPrimaryAction(item) {
        if (!item || !item.filePath)
            return
        closeMenu()
        if (isCurrentEntry(item)) {
            scheduleAction("pause", "")
        } else {
            scheduleAction("play", String(item.filePath))
        }
    }

    function menuPlayLabel(item) {
        if (isCurrentEntry(item))
            return isPaused ? "继续播放" : "暂停播放"
        return "立即播放"
    }

    function menuEntriesFor(item) {
        return [
            { key: "play", label: menuPlayLabel(item), enabled: !!item.filePath, destructive: false, separatorBefore: false },
            { key: "next", label: "下一首播放", enabled: false, destructive: false, separatorBefore: false },
            { key: "similar", label: "播放相似单曲", enabled: false, destructive: false, separatorBefore: false },
            { key: "comment", label: "查看评论", enabled: false, destructive: false, separatorBefore: true },
            { key: "add", label: "添加到歌单", enabled: false, destructive: false, separatorBefore: false },
            { key: "share", label: "分享歌曲", enabled: false, destructive: false, separatorBefore: false },
            { key: "remove", label: "从队列中移除", enabled: !!item.filePath, destructive: true, separatorBefore: true }
        ]
    }

    function handleMenuAction(actionKey) {
        var item = menuPayload
        closeMenu()
        if (!item || !item.filePath)
            return

        if (actionKey === "play") {
            triggerPrimaryAction(item)
        } else if (actionKey === "remove") {
            scheduleAction("remove", String(item.filePath))
        }
    }

    Timer {
        id: deferredActionTimer
        interval: 0
        repeat: false
        onTriggered: {
            var actionKind = root.pendingActionKind
            var filePath = root.pendingActionPath
            root.pendingActionKind = ""
            root.pendingActionPath = ""

            if (actionKind === "pause") {
                root.pauseToggled()
            } else if (actionKind === "play" && filePath.length > 0) {
                root.playRequested(filePath)
            } else if (actionKind === "remove" && filePath.length > 0) {
                root.removeRequested(filePath)
            } else if (actionKind === "clearAll") {
                root.clearAllRequested()
            }
        }
    }

    Rectangle {
        id: panel
        anchors.fill: parent
        color: panelColor
        radius: 0
        clip: true
        layer.enabled: true
        layer.effect: DropShadow {
            horizontalOffset: -10
            verticalOffset: 0
            radius: 24
            samples: 49
            color: "#18000000"
        }

        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: lineColor
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 116
                color: panelColor

                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 22
                    anchors.rightMargin: 18
                    anchors.topMargin: 22
                    anchors.bottomMargin: 14
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Text {
                            text: "播放队列"
                            font.family: uiFontFamily
                            font.pixelSize: 22
                            font.weight: Font.DemiBold
                            color: "#16181D"
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            id: clearButton
                            Layout.preferredWidth: 72
                            Layout.preferredHeight: 30
                            radius: 15
                            color: clearMouse.containsMouse && playlistModel.count > 0 ? "#F3F5F8" : "transparent"
                            border.width: 1
                            border.color: playlistModel.count > 0 ? "#E4E8EE" : "#EDF0F4"
                            opacity: playlistModel.count > 0 ? 1.0 : 0.45

                            Text {
                                anchors.centerIn: parent
                                text: "清空"
                                font.family: uiFontFamily
                                font.pixelSize: 13
                                font.weight: Font.Medium
                                color: "#5C6575"
                            }

                            MouseArea {
                                id: clearMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                enabled: playlistModel.count > 0
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.closeMenu()
                                    root.scheduleAction("clearAll", "")
                                }
                            }
                        }
                    }

                    Text {
                        text: playlistModel.count > 0 ? ("共 " + playlistModel.count + " 首歌曲") : "当前没有待播放歌曲"
                        font.family: uiFontFamily
                        font.pixelSize: 13
                        color: "#626C7D"
                    }

                    Text {
                        text: queueHintText()
                        font.family: uiFontFamily
                        font.pixelSize: 12
                        color: "#98A1B2"
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: lineColor
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ListView {
                    id: listView
                    anchors.fill: parent
                    anchors.leftMargin: 0
                    anchors.rightMargin: 0
                    anchors.topMargin: 8
                    anchors.bottomMargin: 8
                    clip: true
                    spacing: 0
                    boundsBehavior: Flickable.StopAtBounds
                    visible: playlistModel.count > 0

                    model: ListModel {
                        id: playlistModel
                    }

                    delegate: Item {
                        id: delegateRoot
                        width: listView.width
                        height: 82

                        readonly property bool currentRow: root.isCurrentEntry(model)
                        readonly property bool actionVisible: rowMouse.containsMouse || currentRow ||
                                                             (root.menuVisible && root.menuPayload.filePath === model.filePath)

                        Rectangle {
                            anchors.left: parent.left
                            anchors.leftMargin: 14
                            anchors.right: parent.right
                            anchors.rightMargin: 14
                            anchors.top: parent.top
                            anchors.topMargin: 4
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 4
                            radius: 16
                            color: currentRow ? "#FFF5F5" : (rowMouse.containsMouse ? "#F6F8FA" : (index % 2 === 0 ? "#FCFCFD" : "#FFFFFF"))
                            border.width: currentRow ? 1 : 0
                            border.color: currentRow ? "#F7D2D2" : "transparent"

                            Rectangle {
                                anchors.left: parent.left
                                anchors.leftMargin: 0
                                anchors.top: parent.top
                                anchors.topMargin: 18
                                width: 3
                                height: parent.height - 36
                                radius: 2
                                color: currentRow ? accentColor : "transparent"
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 18
                                anchors.rightMargin: 16
                                spacing: 12

                                Rectangle {
                                    id: coverFrame
                                    Layout.preferredWidth: 48
                                    Layout.preferredHeight: 48
                                    radius: 12
                                    color: "#EDEFF3"
                                    clip: true

                                    Image {
                                        anchors.fill: parent
                                        source: (model.cover && model.cover.length > 0)
                                                ? model.cover
                                                : "qrc:/qml/assets/ai/icons/default-music-cover.svg"
                                        fillMode: Image.PreserveAspectCrop
                                        asynchronous: true
                                        cache: true
                                        sourceSize.width: 48
                                        sourceSize.height: 48
                                    }

                                    Rectangle {
                                        anchors.fill: parent
                                        color: (delegateRoot.actionVisible || (delegateRoot.currentRow && !root.isPaused)) ? "#55000000" : "transparent"
                                    }

                                    Canvas {
                                        id: coverStateCanvas
                                        anchors.centerIn: parent
                                        width: 18
                                        height: 18
                                        visible: delegateRoot.actionVisible || delegateRoot.currentRow
                                        onPaint: {
                                            var ctx = getContext("2d")
                                            ctx.clearRect(0, 0, width, height)
                                            ctx.fillStyle = "#FFFFFF"
                                            if (delegateRoot.currentRow && !root.isPaused) {
                                                ctx.fillRect(4, 3, 3, 12)
                                                ctx.fillRect(10, 3, 3, 12)
                                            } else {
                                                ctx.beginPath()
                                                ctx.moveTo(5, 3)
                                                ctx.lineTo(15, 9)
                                                ctx.lineTo(5, 15)
                                                ctx.closePath()
                                                ctx.fill()
                                            }
                                        }

                                        Connections {
                                            target: root
                                            function onCurrentPlayingPathChanged() {
                                                coverStateCanvas.requestPaint()
                                            }
                                            function onIsPausedChanged() {
                                                coverStateCanvas.requestPaint()
                                            }
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.triggerPrimaryAction(model)
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        Text {
                                            Layout.fillWidth: true
                                            text: root.displayTitle(model)
                                            font.family: uiFontFamily
                                            font.pixelSize: 15
                                            font.weight: delegateRoot.currentRow ? Font.DemiBold : Font.Medium
                                            color: delegateRoot.currentRow ? accentColor : "#1C212B"
                                            elide: Text.ElideRight
                                        }

                                        Rectangle {
                                            visible: delegateRoot.currentRow
                                            radius: 9
                                            color: root.stateBadgeColor(model)
                                            Layout.preferredHeight: 20
                                            Layout.preferredWidth: badgeLabel.implicitWidth + 14

                                            Text {
                                                id: badgeLabel
                                                anchors.centerIn: parent
                                                text: root.stateBadgeText(model)
                                                font.family: uiFontFamily
                                                font.pixelSize: 11
                                                font.weight: Font.DemiBold
                                                color: root.stateBadgeTextColor(model)
                                            }
                                        }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: root.displayArtist(model)
                                        font.family: uiFontFamily
                                        font.pixelSize: 12
                                        color: "#8E97A7"
                                        elide: Text.ElideRight
                                    }
                                }

                                RowLayout {
                                    spacing: 8
                                    visible: delegateRoot.actionVisible
                                    opacity: delegateRoot.actionVisible ? 1 : 0

                                    Rectangle {
                                        width: 28
                                        height: 28
                                        radius: 14
                                        color: linkMouse.containsMouse ? "#F1F4F8" : "#FFFFFF"
                                        border.width: 1
                                        border.color: "#E5E9F0"
                                        opacity: 0.75

                                        Canvas {
                                            anchors.centerIn: parent
                                            width: 14
                                            height: 14
                                            onPaint: {
                                                var ctx = getContext("2d")
                                                ctx.clearRect(0, 0, width, height)
                                                ctx.lineWidth = 1.5
                                                ctx.strokeStyle = "#9AA3B2"
                                                ctx.beginPath()
                                                ctx.arc(4.5, 7, 3, -0.8, 0.8, false)
                                                ctx.stroke()
                                                ctx.beginPath()
                                                ctx.arc(9.5, 7, 3, 2.3, 3.95, false)
                                                ctx.stroke()
                                                ctx.beginPath()
                                                ctx.moveTo(5.5, 7)
                                                ctx.lineTo(8.5, 7)
                                                ctx.stroke()
                                            }
                                        }

                                        MouseArea {
                                            id: linkMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            enabled: false
                                        }
                                    }

                                    Rectangle {
                                        id: moreButton
                                        width: 28
                                        height: 28
                                        radius: 14
                                        color: moreMouse.containsMouse || (root.menuVisible && root.menuPayload.filePath === model.filePath) ? "#F1F4F8" : "#FFFFFF"
                                        border.width: 1
                                        border.color: "#E5E9F0"

                                        Canvas {
                                            anchors.centerIn: parent
                                            width: 16
                                            height: 16
                                            onPaint: {
                                                var ctx = getContext("2d")
                                                ctx.clearRect(0, 0, width, height)
                                                ctx.fillStyle = "#5C6575"
                                                for (var i = 0; i < 3; ++i) {
                                                    ctx.beginPath()
                                                    ctx.arc(4 + i * 4, 8, 1.4, 0, Math.PI * 2, false)
                                                    ctx.fill()
                                                }
                                            }
                                        }

                                        MouseArea {
                                            id: moreMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                var point = moreButton.mapToItem(root, moreButton.width, moreButton.height)
                                                root.openMenu(model, point.x, point.y)
                                            }
                                        }
                                    }
                                }
                            }

                            MouseArea {
                                id: rowMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                acceptedButtons: Qt.LeftButton
                                z: -1
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.triggerPrimaryAction(model)
                            }
                        }
                    }

                    ScrollBar.vertical: ScrollBar {
                        width: 6
                        policy: ScrollBar.AsNeeded
                        background: Item { }
                        contentItem: Rectangle {
                            radius: width / 2
                            color: "#D9DEE6"
                        }
                    }
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 10
                    visible: playlistModel.count === 0

                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 54
                        height: 54
                        radius: 27
                        color: "#F4F6F9"
                        border.width: 1
                        border.color: "#E7EBF1"

                        Canvas {
                            anchors.centerIn: parent
                            width: 24
                            height: 24
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)
                                ctx.strokeStyle = "#B0B8C6"
                                ctx.lineWidth = 1.6
                                ctx.beginPath()
                                ctx.moveTo(4, 7)
                                ctx.lineTo(20, 7)
                                ctx.moveTo(4, 12)
                                ctx.lineTo(20, 12)
                                ctx.moveTo(4, 17)
                                ctx.lineTo(14, 17)
                                ctx.stroke()
                            }
                        }
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "当前队列为空"
                        font.family: uiFontFamily
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        color: "#556072"
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "从列表中播放歌曲后，这里会显示接下来的播放顺序"
                        font.family: uiFontFamily
                        font.pixelSize: 12
                        color: "#9AA3B2"
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        visible: root.menuVisible
        z: 20
        acceptedButtons: Qt.AllButtons
        onClicked: root.closeMenu()
    }

    Rectangle {
        id: menuPanel
        visible: root.menuVisible
        z: 21
        width: 204
        radius: 14
        color: "#FFFFFF"
        border.width: 1
        border.color: "#EFF2F6"
        x: Math.max(12, Math.min(root.width - width - 12, root.menuAnchorX - width))
        y: Math.max(12, Math.min(root.height - height - 12, root.menuAnchorY + 8))
        layer.enabled: true
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: 8
            radius: 20
            samples: 41
            color: "#24000000"
        }

        implicitHeight: menuColumn.implicitHeight + 12
        height: implicitHeight

        Column {
            id: menuColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 6
            anchors.bottomMargin: 6
            spacing: 0

            Repeater {
                model: root.menuEntriesFor(root.menuPayload)

                delegate: Item {
                    property var entry: modelData
                    width: menuColumn.width
                    height: (separatorRect.visible ? 7 : 0) + 36

                    Rectangle {
                        id: separatorRect
                        visible: !!entry.separatorBefore
                        anchors.left: parent.left
                        anchors.leftMargin: 14
                        anchors.right: parent.right
                        anchors.rightMargin: 14
                        anchors.top: parent.top
                        height: 1
                        color: "#EEF1F5"
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: separatorRect.visible ? separatorRect.bottom : parent.top
                        anchors.topMargin: separatorRect.visible ? 6 : 0
                        height: 36
                        color: menuItemMouse.containsMouse && entry.enabled ? "#F5F7FA" : "#FFFFFF"
                        opacity: entry.enabled ? 1.0 : 0.45

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 16
                            anchors.verticalCenter: parent.verticalCenter
                            text: entry.label
                            font.family: uiFontFamily
                            font.pixelSize: 13
                            font.weight: Font.Medium
                            color: entry.destructive ? accentColor : "#2B3240"
                        }

                        MouseArea {
                            id: menuItemMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            enabled: entry.enabled
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.handleMenuAction(entry.key)
                        }
                    }
                }
            }
        }
    }

    function loadPlaylist(items) {
        playlistModel.clear()
        closeMenu()
        if (!items)
            return
        for (var i = 0; i < items.length; ++i) {
            playlistModel.append(items[i])
        }
    }

    function addSong(filePath, title, artist, cover) {
        var foundIndex = -1
        for (var i = 0; i < playlistModel.count; ++i) {
            if (playlistModel.get(i).filePath === filePath) {
                foundIndex = i
                break
            }
        }

        var payload = {
            filePath: filePath,
            title: title,
            artist: artist,
            cover: cover,
            isCurrent: filePath === currentPlayingPath
        }

        if (foundIndex >= 0) {
            playlistModel.set(foundIndex, payload)
        } else {
            playlistModel.append(payload)
        }
    }

    function clearAll() {
        closeMenu()
        playlistModel.clear()
    }
}
