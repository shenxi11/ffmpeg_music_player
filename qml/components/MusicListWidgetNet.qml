import QtQuick 2.14
import QtQuick.Controls 2.14

Rectangle {
    id: root
    color: "#F7F9FC"
    
    // 对外暴露的属性
    property int currentPlayingIndex: -1  // 当前播放的歌曲索引，-1表示没有播放
    
    // 信号
    signal playRequested(string filePath, string artist, string cover)
    signal removeRequested(string filePath)
    signal downloadRequested(string filePath)
    signal addToFavorite(string path, string title, string artist, string duration)

    // 顶部标题
    Text {
        id: topTitle
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 20
        anchors.leftMargin: 20
        text: "在线音乐"
        font.pixelSize: 20
        font.bold: true
        color: "#333333"
    }
    
    // 列表头部
    Rectangle {
        id: headerBar
        width: parent.width
        height: 40
        color: "#ffffff"
        anchors.top: topTitle.bottom
        anchors.topMargin: 10
        radius: 4
        
        Row {
            anchors.fill: parent
            anchors.leftMargin: 15
            anchors.rightMargin: 15
            spacing: 10
            
            Text {
                text: "封面"
                width: 50
                font.pixelSize: 14
                font.bold: true
                color: "#333333"
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: "歌曲名"
                width: 240
                font.pixelSize: 14
                font.bold: true
                color: "#333333"
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: "时长"
                width: 80
                font.pixelSize: 14
                font.bold: true
                color: "#333333"
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: "艺术家"
                width: 120
                font.pixelSize: 14
                font.bold: true
                color: "#333333"
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: "操作"
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
                anchors.leftMargin: 15
                anchors.rightMargin: 15
                spacing: 10
                
                // 专辑封面
                Rectangle {
                    width: 44
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
                    }
                }
                
                // 歌曲名
                Text {
                    width: 240
                    text: model.songName
                    font.pixelSize: 14
                    font.bold: model.isPlaying
                    color: model.isPlaying ? "#409EFF" : "#333333"
                    elide: Text.ElideRight
                    anchors.verticalCenter: parent.verticalCenter
                }
                
                // 时长
                Text {
                    width: 80
                    text: model.duration || "0:00"
                    font.pixelSize: 13
                    color: "#666666"
                    anchors.verticalCenter: parent.verticalCenter
                }
                
                // 艺术家
                Text {
                    width: 120
                    text: model.artist || "未知艺术家"
                    font.pixelSize: 13
                    color: "#888888"
                    elide: Text.ElideRight
                    anchors.verticalCenter: parent.verticalCenter
                }
                
                Item { width: 10 }
                
                // 操作按钮（hover 时显示）
                Row {
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
                        
                        Text {
                            anchors.centerIn: parent
                            text: "♡"
                            font.pixelSize: 16
                            color: favBtnArea.containsMouse ? "#ff0000" : "#999999"
                        }
                        
                        MouseArea {
                            id: favBtnArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.addToFavorite(
                                    model.filePath || "",
                                    model.filePath ? model.filePath.split('/').pop() : "",
                                    model.artist || "",
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
                        color: playBtnArea.containsMouse ? "#4A90E2" : "#DDDDDD"
                        
                        Text {
                            anchors.centerIn: parent
                            text: model.isPlaying ? "⏸" : "▶"
                            font.pixelSize: 14
                            color: playBtnArea.containsMouse ? "white" : "#333333"
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
                                root.playRequested(model.filePath, model.artist || "未知艺术家", model.cover || "")
                            }
                        }
                    }
                    
                    // 下载按钮（网络音乐）
                    Rectangle {
                        width: 60
                        height: 28
                        radius: 4
                        color: downloadBtnArea.containsMouse ? "#51CF66" : "#F0F0F0"
                        
                        Text {
                            anchors.centerIn: parent
                            text: "下载"
                            font.pixelSize: 12
                            color: downloadBtnArea.containsMouse ? "white" : "#666666"
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
                    root.playRequested(model.filePath, model.artist || "未知艺术家", model.cover || "")
                }
            }
        }
    }
    
    // 提供给 C++ 调用的方法
    function addSong(songName, filePath, artist, duration, cover) {
        musicListModel.append({
                                  "songName": songName,
                                  "filePath": filePath,
                                  "artist": artist || "",
                                  "duration": duration || "0:00",
                                  "cover": cover || "",
                                  "isPlaying": false
                              })
    }
    
    function addSongList(songNames, relativePaths, durations, coverUrls, artists) {
        coverUrls = coverUrls || []
        artists = artists || []
        for (var i = 0; i < songNames.length; i++) {
            musicListModel.append({
                                      "songName": songNames[i],
                                      "filePath": relativePaths[i],
                                      "artist": artists[i] || "未知艺术家",
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
                root.playRequested(item_next.filePath, item_next.artist || "未知艺术家", item_next.cover || "")
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
                root.playRequested(item_last.filePath, item_last.artist || "未知艺术家", item_last.cover || "")
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
