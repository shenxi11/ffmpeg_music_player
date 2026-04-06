import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../library" as Library
import "../../theme/Theme.js" as Theme

Rectangle {
    id: root
    color: Theme.bgBase

    property bool isLoggedIn: false
    property string userAccount: ""
    property string currentPlayingPath: ""
    property bool isPlaying: false
    property int sideMargin: width >= 1200 ? 20 : 14
    property int innerMargin: width >= 1200 ? 16 : 12
    property int colGap: width >= 1200 ? 10 : 8
    property int colIndexWidth: 42
    property int colCoverWidth: 44
    property int colDurationWidth: width >= 1280 ? 88 : (width >= 960 ? 80 : 70)
    property int colTimeWidth: width >= 1280 ? 150 : (width >= 960 ? 130 : 104)
    property int colActionWidth: 106
    property string listIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/list/"
    property string playerIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/player/"
    property int colTitleWidth: Math.max(140,
                                         width - sideMargin * 2 - innerMargin * 2
                                         - colIndexWidth - colCoverWidth - colDurationWidth
                                         - colTimeWidth - colActionWidth - colGap * 4)

    signal playMusic(string filename)
    signal playMusicWithMetadata(string filePath, string title, string artist, string cover)
    signal addToFavorite(string filePath, string title, string artist, string duration, bool isLocal)
    signal deleteHistory(var selectedPaths)
    signal loginRequested()
    signal refreshRequested()
    signal songActionRequested(string action, var song)

    function looksUnreadable(value) {
        if (value === undefined || value === null) return true
        var text = String(value).trim()
        if (text.length === 0) return true
        if (/^[\?？\s]+$/.test(text)) return true
        return text.indexOf("\uFFFD") >= 0
    }

    function baseNameFromPath(path) {
        if (!path) return ""
        var value = String(path)
        var qPos = value.indexOf("?")
        if (qPos >= 0) value = value.substring(0, qPos)
        var slash = value.lastIndexOf("/")
        var name = slash >= 0 ? value.substring(slash + 1) : value
        var dot = name.lastIndexOf(".")
        if (dot > 0) name = name.substring(0, dot)
        try {
            name = decodeURIComponent(name)
        } catch (e) {
        }
        return name
    }

    function normalizeText(value, fallbackText) {
        if (looksUnreadable(value)) return fallbackText
        return String(value)
    }

    function displayTitle(item) {
        var title = normalizeText(item.title, "")
        if (title.length > 0) return title
        var fromPath = normalizeText(baseNameFromPath(item.path), "")
        return fromPath.length > 0 ? fromPath : "未知歌曲"
    }

    function displayArtist(item) {
        return normalizeText(item.artist, "未知艺术家")
    }

    function normalizePath(path) {
        if (!path) return ""
        var value = String(path).trim()
        if (value.indexOf("file:///") === 0) {
            value = value.substring(8)
        }
        var qPos = value.indexOf("?")
        if (qPos > 0) {
            value = value.substring(0, qPos)
        }
        try {
            value = decodeURIComponent(value)
        } catch (e) {
        }
        return value
    }

    function isSameTrack(pathA, pathB) {
        return normalizePath(pathA) === normalizePath(pathB)
    }

    function buildSongPayload(item) {
        return {
            path: item.path || "",
            playPath: item.path || "",
            title: displayTitle(item),
            artist: displayArtist(item),
            cover: item.cover_art_url || "",
            duration: item.duration || "",
            sourceType: "history",
            isLocal: true
        }
    }

    function setPlayingState(filePath, playing) {
        root.currentPlayingPath = filePath || ""
        root.isPlaying = playing
    }

    ListModel {
        id: historyModel
    }

    property var selectedItems: []

    Rectangle {
        anchors.fill: parent
        visible: !root.isLoggedIn
        color: "transparent"

        Column {
            anchors.centerIn: parent
            spacing: 16

            Rectangle {
                width: 72
                height: 72
                radius: 36
                color: Theme.accentSoft
                anchors.horizontalCenter: parent.horizontalCenter

                Text {
                    anchors.centerIn: parent
                    text: "♪"
                    font.pixelSize: 34
                    font.bold: true
                    color: Theme.accent
                }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "登录后查看最近播放"
                font.pixelSize: 20
                font.weight: Font.DemiBold
                color: Theme.textPrimary
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "最近播放支持多端同步"
                font.pixelSize: 13
                color: Theme.textSecondary
            }

            Rectangle {
                width: 140
                height: 36
                radius: 18
                anchors.horizontalCenter: parent.horizontalCenter
                color: loginArea.containsMouse ? Theme.accent : "transparent"
                border.width: 1
                border.color: Theme.accent

                Text {
                    anchors.centerIn: parent
                    text: "立即登录"
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: loginArea.containsMouse ? "#FFFFFF" : Theme.accent
                }

                MouseArea {
                    id: loginArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.loginRequested()
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        visible: root.isLoggedIn

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 54
            Layout.leftMargin: root.sideMargin
            Layout.rightMargin: root.sideMargin
            radius: 12
            color: Theme.glassLight
            border.width: 1
            border.color: Theme.glassBorder

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 12
                spacing: 10

                Text {
                    text: "播放历史"
                    font.pixelSize: 19
                    font.weight: Font.DemiBold
                    color: Theme.textPrimary
                    Layout.fillWidth: true
                }

                Rectangle {
                    width: 84
                    height: 32
                    radius: 16
                    color: refreshArea.containsMouse ? Theme.accent : "transparent"
                    border.width: 1
                    border.color: Theme.accent

                    Text {
                        anchors.centerIn: parent
                        text: "刷新"
                        font.pixelSize: 12
                        color: refreshArea.containsMouse ? "#FFFFFF" : Theme.accent
                    }

                    MouseArea {
                        id: refreshArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.refreshRequested()
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            Layout.leftMargin: root.sideMargin
            Layout.rightMargin: root.sideMargin
            radius: 10
            color: Theme.glassLight
            border.width: 1
            border.color: Theme.glassBorder

            Row {
                anchors.fill: parent
                anchors.leftMargin: root.innerMargin
                anchors.rightMargin: root.innerMargin
                spacing: root.colGap

                Text {
                    text: "#"
                    width: root.colIndexWidth
                    font.pixelSize: 12
                    color: Theme.textSecondary
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item {
                    width: root.colCoverWidth
                    height: 1
                }

                Text {
                    text: "音频信息"
                    width: root.colTitleWidth
                    font.pixelSize: 12
                    color: Theme.textSecondary
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: "时长"
                    width: root.colDurationWidth
                    font.pixelSize: 12
                    color: Theme.textSecondary
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: "播放时间"
                    width: root.colTimeWidth
                    font.pixelSize: 12
                    color: Theme.textSecondary
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item {
                    width: root.colActionWidth
                    height: 1
                }
            }
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: root.sideMargin
            Layout.rightMargin: root.sideMargin
            spacing: 6
            clip: true
            model: historyModel

            delegate: Rectangle {
                id: itemRoot
                width: listView.width
                height: 62
                radius: 10
                property bool isCurrentTrack: root.isSameTrack(root.currentPlayingPath, model.path)
                property bool playbackActive: isCurrentTrack && root.isPlaying
                property bool rowVisualHovered: rowHoverArea.containsMouse
                                                 || coverAction.interactionActive
                                                 || actionRowHover.hovered
                property bool coverVisualHovered: rowHoverArea.containsMouse
                                                   || coverAction.interactionActive

                color: isCurrentTrack
                       ? "#FDECEC"
                       : (rowVisualHovered ? "#F8FAFF" : (index % 2 === 0 ? Theme.bgCard : "#FCFCFD"))
                border.width: isCurrentTrack ? 1 : 0
                border.color: isCurrentTrack ? Theme.accent : "transparent"

                MouseArea {
                    id: rowHoverArea
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.NoButton
                }

                Rectangle {
                    visible: itemRoot.isCurrentTrack
                    width: 3
                    radius: 2
                    color: Theme.accent
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.margins: 10
                }

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: root.innerMargin
                    anchors.rightMargin: root.innerMargin
                    spacing: root.colGap

                    Text {
                        text: (index + 1).toString()
                        width: root.colIndexWidth
                        font.pixelSize: 13
                        color: Theme.textSecondary
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Library.SongCoverAction {
                        id: coverAction
                        width: root.colCoverWidth
                        height: 44
                        anchors.verticalCenter: parent.verticalCenter
                        z: 2
                        rowHovered: itemRoot.coverVisualHovered
                        isCurrentTrack: itemRoot.isCurrentTrack
                        isPlaying: itemRoot.playbackActive
                        coverSource: model.cover_art_url || ""
                        fallbackSource: "qrc:/new/prefix1/icon/Music.png"

                        onPlayRequested: {
                            if (itemRoot.isCurrentTrack) {
                                root.songActionRequested("toggle_current_playback", root.buildSongPayload(model))
                                return
                            }
                            var filePath = model.path || ""
                            var title = root.displayTitle(model)
                            var artist = root.displayArtist(model)
                            var cover = model.cover_art_url || ""
                            root.playMusicWithMetadata(filePath, title, artist, cover)
                        }

                        onPauseRequested: {
                            root.songActionRequested("toggle_current_playback", root.buildSongPayload(model))
                        }
                    }

                    Column {
                        width: root.colTitleWidth
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 4

                        Text {
                            text: root.displayTitle(model)
                            font.pixelSize: 14
                            font.bold: itemRoot.isCurrentTrack
                            color: itemRoot.isCurrentTrack ? Theme.accent : Theme.textPrimary
                            elide: Text.ElideRight
                            width: parent.width
                        }

                        Text {
                            text: root.displayArtist(model)
                            font.pixelSize: 11
                            color: Theme.textSecondary
                            elide: Text.ElideRight
                            width: parent.width
                        }
                    }

                    Text {
                        text: model.duration || "--:--"
                        width: root.colDurationWidth
                        font.pixelSize: 12
                        color: Theme.textSecondary
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: model.play_time || ""
                        width: root.colTimeWidth
                        font.pixelSize: 12
                        color: Theme.textSecondary
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Row {
                        id: actionRow
                        width: root.colActionWidth
                        spacing: 8
                        anchors.verticalCenter: parent.verticalCenter
                        opacity: itemRoot.rowVisualHovered ? 1.0 : 0.0
                        enabled: itemRoot.rowVisualHovered

                        HoverHandler {
                            id: actionRowHover
                        }

                        Rectangle {
                            width: 32
                            height: 32
                            radius: 16
                            color: favBtnArea.containsMouse ? Theme.accentSoft : "transparent"
                            border.width: 1
                            border.color: favBtnArea.containsMouse ? Theme.accent : "#D6DCE8"

                            Image {
                                anchors.centerIn: parent
                                width: 18
                                height: 18
                                source: favBtnArea.containsMouse
                                        ? root.listIconPrefix + "list_icon_favorite_hover.svg"
                                        : root.listIconPrefix + "list_icon_favorite_default.svg"
                                fillMode: Image.PreserveAspectFit
                            }

                            MouseArea {
                                id: favBtnArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    var filePath = model.path || ""
                                    var title = root.displayTitle(model)
                                    var artist = root.displayArtist(model)
                                    var duration = model.duration || "0:00"
                                    var isLocal = !(filePath.indexOf("http://") === 0 || filePath.indexOf("https://") === 0)
                                    root.addToFavorite(filePath, title, artist, duration, isLocal)
                                }
                            }
                        }

                        Rectangle {
                            width: 32
                            height: 32
                            radius: 16
                            color: deleteBtnArea.containsMouse ? Theme.accentSoft : "transparent"
                            border.width: 1
                            border.color: deleteBtnArea.containsMouse ? Theme.accent : "#D6DCE8"

                            Image {
                                anchors.centerIn: parent
                                width: 18
                                height: 18
                                source: deleteBtnArea.containsMouse
                                        ? root.listIconPrefix + "list_icon_delete_hover.svg"
                                        : root.listIconPrefix + "list_icon_delete_default.svg"
                                fillMode: Image.PreserveAspectFit
                            }

                            MouseArea {
                                id: deleteBtnArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.deleteHistory([model.path])
                            }
                        }
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    onDoubleTapped: {
                        var filePath = model.path || ""
                        var title = root.displayTitle(model)
                        var artist = root.displayArtist(model)
                        var cover = model.cover_art_url || ""
                        root.playMusicWithMetadata(filePath, title, artist, cover)
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                width: 8
            }

            Label {
                anchors.centerIn: parent
                text: "暂无播放历史"
                font.pixelSize: 16
                color: Theme.textSecondary
                visible: listView.count === 0
            }
        }
    }

    function loadHistory(historyData) {
        historyModel.clear()
        root.selectedItems = []

        for (var i = 0; i < historyData.length; i++) {
            var item = historyData[i]
            if (!item.artist || item.artist.length === 0) {
                item.artist = item.singer || item.author || item.artist_name || ""
            }
            item.title = root.displayTitle(item)
            item.artist = root.displayArtist(item)
            historyModel.append(item)
        }
    }

    function clearHistory() {
        historyModel.clear()
        root.selectedItems = []
    }
}
