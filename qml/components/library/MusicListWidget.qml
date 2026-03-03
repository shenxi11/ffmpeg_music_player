import QtQuick 2.14
import QtQuick.Controls 2.14

Rectangle {
    id: root
    color: "transparent"
    radius: 12
    
    // 对外暴露的属性
    property bool isNetMusic: false
    property int currentPlayingIndex: -1  // 当前播放的歌曲索引，-1表示没有播放
    
    // 信号
    signal playRequested(string filePath)
    signal removeRequested(string filePath)
    signal downloadRequested(string filePath)
    signal addButtonClicked()
    
    // 顶部按钮栏
    Rectangle {
        id: topBar
        width: parent.width
        height: 56
        color: "transparent"
        anchors.top: parent.top
        
        Rectangle {
            width: parent.width
            height: 1
            color: "#E8E8E8"
            anchors.bottom: parent.bottom
        }
        
        Row {
            anchors.left: parent.left
            anchors.leftMargin: 15
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            
            // 添加按钮
            Rectangle {
                width: 100
                height: 36
                radius: 18
                color: addBtnArea.containsMouse ? "#3498DB" : "#FFFFFF"
                border.width: 1.5
                border.color: addBtnArea.containsMouse ? "#3498DB" : "#D0D7DE"
                
                Text {
                    anchors.centerIn: parent
                    text: "+ 添加歌曲"
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: addBtnArea.containsMouse ? "white" : "#57606A"
                }
                
                MouseArea {
                    id: addBtnArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.addButtonClicked()
                }
            }
        }
    }
    
    // 列表头部（标题栏）
    Rectangle {
        id: headerBar
        width: parent.width
        height: 42
        color: "#F6F8FA"
        anchors.top: topBar.bottom
        
        Rectangle {
            width: parent.width
            height: 1
            color: "#E8E8E8"
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
                width: 60
                font.pixelSize: 12
                color: "#999999"
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: "大小"
                width: 80
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
        
        // 启用滚动
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.VerticalFlick
        
        // 滚动条
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            width: 8
            interactive: true
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
            color: itemRoot.containsMouse ? "#E8F4FF" : (index % 2 === 0 ? "#FFFFFF" : "#FAFAFA")
            
            property bool containsMouse: false
            
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
                    opacity: itemRoot.containsMouse ? 1.0 : 0.0
                    visible: opacity > 0
                    
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                    
                    // 播放按钮
                    Rectangle {
                        width: 36
                        height: 36
                        radius: 18
                        color: playBtnArea.containsMouse ? (model.isPlaying ? "#E74C3C" : "#3498DB") : "transparent"
                        border.width: 1
                        border.color: model.isPlaying ? "#3498DB" : (playBtnArea.containsMouse ? "transparent" : "#BDC3C7")
                        anchors.verticalCenter: parent.verticalCenter
                        
                        Image {
                            anchors.centerIn: parent
                            width: 16
                            height: 16
                            source: model.isPlaying ? "qrc:/new/prefix1/icon/pause_w.png" : (playBtnArea.containsMouse ? "qrc:/new/prefix1/icon/play_w.png" : "qrc:/new/prefix1/icon/play.png")
                            fillMode: Image.PreserveAspectFit
                            smooth: true
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
                    
                    // 删除按钮（本地音乐）
                    Rectangle {
                        width: 68
                        height: 32
                        radius: 6
                        color: removeBtnArea.containsMouse ? "#E74C3C" : "transparent"
                        border.width: 1
                        border.color: removeBtnArea.containsMouse ? "#E74C3C" : "#D0D7DE"
                        visible: !root.isNetMusic
                        
                        Text {
                            anchors.centerIn: parent
                            text: "删除"
                            font.pixelSize: 13
                            font.weight: Font.Medium
                            color: removeBtnArea.containsMouse ? "white" : "#57606A"
                        }
                        
                        MouseArea {
                            id: removeBtnArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.removeRequested(model.filePath)
                            onEntered: itemRoot.isHovered = true
                            onExited: {
                                var pt = mapToItem(itemRoot, mouseX, mouseY)
                                if (pt.x < 0 || pt.x > itemRoot.width || pt.y < 0 || pt.y > itemRoot.height) {
                                    itemRoot.isHovered = false
                                }
                            }
                        }
                    }
                    
                    // 下载按钮
                    Rectangle {
                        width: 68
                        height: 32
                        radius: 6
                        color: downloadBtnArea.containsMouse ? "#27AE60" : "transparent"
                        border.width: 1
                        border.color: downloadBtnArea.containsMouse ? "#27AE60" : "#D0D7DE"
                        visible: root.isNetMusic
                        
                        Text {
                            anchors.centerIn: parent
                            text: "下载"
                            font.pixelSize: 13
                            font.weight: Font.Medium
                            color: downloadBtnArea.containsMouse ? "white" : "#57606A"
                        }
                        
                        MouseArea {
                            id: downloadBtnArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.downloadRequested(model.filePath)
                            onEntered: itemRoot.isHovered = true
                            onExited: {
                                var pt = mapToItem(itemRoot, mouseX, mouseY)
                                if (pt.x < 0 || pt.x > itemRoot.width || pt.y < 0 || pt.y > itemRoot.height) {
                                    itemRoot.isHovered = false
                                }
                            }
                        }
                    }
                }
            }
            
            // 整个item的悬停检测
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                propagateComposedEvents: true
                onEntered: itemRoot.containsMouse = true
                onExited: itemRoot.containsMouse = false
                onPressed: mouse.accepted = false
                onReleased: mouse.accepted = false
                onClicked: mouse.accepted = false
            }
        }
    }
    
    // 提供给 C++ 调用的方法
    function addSong(songName, filePath, artist, duration, cover, fileSize) {
        musicListModel.append({
            "songName": songName,
            "filePath": filePath,
            "artist": artist || "",
            "duration": duration || "0:00",
            "cover": cover || "",
            "fileSize": fileSize || "-",
            "isPlaying": false
        })
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
    
    // 获取所有歌曲路径列表（用于同步到AudioService）
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
        for (var i = 0; i < musicListModel.count; i++) {
            var item = musicListModel.get(i)
            if (item.songName === songName) {
                item.isPlaying = false;
                var item_next = musicListModel.get((i + 1) % musicListModel.count);
                item_next.isPlaying =true;
                root.playRequested(item_next.filePath)
                break
            }
        }
    }
    function playLast(songName){
        for (var i = 0; i < musicListModel.count; i++) {
            var item = musicListModel.get(i)
            if (item.songName === songName) {
                item.isPlaying = false;
                var item_last = musicListModel.get((i - 1) < 0 ? musicListModel.count - 1 : i  - 1);
                item_last.isPlaying =true;
                root.playRequested(item_last.filePath)
                break
            }
        }
    }
    
    // 更新音乐项的封面和时长（从解码器获取）
    function updateSongMetadata(filePath, coverUrl, duration) {
        for (var i = 0; i < musicListModel.count; i++) {
            if (musicListModel.get(i).filePath === filePath) {
                musicListModel.setProperty(i, "cover", coverUrl)
                musicListModel.setProperty(i, "duration", duration)
                console.log("Updated metadata for:", filePath, "cover:", coverUrl, "duration:", duration)
                break
            }
        }
    }
}
