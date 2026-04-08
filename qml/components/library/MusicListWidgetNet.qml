import QtQuick 2.14
import QtQuick.Controls 2.14

Rectangle {
    id: root
    color: "#F7F9FC"
    property string listIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/list/"
    property string playerIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/player/"
    property string currentPlayingPath: ""
    property bool isPlaying: false
    
    // 对外暴露的属性
    property int currentPlayingIndex: -1  // 当前播放的歌曲索引，-1表示没有播放
    property int sideMargin: 20
    property int innerMargin: 15
    property int colGap: 10
    property int colCoverWidth: 44
    property int colDurationWidth: width >= 1280 ? 96 : (width >= 960 ? 84 : 72)
    property int colArtistWidth: width >= 1280 ? 160 : (width >= 960 ? 130 : 110)
    property int colActionWidth: 160
    property int contentWidth: Math.max(320, width - sideMargin * 2 - innerMargin * 2)
    property var availablePlaylists: []
    property var favoritePaths: []
    property int colTitleWidth: Math.max(140,
                                         contentWidth - colCoverWidth - colDurationWidth
                                         - colArtistWidth - colActionWidth - colGap * 4)
    
    // 信号
    signal playRequested(string filePath, string artist, string cover)
    signal removeRequested(string filePath)
    signal downloadRequested(string filePath)
    signal addToFavorite(string path, string title, string artist, string duration)
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
        var title = normalizeText(item.songName, "")
        if (title.length > 0) return title
        var fromPath = normalizeText(baseNameFromPath(item.filePath), "")
        return fromPath.length > 0 ? fromPath : "未知歌曲"
    }

    function displayArtist(item) {
        return normalizeText(item.artist, "未知艺术家")
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
                if (!entry)
                    continue
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

        if (/^[a-zA-Z]:\//.test(value)) {
            return safeDecode(value)
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

    function buildSongPayload(item) {
        return {
            path: item.filePath || "",
            playPath: item.filePath || "",
            title: displayTitle(item),
            artist: displayArtist(item),
            duration: item.duration || "0:00",
            cover: item.cover || "",
            isLocal: false,
            isFavorite: isFavoritePath(item.filePath || ""),
            sourceType: "online"
        }
    }

    // 顶部标题
    Text {
        id: topTitle
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 20
        anchors.leftMargin: root.sideMargin
        text: "在线音乐"
        font.pixelSize: 20
        font.bold: true
        color: "#333333"
    }
    
    // 列表头部
    Rectangle {
        id: headerBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.sideMargin
        anchors.rightMargin: root.sideMargin
        height: 40
        color: "#ffffff"
        anchors.top: topTitle.bottom
        anchors.topMargin: 10
        radius: 4
        
        Row {
            anchors.fill: parent
            anchors.leftMargin: root.innerMargin
            anchors.rightMargin: root.innerMargin
            spacing: root.colGap
            
            Text {
                text: "封面"
                width: root.colCoverWidth
                font.pixelSize: 14
                font.bold: true
                color: "#333333"
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: "歌曲名"
                width: root.colTitleWidth
                font.pixelSize: 14
                font.bold: true
                color: "#333333"
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: "时长"
                width: root.colDurationWidth
                font.pixelSize: 14
                font.bold: true
                color: "#333333"
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: "艺术家"
                width: root.colArtistWidth
                font.pixelSize: 14
                font.bold: true
                color: "#333333"
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: "操作"
                width: root.colActionWidth
                font.pixelSize: 14
                font.bold: true
                color: "#333333"
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
    
    // 音乐列表
    ListView {
        id: listView
        anchors.top: headerBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.sideMargin
        anchors.rightMargin: root.sideMargin
        anchors.bottom: parent.bottom
        anchors.topMargin: 8
        clip: true
        
        model: musicListModel
        delegate: musicItemDelegate
        spacing: 8
        
        // 滚动条
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            width: 8
        }
        
        // 缓存优化：只缓存可见区域外的几个项
        cacheBuffer: 500
        
        // 空列表提示
        Label {
            anchors.centerIn: parent
            text: "暂无音乐"
            font.pixelSize: 16
            color: "#AAAAAA"
            visible: listView.count === 0
        }
    }
    
    // 音乐项数据模型
    ListModel {
        id: musicListModel
    }
    // 音乐项委托（每一行）
    Component {
        id: musicItemDelegate
        
        Rectangle {
            id: itemRoot
            property bool rowHovered: rowHoverHandler.hovered
                                       || coverAction.interactionActive
                                       || actionStrip.interactionActive
            property bool currentTrack: root.isSameTrack(root.currentPlayingPath, model.filePath || "")
            property bool playbackActive: currentTrack && root.isPlaying
            width: listView.width
            height: 60
            color: rowHovered ? "#f0f0f0" : "#ffffff"
            radius: 4

            HoverHandler {
                id: rowHoverHandler
            }
            
            Row {
                anchors.fill: parent
                anchors.leftMargin: root.innerMargin
                anchors.rightMargin: root.innerMargin
                spacing: root.colGap
                
                // 专辑封面
                SongCoverAction {
                    id: coverAction
                    width: root.colCoverWidth
                    height: 44
                    anchors.verticalCenter: parent.verticalCenter
                    rowHovered: itemRoot.rowHovered
                    isCurrentTrack: itemRoot.currentTrack
                    isPlaying: itemRoot.playbackActive
                    coverSource: model.cover || ""
                    fallbackSource: "qrc:/qml/assets/ai/icons/default-music-cover.svg"

                    onPlayRequested: {
                        if (itemRoot.currentTrack) {
                            root.songActionRequested("toggle_current_playback", root.buildSongPayload(model))
                            return
                        }
                        root.playRequested(model.filePath, root.displayArtist(model), model.cover || "")
                    }

                    onPauseRequested: {
                        root.songActionRequested("toggle_current_playback", root.buildSongPayload(model))
                    }
                }
                
                // 歌曲名
                Text {
                    width: root.colTitleWidth
                    text: root.displayTitle(model)
                    font.pixelSize: 14
                    font.bold: itemRoot.currentTrack
                    color: itemRoot.currentTrack ? "#409EFF" : "#333333"
                    elide: Text.ElideRight
                    anchors.verticalCenter: parent.verticalCenter
                }
                
                // 时长
                Text {
                    width: root.colDurationWidth
                    text: model.duration || "0:00"
                    font.pixelSize: 13
                    color: "#666666"
                    anchors.verticalCenter: parent.verticalCenter
                }
                
                // 艺术家
                Text {
                    width: root.colArtistWidth
                    text: root.displayArtist(model)
                    font.pixelSize: 13
                    color: "#888888"
                    elide: Text.ElideRight
                    anchors.verticalCenter: parent.verticalCenter
                }
                
                // 操作按钮（hover 时显示）
                SongActionStrip {
                    id: actionStrip
                    width: root.colActionWidth
                    anchors.verticalCenter: parent.verticalCenter
                    z: 1
                    opacity: rowHovered ? 1.0 : 0.0
                    enabled: rowHovered
                    availablePlaylists: root.availablePlaylists
                    songData: root.buildSongPayload(model)
                    favoriteActive: root.isFavoritePath(model.filePath || "")
                    showDownloadButton: true
                    showRemoveAction: false

                    onActionRequested: function(action, payload) {
                        if (action === "play") {
                            root.playRequested(model.filePath, root.displayArtist(model), model.cover || "")
                            return
                        }
                        if (action === "add_favorite") {
                            root.addToFavorite(
                                model.filePath || "",
                                root.displayTitle(model),
                                root.displayArtist(model),
                                model.duration || ""
                            )
                            return
                        }
                        if (action === "download") {
                            root.downloadRequested(model.filePath || "")
                            return
                        }
                        root.songActionRequested(action, payload)
                    }
                }
            }
            
            MouseArea {
                id: itemArea
                anchors.fill: parent
                propagateComposedEvents: true
                onDoubleClicked: {
                    root.playRequested(model.filePath, root.displayArtist(model), model.cover || "")
                }
            }
        }
    }
    
    // 提供给 C++ 调用的方法
    function addSong(songName, filePath, artist, duration, cover) {
        var normalizedName = normalizeText(songName, "")
        if (normalizedName.length === 0) {
            normalizedName = normalizeText(baseNameFromPath(filePath), "未知歌曲")
        }
        musicListModel.append({
                                  "songName": normalizedName,
                                  "filePath": filePath,
                                  "artist": normalizeText(artist, "未知艺术家"),
                                  "duration": duration || "0:00",
                                  "cover": cover || "",
                                  "isPlaying": false
                              })
    }
    
    function addSongList(songNames, relativePaths, durations, coverUrls, artists) {
        coverUrls = coverUrls || []
        artists = artists || []
        for (var i = 0; i < songNames.length; i++) {
            var normalizedName = normalizeText(songNames[i], "")
            if (normalizedName.length === 0) {
                normalizedName = normalizeText(baseNameFromPath(relativePaths[i]), "未知歌曲")
            }
            musicListModel.append({
                                      "songName": normalizedName,
                                      "filePath": relativePaths[i],
                                      "artist": normalizeText(artists[i], "未知艺术家"),
                                      "duration": durations[i] || "0:00",
                                      "cover": coverUrls[i] || "",
                                      "isPlaying": false
                                  })
        }
    }
    
    function removeSong(filePath) {
        for (var i = 0; i < musicListModel.count; i++) {
            if (isSameTrack(musicListModel.get(i).filePath, filePath)) {
                musicListModel.remove(i)
                break
            }
        }
    }
    
    function clearAll() {
        musicListModel.clear()
    }
    
    function getCount() {
        return musicListModel.count
    }
    
    function setPlayingState(filePath, playing) {
        root.currentPlayingPath = filePath || ""
        root.isPlaying = playing

        // 如果 filePath 为空，则清除当前播放状态
        if (filePath === "") {
            if (root.currentPlayingIndex >= 0 && root.currentPlayingIndex < musicListModel.count) {
                musicListModel.get(root.currentPlayingIndex).isPlaying = false
            }
            root.currentPlayingIndex = -1
            return
        }

        // 查找匹配的歌曲，统一走当前列表的路径归一化规则。
        for (var i = 0; i < musicListModel.count; i++) {
            if (isSameTrack(filePath, musicListModel.get(i).filePath)) {
                // 找到了目标歌曲
                if (playing) {
                    // 关闭上一首
                    if (root.currentPlayingIndex >= 0 && root.currentPlayingIndex !== i) {
                        musicListModel.get(root.currentPlayingIndex).isPlaying = false
                    }
                    // 开启当前
                    musicListModel.get(i).isPlaying = true
                    root.currentPlayingIndex = i
                } else {
                    // 保留当前索引，只关闭“正在播放”状态，便于 UI 呈现暂停态
                    musicListModel.get(i).isPlaying = false
                    root.currentPlayingIndex = i
                }
                return
            }
        }

        // 没有找到，可能是其他列表的歌曲
        if (root.currentPlayingIndex >= 0) {
            musicListModel.get(root.currentPlayingIndex).isPlaying = false
            if (playing) {
                root.currentPlayingIndex = -1
            }
        }
    }

    function playNext(songName){
        for(var i = 0; i < musicListModel.count; i++){
            var item = musicListModel.get(i)
            if(item.songName === songName){
                var item_next = musicListModel.get((i + 1) % musicListModel.count);
                item.isPlaying = false
                item_next.isPlaying = true
                root.playRequested(item_next.filePath, root.displayArtist(item_next), item_next.cover || "")
                break
            }
        }
    }
    function playLast(songName){
        for(var i = 0; i < musicListModel.count; i++){
            var item = musicListModel.get(i);
            if(item.songName === songName){
                var item_last = musicListModel.get(i - 1 < 0 ? musicListModel.count - 1 : i  - 1);
                item.isPlaying = false;
                item_last.isPlaying = true;
                root.playRequested(item_last.filePath, root.displayArtist(item_last), item_last.cover || "")
                break;
            }
        }
    }
    
    // 获取所有歌曲文件路径
    function getAllFilePaths() {
        var paths = []
        for (var i = 0; i < musicListModel.count; i++) {
            paths.push(musicListModel.get(i).filePath)
        }
        return paths
    }
    
    // 根据filePath查找索引
    function getIndexByFilePath(filePath) {
        for (var i = 0; i < musicListModel.count; i++) {
            if (isSameTrack(musicListModel.get(i).filePath, filePath)) {
                return i
            }
        }
        return -1
    }
}

