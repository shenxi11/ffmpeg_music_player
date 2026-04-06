import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../theme/Theme.js" as Theme
import "../library" as Library

Rectangle {
    id: root
    color: Theme.bgBase

    property string userAccount: ""
    property string currentPlayingPath: ""
    property int sideMargin: width >= 1200 ? 20 : 14
    property int innerMargin: width >= 1200 ? 16 : 12
    property int colGap: width >= 1200 ? 10 : 8
    property int colIndexWidth: 42
    property int colCoverWidth: 44
    property int colDurationWidth: width >= 1280 ? 88 : (width >= 960 ? 80 : 70)
    property int colTimeWidth: width >= 1280 ? 150 : (width >= 960 ? 130 : 104)
    property int colActionWidth: 160
    property var availablePlaylists: []
    property var favoritePaths: []
    property string listIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/list/"
    property string playerIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/player/"
    property int colTitleWidth: Math.max(140,
                                         width - sideMargin * 2 - innerMargin * 2
                                         - colIndexWidth - colCoverWidth - colDurationWidth
                                         - colTimeWidth - colActionWidth - colGap * 4)

    signal playMusic(string filename)
    signal removeFavorite(var selectedPaths)
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

    function buildSongPayload(item) {
        return {
            path: item.path || "",
            playPath: item.path || "",
            title: displayTitle(item),
            artist: displayArtist(item),
            duration: item.duration || "0:00",
            cover: item.cover_art_url || "",
            isLocal: !!item.is_local,
            isFavorite: true,
            sourceType: "favorite"
        }
    }

    ListModel {
        id: favoriteModel
    }

    property var selectedItems: []

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

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
                    text: "我喜欢的音乐"
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
                    text: "添加时间"
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
            model: favoriteModel

            delegate: Rectangle {
                id: itemRoot
                width: listView.width
                height: 62
                radius: 10
                property bool rowHovered: rowHoverHandler.hovered || actionStrip.interactionActive
                property bool isPlaying: root.currentPlayingPath === model.path

                color: isPlaying
                       ? "#FDECEC"
                       : (rowHovered ? "#F8FAFF" : (index % 2 === 0 ? Theme.bgCard : "#FCFCFD"))
                border.width: isPlaying ? 1 : 0
                border.color: isPlaying ? Theme.accent : "transparent"

                HoverHandler {
                    id: rowHoverHandler
                }

                Rectangle {
                    visible: itemRoot.isPlaying
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

                    Rectangle {
                        width: root.colCoverWidth
                        height: 44
                        anchors.verticalCenter: parent.verticalCenter
                        radius: 6
                        color: "#E9ECF5"
                        border.width: 1
                        border.color: "#D9DFEA"

                        Image {
                            anchors.fill: parent
                            anchors.margins: 2
                            source: {
                                var url = model.cover_art_url || ""
                                return url && url.length > 0 ? url : "qrc:/new/prefix1/icon/Music.png"
                            }
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            cache: true
                            sourceSize.width: 44
                            sourceSize.height: 44

                            onStatusChanged: {
                                if (status === Image.Error) {
                                    source = "qrc:/new/prefix1/icon/Music.png"
                                }
                            }
                        }
                    }

                    Column {
                        width: root.colTitleWidth
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 4

                        Text {
                            text: root.displayTitle(model)
                            font.pixelSize: 14
                            font.bold: itemRoot.isPlaying
                            color: itemRoot.isPlaying ? Theme.accent : Theme.textPrimary
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
                        text: model.added_at || ""
                        width: root.colTimeWidth
                        font.pixelSize: 12
                        color: Theme.textSecondary
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Library.SongActionStrip {
                        id: actionStrip
                        width: root.colActionWidth
                        anchors.verticalCenter: parent.verticalCenter
                        z: 1
                        opacity: itemRoot.rowHovered ? 1.0 : 0.0
                        enabled: itemRoot.rowHovered
                        availablePlaylists: root.availablePlaylists
                        songData: root.buildSongPayload(model)
                        favoriteActive: true
                        showDownloadButton: !model.is_local
                        showRemoveAction: true
                        removeActionText: "移除喜欢"

                        onActionRequested: function(action, payload) {
                            if (action === "play") {
                                root.playMusic(model.path || "")
                                return
                            }
                            if (action === "remove_favorite" || action === "remove_or_delete") {
                                root.removeFavorite([model.path])
                                return
                            }
                            root.songActionRequested(action, payload)
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    propagateComposedEvents: true
                    onPressed: mouse.accepted = false
                    onReleased: mouse.accepted = false
                    onClicked: mouse.accepted = false
                    onDoubleClicked: root.playMusic(model.path || "")
                }
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                width: 8
            }

            Label {
                anchors.centerIn: parent
                text: "暂无喜欢的音乐"
                font.pixelSize: 16
                color: Theme.textSecondary
                visible: listView.count === 0
            }
        }
    }

    function loadFavorites(favoritesData) {
        favoriteModel.clear()
        root.selectedItems = []

        for (var i = 0; i < favoritesData.length; i++) {
            var item = favoritesData[i]
            item.uniqueId = i
            item.title = root.displayTitle(item)
            item.artist = root.displayArtist(item)
            favoriteModel.append(item)
        }
    }

    function clearFavorites() {
        favoriteModel.clear()
        root.selectedItems = []
    }
}
