import QtQuick 2.14
import QtQuick.Controls 2.14

Rectangle {
    id: root
    color: "#F7F9FC"
    
    // 对外暴露的属性
    property string downloadDir: ""
    
    // 信号
    signal playRequested(string filePath)
    signal removeRequested(string filePath)
    signal downloadRequested(string filePath)
    signal chooseDownloadDir()

    // 顶部下载路径栏
    Rectangle {
        id: topBar
        width: parent.width
        height: 50
        color: "white"
        anchors.top: parent.top
        
        Rectangle {
            width: parent.width
            height: 1
            color: "#E0E0E0"
            anchors.bottom: parent.bottom
        }
        
        Row {
            anchors.left: parent.left
            anchors.leftMargin: 15
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            
            // 下载路径按钮
            Rectangle {
                width: 100
                height: 32
                radius: 4
                color: "transparent"
                
                Text {
                    anchors.centerIn: parent
                    text: "下载路径:"
                    font.pixelSize: 14
                    color: "#333333"
                }
                
                MouseArea {
                    id: dirBtnArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.chooseDownloadDir()
                }
            }
            
            // 路径显示
            Text {
                text: root.downloadDir || "未设置"
                font.pixelSize: 13
                color: "#666666"
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
    
    // 列表头部（标题栏）
    Rectangle {
        id: headerBar
        width: parent.width
        height: 40
        color: "#FAFAFA"
        anchors.top: topBar.bottom
        
        Rectangle {
            width: parent.width
            height: 1
            color: "#E0E0E0"
            anchors.bottom: parent.bottom
        }
        
        Row {
            anchors.fill: parent
            anchors.leftMargin: 15
            anchors.rightMargin: 15
            spacing: 10
            
            Text {
                text: "#"
                width: 50
                font.pixelSize: 12
                color: "#999999"
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: "标题"
                width: 350
                font.pixelSize: 12
                color: "#999999"
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Item { width: 50 }
            
            Text {
                text: "时长"
                width: 100
                font.pixelSize: 12
                color: "#999999"
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
    
    // 音乐列表（使用 ListView 实现懒加载）
    ListView {
        id: listView
        anchors.top: headerBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 0
        clip: true
        
        model: musicListModel
        delegate: musicItemDelegate
        spacing: 2
        
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
            color: itemArea.containsMouse ? "#E8F4FF" : (index % 2 === 0 ? "#FFFFFF" : "#FAFAFA")
            
            Row {
                anchors.fill: parent
                anchors.leftMargin: 15
                anchors.rightMargin: 15
                spacing: 10
                
                // 序号
                Text {
                    text: (index + 1).toString()
                    width: 50
                    font.pixelSize: 13
                    color: "#666666"
                    anchors.verticalCenter: parent.verticalCenter
                }
                
                // 封面
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
                    }
                }
                
                // 歌曲信息
                Column {
                    width: 280
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4
                    
                    Text {
                        text: model.songName
                        font.pixelSize: 14
                        font.bold: model.isPlaying
                        color: model.isPlaying ? "#4A90E2" : "#333333"
                        elide: Text.ElideRight
                        width: parent.width
                    }
                    
                    Text {
                        text: model.artist || "未知艺术家"
                        font.pixelSize: 11
                        color: "#888888"
                    }
                }
                
                Item { width: 20 }
                
                // 时长
                Text {
                    text: model.duration || "0:00"
                    width: 100
                    font.pixelSize: 12
                    color: "#666666"
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
                                root.playRequested(model.filePath)
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
                    root.playRequested(model.filePath)
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
    
    function addSongList(songNames, durations, coverUrls) {
        coverUrls = coverUrls || []
        for (var i = 0; i < songNames.length; i++) {
            musicListModel.append({
                                      "songName": songNames[i],
                                      "filePath": songNames[i],
                                      "artist": "",
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
    
    function setDownloadDir(dir) {
        root.downloadDir = dir
    }
    
    function setPlayingState(filePath, playing) {
        console.log("[PLAY_STATE] MusicListWidgetNet QML setPlayingState 被调用, filePath=", filePath, ", playing=", playing)
        
        // 如果 filePath 为空，则清除所有播放状态
        if (filePath === "") {
            console.log("[PLAY_STATE] filePath 为空，清除所有播放状态")
            for (var i = 0; i < musicListModel.count; i++) {
                musicListModel.get(i).isPlaying = false
            }
            return
        }
        
        console.log("[PLAY_STATE] 当前列表有", musicListModel.count, "首歌曲")
        
        // 从完整 URL 中提取相对路径（如果是网络路径）
        var pathToMatch = filePath
        if (filePath.indexOf("http") === 0) {
            // 提取 /uploads/ 之后的部分
            var uploadsIndex = filePath.indexOf("/uploads/")
            if (uploadsIndex !== -1) {
                pathToMatch = filePath.substring(uploadsIndex + 9) // 9 是 "/uploads/" 的长度
                console.log("[PLAY_STATE] 从 URL 提取路径:", pathToMatch)
            }
        }
        
        // 先将所有歌曲设置为非播放状态
        var found = false
        for (var i = 0; i < musicListModel.count; i++) {
            var item = musicListModel.get(i)
            console.log("[PLAY_STATE] 检查项", i, ": filePath=", item.filePath)
            if (item.filePath === pathToMatch) {
                console.log("[PLAY_STATE] 找到匹配项，设置 isPlaying=", playing)
                item.isPlaying = playing
                found = true
            } else if (playing) {
                // 如果正在播放新歌曲，停止其他歌曲
                item.isPlaying = false
            }
        }
        if (!found) {
            console.log("[PLAY_STATE] 警告：没有找到匹配的 filePath!")
        }
    }

    function playNext(songName){
        for(var i = 0; i < musicListModel.count; i++){
            var item = musicListModel.get(i)
            if(item.songName === songName){
                var item_next = musicListModel.get((i + 1) % musicListModel.count);
                item.isPlaying = false
                item_next.isPlaying = true
                root.playRequested(item_next.filePath)
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
                root.playRequested(item_last.filePath)
                break;
            }
        }
    }
}
