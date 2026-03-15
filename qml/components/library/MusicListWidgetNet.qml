import QtQuick 2.14
import QtQuick.Controls 2.14

Rectangle {
    id: root
    color: "#F7F9FC"
    property string listIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/list/"
    property string playerIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/player/"
    
    // 对外暴露的属性
    property int currentPlayingIndex: -1  // 当前播放的歌曲索引，-1表示没有播放
    property int sideMargin: 20
    property int innerMargin: 15
    property int colGap: 10
    property int colCoverWidth: 44
    property int colDurationWidth: width >= 1280 ? 96 : (width >= 960 ? 84 : 72)
    property int colArtistWidth: width >= 1280 ? 160 : (width >= 960 ? 130 : 110)
    property int colActionWidth: 124
    property int contentWidth: Math.max(320, width - sideMargin * 2 - innerMargin * 2)
    property int colTitleWidth: Math.max(140,
                                         contentWidth - colCoverWidth - colDurationWidth
                                         - colArtistWidth - colActionWidth - colGap * 4)
    
    // 信号
    signal playRequested(string filePath, string artist, string cover)
    signal removeRequested(string filePath)
    signal downloadRequested(string filePath)
    signal addToFavorite(string path, string title, string artist, string duration)

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
            width: listView.width
            height: 60
            color: itemArea.containsMouse || playBtnArea.containsMouse || downloadBtnArea.containsMouse ? "#f0f0f0" : "#ffffff"
            radius: 4
            
            Row {
                anchors.fill: parent
                anchors.leftMargin: root.innerMargin
                anchors.rightMargin: root.innerMargin
                spacing: root.colGap
                
                // 专辑封面
                Rectangle {
                    width: root.colCoverWidth
                    height: 44
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 4
                    color: "#E0E0E0"
                    
                    Image {
                        anchors.fill: parent
                        anchors.margins: 2
                        source: model.cover !== "" ? model.cover : "qrc:/new/prefix1/icon/Music.png"
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: true
                        sourceSize.width: 44
                        sourceSize.height: 44
                    }
                }
                
                // 歌曲名
                Text {
                    width: root.colTitleWidth
                    text: root.displayTitle(model)
                    font.pixelSize: 14
                    font.bold: model.isPlaying
                    color: model.isPlaying ? "#409EFF" : "#333333"
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
                Row {
                    width: root.colActionWidth
                    spacing: 8
                    anchors.verticalCenter: parent.verticalCenter
                    opacity: itemArea.containsMouse ? 1.0 : 0.0
                    visible: opacity > 0
                    
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                    
                    // 喜欢按钮
                    Rectangle {
                        width: 32
                        height: 32
                        radius: 16
                        color: favBtnArea.containsMouse ? "#ffe0e6" : "transparent"
                        border.width: 1
                        border.color: favBtnArea.containsMouse ? "#EC4141" : "#D6DCE8"
                        
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
                                root.addToFavorite(
                                    model.filePath || "",
                                    root.displayTitle(model),
                                    root.displayArtist(model),
                                    model.duration || ""
                                )
                            }
                        }
                    }
                    
                    // 播放按钮
                    Rectangle {
                        width: 32
                        height: 32
                        radius: 16
                        color: model.isPlaying ? "#EC4141" : (playBtnArea.containsMouse ? "#FDECEC" : "transparent")
                        border.width: 1
                        border.color: model.isPlaying ? "#EC4141" : "#D6DCE8"
                        
                        Image {
                            anchors.centerIn: parent
                            width: 18
                            height: 18
                            source: {
                                if (model.isPlaying) {
                                    return playBtnArea.containsMouse
                                            ? root.playerIconPrefix + "player_btn_pause_hover.svg"
                                            : root.playerIconPrefix + "player_btn_pause_default.svg"
                                }
                                return playBtnArea.containsMouse
                                        ? root.playerIconPrefix + "player_btn_play_hover.svg"
                                        : root.playerIconPrefix + "player_btn_play_default.svg"
                            }
                            fillMode: Image.PreserveAspectFit
                        }
                        
                        MouseArea {
                            id: playBtnArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                console.log("[MusicListWidgetNet] Play clicked - path:", model.filePath)
                                console.log("[MusicListWidgetNet] artist:", model.artist)
                                console.log("[MusicListWidgetNet] cover:", model.cover)
                                root.playRequested(model.filePath, root.displayArtist(model), model.cover || "")
                            }
                        }
                    }
                    
                    // 下载按钮（网络音乐）
                    Rectangle {
                        width: 32
                        height: 32
                        radius: 16
                        color: downloadBtnArea.containsMouse ? "#FDECEC" : "transparent"
                        border.width: 1
                        border.color: downloadBtnArea.containsMouse ? "#EC4141" : "#D6DCE8"
                        
                        Image {
                            anchors.centerIn: parent
                            width: 18
                            height: 18
                            source: downloadBtnArea.containsMouse
                                    ? root.listIconPrefix + "list_icon_download_hover.svg"
                                    : root.listIconPrefix + "list_icon_download_default.svg"
                            fillMode: Image.PreserveAspectFit
                        }
                        
                        MouseArea {
                            id: downloadBtnArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.downloadRequested(model.filePath)
                        }
                    }
                }
            }
            
            MouseArea {
                id: itemArea
                anchors.fill: parent
                hoverEnabled: true
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
            if (musicListModel.get(i).filePath === filePath) {
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
        // 如果 filePath 为空，则清除当前播放状态
        if (filePath === "") {
            if (root.currentPlayingIndex >= 0 && root.currentPlayingIndex < musicListModel.count) {
                musicListModel.get(root.currentPlayingIndex).isPlaying = false
            }
            root.currentPlayingIndex = -1
            return
        }
        
        // 从完整 URL 中提取相对路径（如果是网络路径）
        var pathToMatch = filePath
        if (filePath.indexOf("http") === 0) {
            var uploadsIndex = filePath.indexOf("/uploads/")
            if (uploadsIndex !== -1) {
                pathToMatch = filePath.substring(uploadsIndex + 9)
            }
        }
        
        // 查找匹配的歌曲
        for (var i = 0; i < musicListModel.count; i++) {
            if (musicListModel.get(i).filePath === pathToMatch) {
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
                    // 关闭当前
                    musicListModel.get(i).isPlaying = false
                    if (root.currentPlayingIndex === i) {
                        root.currentPlayingIndex = -1
                    }
                }
                return
            }
        }
        
        // 没有找到，可能是其他列表的歌曲
        if (playing && root.currentPlayingIndex >= 0) {
            musicListModel.get(root.currentPlayingIndex).isPlaying = false
            root.currentPlayingIndex = -1
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
            if (musicListModel.get(i).filePath === filePath) {
                return i
            }
        }
        return -1
    }
}

