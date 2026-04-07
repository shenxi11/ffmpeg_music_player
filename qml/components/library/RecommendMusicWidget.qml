import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#f5f5f5"

    property bool isLoggedIn: false
    property string userAccount: ""
    property string currentPlayingPath: ""
    property bool isPlaying: false
    property string requestId: ""
    property string modelVersion: ""
    property string scene: "home"
    property var availablePlaylists: []
    property var favoritePaths: []
    property string playerIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/player/"
    property string listIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/list/"

    signal playMusicWithMetadata(string filePath, string title, string artist, string cover, string duration,
                                 string songId, string requestId, string modelVersion, string scene)
    signal addToFavorite(string filePath, string title, string artist, string duration, bool isLocal)
    signal feedbackEvent(string songId, string eventType, int playMs, int durationMs,
                         string scene, string requestId, string modelVersion)
    signal loginRequested()
    signal refreshRequested()
    signal songActionRequested(string action, var song)

    ListModel {
        id: recommendModel
    }

    function normalizeText(value, fallbackText) {
        if (value === undefined || value === null) return fallbackText
        var text = String(value).trim()
        return text.length > 0 ? text : fallbackText
    }

    function normalizePath(path) {
        if (!path) return ""
        var value = String(path).trim()
        var qPos = value.indexOf("?")
        if (qPos >= 0) value = value.substring(0, qPos)
        try {
            value = decodeURIComponent(value)
        } catch (e) {
        }
        return value
    }

    function isSameTrack(pathA, pathB) {
        return normalizePath(pathA) === normalizePath(pathB)
    }

    function isFavoritePath(path) {
        var target = normalizePath(path)
        for (var i = 0; i < favoritePaths.length; ++i) {
            if (normalizePath(favoritePaths[i]) === target) {
                return true
            }
        }
        return false
    }

    function playPath(item) {
        return normalizeText(item.play_path, normalizeText(item.stream_url, item.path))
    }

    function buildSongPayload(item) {
        return {
            path: normalizeText(item.path, ""),
            playPath: playPath(item),
            title: normalizeText(item.title, "未知歌曲"),
            artist: normalizeText(item.artist, "未知艺术家"),
            duration: normalizeText(item.duration, "0:00"),
            cover: normalizeText(item.cover_art_url, ""),
            isLocal: false,
            isFavorite: isFavoritePath(normalizeText(item.path, playPath(item))),
            sourceType: "recommend",
            songId: normalizeText(item.song_id, item.path)
        }
    }

    function durationMs(item) {
        var sec = Number(item.duration_sec)
        if (isNaN(sec) || sec <= 0) return -1
        return Math.round(sec * 1000)
    }

    function reportEvent(item, eventType, playMs) {
        var sid = normalizeText(item.song_id, item.path)
        if (sid.length === 0) return
        feedbackEvent(
            sid,
            eventType,
            playMs === undefined ? -1 : playMs,
            durationMs(item),
            normalizeText(item.scene, root.scene),
            normalizeText(item.request_id, root.requestId),
            normalizeText(item.model_version, root.modelVersion)
        )
    }

    function playItem(item, reportClick) {
        var path = playPath(item)
        if (path.length === 0) return
        if (reportClick) {
            reportEvent(item, "click", -1)
        }
        reportEvent(item, "play", 0)
        playMusicWithMetadata(
            path,
            normalizeText(item.title, "未知歌曲"),
            normalizeText(item.artist, "未知艺术家"),
            normalizeText(item.cover_art_url, ""),
            normalizeText(item.duration, "0:00"),
            normalizeText(item.song_id, item.path),
            normalizeText(item.request_id, root.requestId),
            normalizeText(item.model_version, root.modelVersion),
            normalizeText(item.scene, root.scene)
        )
    }

    function setPlayingState(filePath, playing) {
        root.currentPlayingPath = filePath || ""
        root.isPlaying = playing
    }

    function clearRecommendations() {
        recommendModel.clear()
        root.requestId = ""
        root.modelVersion = ""
    }

    function loadRecommendations(meta, items) {
        recommendModel.clear()
        root.requestId = normalizeText(meta.request_id, "")
        root.modelVersion = normalizeText(meta.model_version, "")
        root.scene = normalizeText(meta.scene, "home")

        for (var i = 0; i < items.length; i++) {
            var raw = items[i]
            var item = {
                song_id: normalizeText(raw.song_id, raw.path),
                path: normalizeText(raw.path, ""),
                play_path: normalizeText(raw.play_path, normalizeText(raw.stream_url, raw.path)),
                title: normalizeText(raw.title, "未知歌曲"),
                artist: normalizeText(raw.artist, "未知艺术家"),
                album: normalizeText(raw.album, ""),
                duration_sec: Number(raw.duration_sec),
                duration: normalizeText(raw.duration, "0:00"),
                cover_art_url: normalizeText(raw.cover_art_url, ""),
                stream_url: normalizeText(raw.stream_url, ""),
                score: Number(raw.score),
                reason: normalizeText(raw.reason, ""),
                source: normalizeText(raw.source, ""),
                request_id: normalizeText(raw.request_id, root.requestId),
                model_version: normalizeText(raw.model_version, root.modelVersion),
                scene: normalizeText(raw.scene, root.scene)
            }
            recommendModel.append(item)
            reportEvent(item, "impression", -1)
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: !root.isLoggedIn
        color: "#f5f5f5"

        Column {
            anchors.centerIn: parent
            spacing: 14

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "推荐音乐需要登录"
                font.pixelSize: 18
                color: "#333333"
                font.bold: true
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "登录后可根据播放与喜欢记录获取个性化推荐"
                font.pixelSize: 13
                color: "#888888"
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 150
                height: 40
                text: "立即登录"

                background: Rectangle {
                    color: parent.pressed ? "#d93636" : (parent.hovered ? "#ff5757" : "#ec4141")
                    radius: 6
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: root.loginRequested()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        visible: root.isLoggedIn

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: "今日推荐"
                    font.pixelSize: 22
                    font.bold: true
                    color: "#1f1f1f"
                }

                Text {
                    text: "基于你的播放行为与喜好动态生成"
                    font.pixelSize: 12
                    color: "#7d7d7d"
                }
            }

            Button {
                text: "刷新推荐"
                width: 92
                height: 34

                background: Rectangle {
                    color: parent.pressed ? "#d93636" : (parent.hovered ? "#ff5757" : "#ec4141")
                    radius: 17
                }
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: root.refreshRequested()
            }
        }

        GridView {
            id: gridView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.bottomMargin: 14
            clip: true
            model: recommendModel

            property int columnCount: width >= 1120 ? 5 : (width >= 860 ? 4 : (width >= 640 ? 3 : 2))
            cellWidth: Math.floor((width - (columnCount - 1) * 16) / columnCount)
            cellHeight: cellWidth + 86

            delegate: Rectangle {
                id: card
                width: gridView.cellWidth
                height: gridView.cellHeight
                radius: 10
                color: hovered ? "#ffffff" : "#fafafa"
                border.color: hovered ? "#ffd1d1" : "#ececec"
                border.width: 1

                property bool hovered: false
                property string thisPlayPath: root.playPath(model)
                property bool isCurrentTrack: root.isSameTrack(root.currentPlayingPath, thisPlayPath)
                property bool showPauseIcon: isCurrentTrack && root.isPlaying

                Rectangle {
                    id: coverWrapper
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: gridView.cellWidth
                    radius: 10
                    color: "#e8e8e8"
                    clip: true

                    Image {
                        anchors.fill: parent
                        source: model.cover_art_url || "qrc:/qml/assets/ai/icons/default-music-cover.svg"
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: true
                    }

                    Rectangle {
                        anchors.fill: parent
                        visible: card.hovered
                        color: "#66000000"
                    }

                    Rectangle {
                        width: 58
                        height: 58
                        radius: 29
                        anchors.centerIn: parent
                        visible: card.hovered
                        color: "#FFFFFF"
                        border.width: 1
                        border.color: "#F3CACA"

                        Image {
                            anchors.centerIn: parent
                            width: 24
                            height: 24
                            source: card.showPauseIcon
                                    ? root.playerIconPrefix + "player_btn_pause_hover.svg"
                                    : root.playerIconPrefix + "player_btn_play_hover.svg"
                            fillMode: Image.PreserveAspectFit
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.playItem(model, true)
                        }
                    }

                    Rectangle {
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 8
                        height: 22
                        radius: 11
                        color: "#99000000"
                        visible: (model.duration || "").length > 0

                        Text {
                            anchors.centerIn: parent
                            text: model.duration || ""
                            color: "white"
                            font.pixelSize: 11
                        }

                        width: timeText.width + 14

                        Text {
                            id: timeText
                            visible: false
                            text: model.duration || ""
                            font.pixelSize: 11
                        }
                    }
                }

                Column {
                    anchors.top: coverWrapper.bottom
                    anchors.topMargin: 8
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 4

                    Text {
                        text: model.title || "未知歌曲"
                        font.pixelSize: 14
                        font.bold: card.isCurrentTrack
                        color: card.isCurrentTrack ? "#ec4141" : "#2f2f2f"
                        elide: Text.ElideRight
                        width: parent.width
                    }

                    Text {
                        text: model.artist || "未知艺术家"
                        font.pixelSize: 12
                        color: "#7d7d7d"
                        elide: Text.ElideRight
                        width: parent.width
                    }
                }

                SongActionStrip {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    anchors.bottomMargin: 8
                    opacity: card.hovered ? 1.0 : 0.0
                    visible: opacity > 0
                    availablePlaylists: root.availablePlaylists
                    songData: root.buildSongPayload(model)
                    favoriteActive: root.isFavoritePath(root.normalizeText(model.path, root.playPath(model)))
                    showDownloadButton: true
                    showRemoveAction: false

                    Behavior on opacity { NumberAnimation { duration: 120 } }

                    onActionRequested: function(action, payload) {
                        if (action === "play") {
                            root.playItem(model, true)
                            return
                        }
                        if (action === "add_favorite") {
                            root.reportEvent(model, "like", -1)
                            root.addToFavorite(root.normalizeText(model.path, root.playPath(model)),
                                               model.title || "未知歌曲",
                                               model.artist || "未知艺术家",
                                               model.duration || "0:00",
                                               false)
                            return
                        }
                        root.songActionRequested(action, payload)
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    propagateComposedEvents: true
                    onEntered: card.hovered = true
                    onExited: card.hovered = false
                    onPressed: mouse.accepted = false
                    onReleased: mouse.accepted = false
                    onClicked: {
                        root.reportEvent(model, "click", -1)
                        mouse.accepted = false
                    }
                    onDoubleClicked: root.playItem(model, true)
                }
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                width: 8
            }

            Label {
                anchors.centerIn: parent
                visible: recommendModel.count === 0
                text: "暂无推荐结果，点击右上角刷新"
                color: "#9a9a9a"
                font.pixelSize: 15
            }
        }
    }
}
