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
    property string currentSection: isLoggedIn ? "recommend" : "hot"
    property string hotChartWindow: "30d"
    property string lastHotChartWindow: ""
    property string hotChartTitle: "热歌榜"
    property string hotChartGeneratedAt: ""
    property bool recommendationLoading: false
    property bool recommendationLoaded: false
    property bool hotChartLoading: false
    property bool hotChartLoaded: false
    property string hotChartErrorMessage: ""
    property int hotChartStatusCode: 0

    signal playMusicWithMetadata(string filePath, string musicPath, string title, string artist, string cover, string duration,
                                 string songId, string requestId, string modelVersion, string scene)
    signal addToFavorite(string filePath, string title, string artist, string duration, bool isLocal)
    signal feedbackEvent(string songId, string eventType, int playMs, int durationMs,
                         string scene, string requestId, string modelVersion)
    signal requestRecommendations()
    signal requestHotChart(string window)
    signal loginRequested()
    signal songActionRequested(string action, var song)

    ListModel {
        id: recommendModel
    }

    ListModel {
        id: hotChartModel
    }

    function normalizeText(value, fallbackText) {
        if (value === undefined || value === null) return fallbackText
        var text = String(value).trim()
        return text.length > 0 ? text : fallbackText
    }

    function normalizePath(path) {
        if (!path) return ""
        var value = String(path).trim()

        function safeDecode(text) {
            try {
                return decodeURIComponent(text)
            } catch (e) {
                return text
            }
        }

        function extractProxySource(text) {
            var question = text.indexOf("?")
            if (question < 0 || question >= text.length - 1) {
                return ""
            }

            var query = text.substring(question + 1).split("&")
            for (var i = 0; i < query.length; ++i) {
                var entry = query[i]
                if (!entry) continue
                var eq = entry.indexOf("=")
                var key = eq >= 0 ? entry.substring(0, eq) : entry
                var rawValue = eq >= 0 ? entry.substring(eq + 1) : ""
                if (safeDecode(key) === "src") {
                    return safeDecode(rawValue)
                }
            }
            return ""
        }

        value = value.replace(/\\/g, "/")
        var lowerValue = value.toLowerCase()

        if (lowerValue.indexOf("file:///") === 0) {
            value = safeDecode(value.substring(8))
            value = value.replace(/\\/g, "/")
            return value
        }

        if (lowerValue.indexOf("http://") === 0 || lowerValue.indexOf("https://") === 0) {
            if (value.indexOf("/proxy") >= 0) {
                var proxySource = extractProxySource(value)
                if (proxySource.length > 0) {
                    return normalizePath(proxySource)
                }
            }

            var schemePos = value.indexOf("://")
            var pathStart = value.indexOf("/", schemePos >= 0 ? schemePos + 3 : 0)
            value = pathStart >= 0 ? value.substring(pathStart) : ""
            lowerValue = value.toLowerCase()

            var uploadsPos = lowerValue.indexOf("/uploads/")
            if (uploadsPos >= 0) {
                value = value.substring(uploadsPos + 1)
            }
        }

        value = safeDecode(value)
        value = value.replace(/\\/g, "/")

        while (value.indexOf("/") === 0) {
            value = value.substring(1)
        }

        lowerValue = value.toLowerCase()
        if (lowerValue.indexOf("uploads/uploads/") === 0) {
            value = "uploads/" + value.substring("uploads/uploads/".length)
            lowerValue = value.toLowerCase()
        }
        if (lowerValue.indexOf("uploads/") === 0) {
            value = value.substring("uploads/".length)
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

    function durationMs(item) {
        var sec = Number(item.duration_sec)
        if (isNaN(sec) || sec <= 0) return -1
        return Math.round(sec * 1000)
    }

    function durationText(item) {
        if (item.duration !== undefined && String(item.duration).trim().length > 0) {
            return String(item.duration).trim()
        }
        var ms = durationMs(item)
        if (ms <= 0) return "0:00"
        var totalSec = Math.floor(ms / 1000)
        var min = Math.floor(totalSec / 60)
        var sec = totalSec % 60
        return min + ":" + (sec < 10 ? "0" + sec : sec)
    }

    function formatPlayCount(value) {
        var count = Math.max(0, Math.round(Number(value)))
        if (isNaN(count)) count = 0
        return String(count).replace(/\B(?=(\d{3})+(?!\d))/g, ",")
    }

    function hotChartSubtitle(item) {
        var pieces = ["热度 " + formatPlayCount(item.play_count)]
        var sourceName = normalizeText(item.source, "")
        if (sourceName.length > 0) {
            pieces.push(sourceName)
        }
        return pieces.join(" · ")
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

    function buildRecommendationPayload(item) {
        var pathValue = normalizeText(item.path, "")
        return {
            path: pathValue,
            musicPath: pathValue,
            playPath: playPath(item),
            title: normalizeText(item.title, "未知歌曲"),
            artist: normalizeText(item.artist, "未知艺术家"),
            album: normalizeText(item.album, ""),
            duration: normalizeText(item.duration, "0:00"),
            duration_sec: Number(item.duration_sec),
            cover: normalizeText(item.cover_art_url, ""),
            isLocal: false,
            isFavorite: isFavoritePath(pathValue),
            sourceType: "recommend",
            source: normalizeText(item.source, ""),
            sourceId: normalizeText(item.song_id, pathValue),
            songId: normalizeText(item.song_id, pathValue)
        }
    }

    function buildHotChartPayload(item) {
        var musicPath = normalizeText(item.music_path, normalizeText(item.path, ""))
        var relativePath = normalizeText(item.path, musicPath)
        return {
            path: relativePath,
            musicPath: musicPath,
            playPath: normalizeText(item.play_path, relativePath),
            title: normalizeText(item.title, "未知歌曲"),
            artist: normalizeText(item.artist, "未知艺术家"),
            album: normalizeText(item.album, ""),
            duration: durationText(item),
            duration_sec: Number(item.duration_sec),
            cover: normalizeText(item.cover_art_url, ""),
            isLocal: false,
            isFavorite: isFavoritePath(relativePath),
            sourceType: "online",
            source: normalizeText(item.source, ""),
            sourceId: normalizeText(item.source_id, musicPath),
            playCount: Number(item.play_count),
            rank: Number(item.rank)
        }
    }

    function playRecommendationItem(item, reportClick) {
        var path = playPath(item)
        if (path.length === 0) return
        if (reportClick) {
            reportEvent(item, "click", -1)
        }
        reportEvent(item, "play", 0)
        playMusicWithMetadata(
            path,
            normalizeText(item.path, path),
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

    function playHotChartItem(item) {
        root.songActionRequested("play", buildHotChartPayload(item))
    }

    function currentHotChartCover() {
        if (hotChartModel.count <= 0) return ""
        return normalizeText(hotChartModel.get(0).cover_art_url, "")
    }

    function formatGeneratedAt(raw) {
        var value = normalizeText(raw, "")
        if (value.length === 0) return "暂无更新时间"
        if (value.indexOf("T") >= 0) {
            value = value.replace("T", " ")
        }
        var plusPos = value.indexOf("+")
        if (plusPos > 0) value = value.substring(0, plusPos)
        if (value.length >= 19) return value.substring(0, 19)
        return value
    }

    function hotChartWindowLabel(windowValue) {
        if (windowValue === "7d") return "近7天"
        if (windowValue === "all") return "总榜"
        return "近30天"
    }

    function defaultSection() {
        return isLoggedIn ? "recommend" : "hot"
    }

    function selectSection(section) {
        currentSection = section
        ensureSectionLoaded(false)
    }

    function requestRecommendationReload() {
        if (!isLoggedIn) return
        recommendationLoading = true
        requestRecommendations()
    }

    function requestHotChartReload(windowValue) {
        hotChartLoading = true
        hotChartErrorMessage = ""
        hotChartStatusCode = 0
        requestHotChart(windowValue)
    }

    function ensureSectionLoaded(forceReload) {
        if (currentSection === "recommend") {
            if (!isLoggedIn) return
            if (forceReload || !recommendationLoaded || recommendModel.count === 0) {
                requestRecommendationReload()
            }
            return
        }

        if (forceReload || !hotChartLoaded || hotChartWindow !== lastHotChartWindow) {
            requestHotChartReload(hotChartWindow)
        }
    }

    function activateForEntry() {
        if (!isLoggedIn && currentSection === "recommend") {
            currentSection = "hot"
        }
        if (!currentSection || currentSection.length === 0) {
            currentSection = defaultSection()
        }
        ensureSectionLoaded(false)
    }

    function setPlayingState(filePath, playing) {
        root.currentPlayingPath = filePath || ""
        root.isPlaying = playing
    }

    function clearRecommendations() {
        recommendModel.clear()
        recommendationLoading = false
        recommendationLoaded = false
        root.requestId = ""
        root.modelVersion = ""
    }

    function loadRecommendations(meta, items) {
        recommendModel.clear()
        recommendationLoading = false
        recommendationLoaded = true
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

    function loadHotChart(meta, items) {
        hotChartModel.clear()
        hotChartLoading = false
        hotChartLoaded = true
        hotChartErrorMessage = ""
        hotChartStatusCode = 0
        hotChartTitle = normalizeText(meta.title, "热歌榜")
        hotChartGeneratedAt = normalizeText(meta.generated_at, "")
        lastHotChartWindow = normalizeText(meta.window, hotChartWindow)
        if (lastHotChartWindow.length > 0) {
            hotChartWindow = lastHotChartWindow
        }

        for (var i = 0; i < items.length; ++i) {
            var raw = items[i]
            hotChartModel.append({
                rank: Number(raw.rank),
                music_path: normalizeText(raw.music_path, raw.path),
                path: normalizeText(raw.path, raw.music_path),
                play_path: normalizeText(raw.play_path, normalizeText(raw.path, raw.music_path)),
                title: normalizeText(raw.title, "未知歌曲"),
                artist: normalizeText(raw.artist, "未知艺术家"),
                album: normalizeText(raw.album, ""),
                duration_sec: Number(raw.duration_sec),
                duration: normalizeText(raw.duration, ""),
                cover_art_url: normalizeText(raw.cover_art_url, ""),
                source: normalizeText(raw.source, ""),
                source_id: normalizeText(raw.source_id, ""),
                play_count: Number(raw.play_count)
            })
        }
    }

    function showHotChartError(message, statusCode, windowValue) {
        if (windowValue && String(windowValue) !== hotChartWindow) {
            return
        }
        hotChartLoading = false
        hotChartLoaded = false
        hotChartErrorMessage = normalizeText(message, "热歌榜加载失败")
        hotChartStatusCode = Number(statusCode)
        hotChartModel.clear()
    }

    onIsLoggedInChanged: {
        if (!isLoggedIn && currentSection === "recommend") {
            currentSection = "hot"
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        anchors.topMargin: 18
        anchors.bottomMargin: 14
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 58

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: "推荐"
                    font.pixelSize: 24
                    font.bold: true
                    color: "#202020"
                }

                Text {
                    text: currentSection === "recommend"
                          ? "个性推荐与你的播放喜好联动，热歌榜则展示公开的在线热门歌曲。"
                          : "热歌榜来自服务端在线播放历史热度统计，支持公开浏览。"
                    font.pixelSize: 12
                    color: "#7d7d7d"
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                implicitWidth: tabRow.implicitWidth + 8
                implicitHeight: 40
                radius: 20
                color: "#FFFFFF"
                border.width: 1
                border.color: "#E7E7E7"

                Row {
                    id: tabRow
                    anchors.centerIn: parent
                    spacing: 6

                    Repeater {
                        model: [
                            { "key": "recommend", "label": "个性推荐" },
                            { "key": "hot", "label": "热歌榜" }
                        ]

                        delegate: Rectangle {
                            width: 92
                            height: 30
                            radius: 15
                            color: root.currentSection === modelData.key ? "#EC4141" : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: modelData.label
                                color: root.currentSection === modelData.key ? "#FFFFFF" : "#555555"
                                font.pixelSize: 13
                                font.bold: root.currentSection === modelData.key
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.selectSection(modelData.key)
                            }
                        }
                    }
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.currentSection === "recommend" ? 0 : 1

            Item {
                id: recommendPage
                Layout.fillWidth: true
                Layout.fillHeight: true

                Rectangle {
                    anchors.fill: parent
                    radius: 18
                    color: "#FFFFFF"
                    border.width: 1
                    border.color: "#ECECEC"
                }

                Item {
                    anchors.fill: parent
                    anchors.margins: 18

                    Item {
                        anchors.fill: parent
                        visible: !root.isLoggedIn

                        Column {
                            anchors.centerIn: parent
                            spacing: 14

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "登录后可查看个性推荐"
                                font.pixelSize: 20
                                font.bold: true
                                color: "#2A2A2A"
                            }

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "热歌榜可直接浏览，个性推荐会根据你的播放和喜欢记录动态生成。"
                                font.pixelSize: 13
                                color: "#7F7F7F"
                            }

                            Rectangle {
                                width: 148
                                height: 42
                                radius: 21
                                anchors.horizontalCenter: parent.horizontalCenter
                                color: loginArea.pressed ? "#D63737" : (loginArea.containsMouse ? "#FF5757" : "#EC4141")

                                Text {
                                    anchors.centerIn: parent
                                    text: "立即登录"
                                    color: "#FFFFFF"
                                    font.pixelSize: 14
                                    font.bold: true
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
                        visible: root.isLoggedIn
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 52

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: "个性推荐"
                                    font.pixelSize: 22
                                    font.bold: true
                                    color: "#1F1F1F"
                                }

                                Text {
                                    text: "基于你的播放行为与喜好动态生成"
                                    font.pixelSize: 12
                                    color: "#7D7D7D"
                                }
                            }

                            Rectangle {
                                width: 96
                                height: 34
                                radius: 17
                                color: recommendRefreshArea.pressed ? "#D63737"
                                       : (recommendRefreshArea.containsMouse ? "#FF5757" : "#EC4141")

                                Text {
                                    anchors.centerIn: parent
                                    text: "刷新推荐"
                                    color: "#FFFFFF"
                                    font.pixelSize: 12
                                }

                                MouseArea {
                                    id: recommendRefreshArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.ensureSectionLoaded(true)
                                }
                            }
                        }

                        GridView {
                            id: gridView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
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
                                color: hovered ? "#FFFFFF" : "#FAFAFA"
                                border.color: hovered ? "#FFD1D1" : "#ECECEC"
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
                                    color: "#E8E8E8"
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
                                            onClicked: root.playRecommendationItem(model, true)
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
                                        width: timeText.width + 14

                                        Text {
                                            anchors.centerIn: parent
                                            text: model.duration || ""
                                            color: "white"
                                            font.pixelSize: 11
                                        }

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
                                        color: card.isCurrentTrack ? "#EC4141" : "#2F2F2F"
                                        elide: Text.ElideRight
                                        width: parent.width
                                    }

                                    Text {
                                        text: model.artist || "未知艺术家"
                                        font.pixelSize: 12
                                        color: "#7D7D7D"
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
                                    songData: root.buildRecommendationPayload(model)
                                    favoriteActive: root.isFavoritePath(root.normalizeText(model.path, root.playPath(model)))
                                    showDownloadButton: true
                                    showRemoveAction: false

                                    Behavior on opacity { NumberAnimation { duration: 120 } }

                                    onActionRequested: function(action, payload) {
                                        if (action === "play") {
                                            root.playRecommendationItem(model, true)
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
                                    onDoubleClicked: root.playRecommendationItem(model, true)
                                }
                            }

                            ScrollBar.vertical: ScrollBar {
                                policy: ScrollBar.AsNeeded
                                width: 8
                            }

                            Rectangle {
                                anchors.fill: parent
                                visible: root.recommendationLoading
                                color: "#40FFFFFF"

                                BusyIndicator {
                                    anchors.centerIn: parent
                                    running: parent.visible
                                }
                            }

                            Label {
                                anchors.centerIn: parent
                                visible: !root.recommendationLoading && root.recommendationLoaded
                                         && recommendModel.count === 0
                                text: "暂无推荐结果，点击右上角刷新"
                                color: "#9A9A9A"
                                font.pixelSize: 15
                            }
                        }
                    }
                }
            }

            Item {
                id: hotChartPage
                Layout.fillWidth: true
                Layout.fillHeight: true

                Rectangle {
                    anchors.fill: parent
                    radius: 18
                    color: "#FFFFFF"
                    border.width: 1
                    border.color: "#ECECEC"
                }

                ListView {
                    id: hotChartListView
                    anchors.fill: parent
                    anchors.margins: 18
                    clip: true
                    spacing: 0
                    model: hotChartModel

                    property int sideMargin: 10
                    property int rowGap: 12
                    property int rankWidth: 58
                    property int coverWidth: 44
                    property int durationWidth: width >= 1080 ? 86 : 76
                    property int artistWidth: width >= 1180 ? 220 : 180
                    property int albumWidth: width >= 1180 ? 240 : 180
                    property int songWidth: Math.max(260, width - sideMargin * 2 - rankWidth
                                                     - artistWidth - albumWidth - durationWidth
                                                     - rowGap * 3)

                    header: Column {
                        width: hotChartListView.width
                        spacing: 18

                        Rectangle {
                            width: parent.width
                            height: 228
                            radius: 18
                            color: "#F6F6F6"

                            Row {
                                anchors.fill: parent
                                anchors.margins: 22
                                spacing: 22

                                Rectangle {
                                    width: 184
                                    height: 184
                                    radius: 18
                                    color: "#E1E1E1"
                                    clip: true

                                    Image {
                                        anchors.fill: parent
                                        source: root.currentHotChartCover() || "qrc:/qml/assets/ai/icons/default-music-cover.svg"
                                        fillMode: Image.PreserveAspectCrop
                                        asynchronous: true
                                        cache: true
                                    }

                                    Rectangle {
                                        anchors.fill: parent
                                        color: "#60000000"
                                    }

                                    Column {
                                        anchors.centerIn: parent
                                        spacing: 6

                                        Text {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            text: "热歌"
                                            color: "#FFFFFF"
                                            font.pixelSize: 18
                                            font.bold: true
                                            opacity: 0.92
                                        }

                                        Text {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            text: "TOP"
                                            color: "#FFFFFF"
                                            font.pixelSize: 34
                                            font.bold: true
                                        }
                                    }
                                }

                                Column {
                                    width: parent.width - 184 - 22
                                    height: 184
                                    spacing: 12

                                    Text {
                                        text: root.hotChartTitle
                                        color: "#111111"
                                        font.pixelSize: 32
                                        font.bold: true
                                    }

                                    Text {
                                        text: "按在线歌曲播放历史热度排序，" + root.hotChartWindowLabel(root.hotChartWindow) + "榜单。"
                                        color: "#707070"
                                        font.pixelSize: 13
                                    }

                                    Text {
                                        text: "更新时间：" + root.formatGeneratedAt(root.hotChartGeneratedAt)
                                        color: "#8B8B8B"
                                        font.pixelSize: 12
                                    }

                                    Row {
                                        spacing: 8

                                        Repeater {
                                            model: [
                                                { "key": "7d", "label": "近7天" },
                                                { "key": "30d", "label": "近30天" },
                                                { "key": "all", "label": "总榜" }
                                            ]

                                            delegate: Rectangle {
                                                width: 72
                                                height: 30
                                                radius: 15
                                                color: root.hotChartWindow === modelData.key ? "#111111" : "#F0F0F0"

                                                Text {
                                                    anchors.centerIn: parent
                                                    text: modelData.label
                                                    color: root.hotChartWindow === modelData.key ? "#FFFFFF" : "#555555"
                                                    font.pixelSize: 12
                                                    font.bold: root.hotChartWindow === modelData.key
                                                }

                                                MouseArea {
                                                    anchors.fill: parent
                                                    cursorShape: Qt.PointingHandCursor
                                                    enabled: !root.hotChartLoading
                                                    onClicked: {
                                                        if (root.hotChartWindow === modelData.key) {
                                                            return
                                                        }
                                                        root.hotChartWindow = modelData.key
                                                        root.ensureSectionLoaded(true)
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    Row {
                                        anchors.bottom: parent.bottom
                                        spacing: 10

                                        Rectangle {
                                            width: 118
                                            height: 38
                                            radius: 19
                                            color: chartPlayArea.pressed ? "#18B988"
                                                   : (chartPlayArea.containsMouse ? "#27D9A5" : "#1ECE9B")

                                            Text {
                                                anchors.centerIn: parent
                                                text: "播放榜首"
                                                color: "#FFFFFF"
                                                font.pixelSize: 14
                                                font.bold: true
                                            }

                                            MouseArea {
                                                id: chartPlayArea
                                                anchors.fill: parent
                                                enabled: hotChartModel.count > 0 && !root.hotChartLoading
                                                hoverEnabled: true
                                                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                                                onClicked: {
                                                    if (hotChartModel.count > 0) {
                                                        root.playHotChartItem(hotChartModel.get(0))
                                                    }
                                                }
                                            }
                                        }

                                        Rectangle {
                                            width: 98
                                            height: 38
                                            radius: 19
                                            color: refreshChartArea.pressed ? "#E5E5E5"
                                                   : (refreshChartArea.containsMouse ? "#F2F2F2" : "#F7F7F7")
                                            border.width: 1
                                            border.color: "#E3E3E3"

                                            Text {
                                                anchors.centerIn: parent
                                                text: "刷新榜单"
                                                color: "#4E4E4E"
                                                font.pixelSize: 14
                                            }

                                            MouseArea {
                                                id: refreshChartArea
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: root.ensureSectionLoaded(true)
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: 42
                            radius: 10
                            color: "#FAFAFA"

                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: hotChartListView.sideMargin
                                anchors.rightMargin: hotChartListView.sideMargin
                                spacing: hotChartListView.rowGap

                                Item {
                                    width: hotChartListView.rankWidth
                                    height: parent.height
                                }

                                Item {
                                    width: hotChartListView.songWidth
                                    height: parent.height

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.leftMargin: hotChartListView.coverWidth + 12
                                        text: "歌曲"
                                        color: "#7A7A7A"
                                        font.pixelSize: 13
                                    }
                                }

                                Text {
                                    width: hotChartListView.artistWidth
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "歌手"
                                    color: "#7A7A7A"
                                    font.pixelSize: 13
                                }

                                Text {
                                    width: hotChartListView.albumWidth
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "专辑"
                                    color: "#7A7A7A"
                                    font.pixelSize: 13
                                }

                                Text {
                                    width: hotChartListView.durationWidth
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "时长"
                                    color: "#7A7A7A"
                                    font.pixelSize: 13
                                }
                            }
                        }
                    }

                    delegate: Rectangle {
                        id: hotRow
                        width: hotChartListView.width
                        height: 68
                        radius: 6
                        color: hotRow.rowHovered ? "#F0F0F0" : "#FFFFFF"

                        property bool currentTrack: root.isSameTrack(root.currentPlayingPath,
                                                                     model.play_path || model.path || model.music_path)
                        property bool playbackActive: currentTrack && root.isPlaying
                        property bool rowHovered: rowHoverHandler.hovered
                                                  || coverAction.interactionActive
                                                  || actionStrip.interactionActive

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: hotChartListView.sideMargin
                            anchors.rightMargin: hotChartListView.sideMargin
                            spacing: hotChartListView.rowGap

                            Item {
                                width: hotChartListView.rankWidth
                                height: parent.height

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: model.rank > 0 ? model.rank : (index + 1)
                                    color: model.rank <= 3 ? "#EC4141" : "#666666"
                                    font.pixelSize: model.rank <= 3 ? 22 : 18
                                    font.bold: model.rank <= 3
                                }
                            }

                            Item {
                                width: hotChartListView.songWidth
                                height: parent.height

                                Row {
                                    anchors.fill: parent
                                    spacing: 12

                                    SongCoverAction {
                                        id: coverAction
                                        width: hotChartListView.coverWidth
                                        height: hotChartListView.coverWidth
                                        anchors.verticalCenter: parent.verticalCenter
                                        rowHovered: hotRow.rowHovered
                                        isCurrentTrack: hotRow.currentTrack
                                        isPlaying: hotRow.playbackActive
                                        coverSource: model.cover_art_url || ""
                                        fallbackSource: "qrc:/qml/assets/ai/icons/default-music-cover.svg"

                                        onPlayRequested: {
                                            if (hotRow.currentTrack) {
                                                root.songActionRequested("toggle_current_playback",
                                                                         root.buildHotChartPayload(model))
                                                return
                                            }
                                            root.playHotChartItem(model)
                                        }

                                        onPauseRequested: {
                                            root.songActionRequested("toggle_current_playback",
                                                                     root.buildHotChartPayload(model))
                                        }
                                    }

                                    Item {
                                        width: parent.width - hotChartListView.coverWidth - 12
                                        height: parent.height

                                        Row {
                                            anchors.fill: parent
                                            spacing: 12

                                            Item {
                                                width: hotRow.rowHovered
                                                       ? Math.max(140, parent.width - actionStrip.width - 12)
                                                       : parent.width
                                                height: parent.height

                                                Column {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    width: parent.width
                                                    spacing: 4

                                                    Text {
                                                        width: parent.width
                                                        text: model.title || "未知歌曲"
                                                        color: hotRow.currentTrack ? "#409EFF" : "#333333"
                                                        font.pixelSize: 14
                                                        font.bold: hotRow.currentTrack
                                                        elide: Text.ElideRight
                                                    }

                                                    Text {
                                                        width: parent.width
                                                        text: root.hotChartSubtitle(model)
                                                        color: "#8A8A8A"
                                                        font.pixelSize: 12
                                                        elide: Text.ElideRight
                                                    }
                                                }
                                            }

                                            SongActionStrip {
                                                id: actionStrip
                                                anchors.verticalCenter: parent.verticalCenter
                                                visible: hotRow.rowHovered
                                                opacity: hotRow.rowHovered ? 1.0 : 0.0
                                                enabled: hotRow.rowHovered
                                                availablePlaylists: root.availablePlaylists
                                                songData: root.buildHotChartPayload(model)
                                                favoriteActive: root.isFavoritePath(model.path || model.music_path || "")
                                                showDownloadButton: true
                                                showRemoveAction: false

                                                Behavior on opacity { NumberAnimation { duration: 120 } }

                                                onActionRequested: function(action, payload) {
                                                    root.songActionRequested(action, payload)
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            Text {
                                width: hotChartListView.artistWidth
                                anchors.verticalCenter: parent.verticalCenter
                                text: model.artist || "未知艺术家"
                                color: "#888888"
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }

                            Text {
                                width: hotChartListView.albumWidth
                                anchors.verticalCenter: parent.verticalCenter
                                text: model.album || "-"
                                color: "#888888"
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }

                            Text {
                                width: hotChartListView.durationWidth
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.durationText(model)
                                color: "#666666"
                                font.pixelSize: 13
                            }
                        }

                        HoverHandler {
                            id: rowHoverHandler
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            propagateComposedEvents: true
                            onPressed: mouse.accepted = false
                            onReleased: mouse.accepted = false
                            onClicked: mouse.accepted = false
                            onDoubleClicked: root.playHotChartItem(model)
                        }
                    }

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                        width: 8
                    }

                    footer: Item {
                        width: hotChartListView.width
                        height: 24
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    radius: 18
                    visible: root.hotChartLoading
                    color: "#65FFFFFF"

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: parent.visible
                    }
                }

                Item {
                    anchors.fill: parent
                    visible: !root.hotChartLoading && root.hotChartErrorMessage.length > 0

                    Column {
                        anchors.centerIn: parent
                        spacing: 12

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: root.hotChartStatusCode === 503
                                  ? "榜单服务暂不可用，请稍后再试"
                                  : root.hotChartErrorMessage
                            color: "#5D5D5D"
                            font.pixelSize: 16
                        }

                        Rectangle {
                            width: 94
                            height: 36
                            radius: 18
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: retryArea.pressed ? "#E0E0E0" : (retryArea.containsMouse ? "#F2F2F2" : "#F7F7F7")
                            border.width: 1
                            border.color: "#E1E1E1"

                            Text {
                                anchors.centerIn: parent
                                text: "重试"
                                color: "#4A4A4A"
                                font.pixelSize: 13
                            }

                            MouseArea {
                                id: retryArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.ensureSectionLoaded(true)
                            }
                        }
                    }
                }

                Item {
                    anchors.fill: parent
                    visible: !root.hotChartLoading && root.hotChartErrorMessage.length === 0
                             && root.hotChartLoaded && hotChartModel.count === 0

                    Text {
                        anchors.centerIn: parent
                        text: "暂无热歌数据"
                        color: "#8F8F8F"
                        font.pixelSize: 16
                    }
                }
            }
        }
    }
}
