import QtQuick 2.14
import QtQuick.Controls 2.14

Rectangle {
    id: root
    color: "transparent"
    radius: 12
    
    // 对外暴露的属性
    property bool isNetMusic: false
    
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
        
        Item {
            id: itemRoot
            width: listView.width
            height: 64
            
            property bool isHovered: false
            
            Rectangle {
                id: backgroundRect
                anchors.fill: parent
                color: itemRoot.isHovered ? "#F0F7FF" : "transparent"
                
                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    anchors.topMargin: 2
                    anchors.bottomMargin: 2
                    color: "transparent"
                    radius: 6
                    border.width: itemRoot.isHovered ? 1 : 0
                    border.color: "#D0E7FF"
                }
            }
            
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
                    width: 48
                    height: 48
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 6
                    color: "#F0F0F0"
                    border.width: 1
                    border.color: "#E0E0E0"
                    
                    Image {
                        anchors.fill: parent
                        anchors.margins: 1
                        source: model.cover !== "" ? model.cover : "qrc:/new/prefix1/icon/Music.png"
                        fillMode: Image.PreserveAspectCrop
                        smooth: true
                    }
                    
                    // 播放中的波纹效果
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        color: "#3498DB"
                        opacity: model.isPlaying ? 0.2 : 0
                        visible: model.isPlaying
                        
                        SequentialAnimation on opacity {
                            running: model.isPlaying
                            loops: Animation.Infinite
                            NumberAnimation { from: 0.2; to: 0.4; duration: 1000 }
                            NumberAnimation { from: 0.4; to: 0.2; duration: 1000 }
                        }
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
                    width: 60
                    font.pixelSize: 12
                    color: "#666666"
                    anchors.verticalCenter: parent.verticalCenter
                }
                
                // 大小
                Text {
                    text: model.fileSize || "-"
                    width: 80
                    font.pixelSize: 12
                    color: "#666666"
                    anchors.verticalCenter: parent.verticalCenter
                }
                
                Item { width: 10 }
                
                // 操作按钮（hover 时显示）
                Row {
                    spacing: 8
                    anchors.verticalCenter: parent.verticalCenter
                    opacity: itemRoot.isHovered ? 1.0 : 0.0
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
                            onEntered: itemRoot.isHovered = true
                            onExited: {
                                // 检查鼠标是否真的离开了整个item
                                var pt = mapToItem(itemRoot, mouseX, mouseY)
                                if (pt.x < 0 || pt.x > itemRoot.width || pt.y < 0 || pt.y > itemRoot.height) {
                                    itemRoot.isHovered = false
                                }
                            }
                        }
                    }
                    
                    // 删除按钮
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
                onEntered: itemRoot.isHovered = true
                onExited: itemRoot.isHovered = false
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
    
    function setPlayingState(filePath, playing) {
        console.log("[PLAY_STATE] MusicListWidget QML setPlayingState 被调用, filePath=", filePath, ", playing=", playing)
        
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
}
