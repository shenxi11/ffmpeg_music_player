import QtQuick 2.14
import QtQuick.Controls 2.14
import QtGraphicalEffects 1.14

// 播放列表面板：数据来源于真实播放队列快照，而不是本地 append 历史。
Rectangle {
    id: root
    width: 400
    height: 600
    color: "#F5F5F7"
    radius: 8
    // QML内容默认可见，由C++端的QQuickWidget控制show/hide
    
    // 阴影效果
    layer.enabled: true
    layer.effect: DropShadow {
        horizontalOffset: 0
        verticalOffset: 4
        radius: 12
        samples: 25
        color: "#40000000"
    }
    
    signal playRequested(string filePath)
    signal removeRequested(string filePath)
    signal clearAllRequested()
    signal pauseToggled()  // 暂停/继续播放信号
    
    // 当前播放的歌曲路径
    property string currentPlayingPath: ""
    property bool isPaused: false  // 是否处于暂停状态

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
        var fromPath = normalizeText(baseNameFromPath(item.filePath), "")
        return fromPath.length > 0 ? fromPath : "未知歌曲"
    }

    function displayArtist(item) {
        return normalizeText(item.artist, "未知艺术家")
    }
    
    // 标题栏
    Rectangle {
        id: headerBar
        width: parent.width
        height: 60
        color: "#FFFFFF"
        radius: 8
        
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: parent.radius
            color: parent.color
        }
        
        Text {
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            text: "播放列表"
            font.pixelSize: 18
            font.bold: true
            color: "#333333"
        }
        
        Text {
            id: countText
            anchors.left: parent.left
            anchors.leftMargin: 120
            anchors.verticalCenter: parent.verticalCenter
            text: playlistModel.count + " 首歌曲"
            font.pixelSize: 14
            color: "#666666"
        }
        
        Button {
            id: clearButton
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            width: 80
            height: 32
            
            background: Rectangle {
                color: clearButton.hovered ? "#E8E8E8" : "transparent"
                radius: 4
            }
            
            contentItem: Text {
                text: "清空"
                color: "#666666"
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            
            onClicked: {
                root.clearAllRequested()
            }
        }
    }
    
    // 分隔线
    Rectangle {
        anchors.top: headerBar.bottom
        width: parent.width
        height: 1
        color: "#E0E0E0"
    }
    
    // 歌曲列表
    ListView {
        id: listView
        anchors.top: headerBar.bottom
        anchors.topMargin: 1
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true
        
        model: ListModel {
            id: playlistModel
        }
        
        delegate: Item {
            width: listView.width
            height: 70
            
            Rectangle {
                anchors.fill: parent
                color: {
                    if (model.isCurrent || model.filePath === root.currentPlayingPath) {
                        return "#E8F5E9"  // 当前播放：淡绿色
                    }
                    return mouseArea.containsMouse ? "#F0F0F0" : "transparent"
                }
                
                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    spacing: 12
                    
                    // 专辑封面
                    Rectangle {
                        width: 50
                        height: 50
                        anchors.verticalCenter: parent.verticalCenter
                        radius: 4
                        color: "#E0E0E0"
                        
                        Image {
                            anchors.fill: parent
                            source: (model.cover && model.cover.length > 0)
                                    ? model.cover
                                    : "qrc:/new/prefix1/icon/Music.png"
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            cache: true
                            sourceSize.width: 50
                            sourceSize.height: 50
                            layer.enabled: true
                            layer.effect: OpacityMask {
                                maskSource: Rectangle {
                                    width: 50
                                    height: 50
                                    radius: 4
                                }
                            }
                        }
                    }
                    
                    // 歌曲信息
                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 50 - 100 - 24 - 12*3
                        spacing: 4
                        
                        Text {
                            text: root.displayTitle(model)
                            font.pixelSize: 14
                            font.bold: model.isCurrent || model.filePath === root.currentPlayingPath
                            color: (model.isCurrent || model.filePath === root.currentPlayingPath) ? "#EC4141" : "#333333"
                            elide: Text.ElideRight
                            width: parent.width
                        }
                        
                        Text {
                            text: root.displayArtist(model)
                            font.pixelSize: 12
                            color: "#999999"
                            elide: Text.ElideRight
                            width: parent.width
                        }
                    }
                    
                    // 播放按钮
                    Rectangle {
                        width: 32
                        height: 32
                        anchors.verticalCenter: parent.verticalCenter
                        radius: 16
                        color: playMouseArea.containsMouse ? "#E8E8E8" : "transparent"
                        
                        // 根据是否是当前播放的歌曲显示不同图标
                        Canvas {
                            id: playIcon
                            anchors.centerIn: parent
                            width: 16
                            height: 16
                            
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)
                                
                                var isCurrentSong = model.isCurrent || model.filePath === root.currentPlayingPath
                                var isPlaying = isCurrentSong && !root.isPaused
                                ctx.fillStyle = isCurrentSong ? "#EC4141" : "#666666"
                                
                                if (isPlaying) {
                                    // 绘制暂停图标（两条竖线）
                                    ctx.fillRect(4, 2, 3, 12)
                                    ctx.fillRect(9, 2, 3, 12)
                                } else {
                                    // 绘制播放图标（三角形）
                                    ctx.beginPath()
                                    ctx.moveTo(4, 2)
                                    ctx.lineTo(14, 8)
                                    ctx.lineTo(4, 14)
                                    ctx.closePath()
                                    ctx.fill()
                                }
                            }
                            
                            // 当 currentPlayingPath 变化时重绘
                            Connections {
                                target: root
                                function onCurrentPlayingPathChanged() {
                                    playIcon.requestPaint()
                                }
                                function onIsPausedChanged() {
                                    playIcon.requestPaint()
                                }
                            }
                        }
                        
                        MouseArea {
                            id: playMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (model.filePath === root.currentPlayingPath) {
                                    // 当前播放的歌曲，切换暂停/播放
                                    root.pauseToggled()
                                } else {
                                    // 其他歌曲，播放
                                    root.playRequested(model.filePath)
                                }
                            }
                        }
                    }
                    
                    // 删除按钮
                    Rectangle {
                        width: 32
                        height: 32
                        anchors.verticalCenter: parent.verticalCenter
                        radius: 16
                        color: removeMouseArea.containsMouse ? "#FFE5E5" : "transparent"
                        
                        Image {
                            anchors.centerIn: parent
                            width: 14
                            height: 14
                            source: "qrc:/new/prefix1/icon/close.png"
                            fillMode: Image.PreserveAspectFit
                        }
                        
                        MouseArea {
                            id: removeMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.removeRequested(model.filePath)
                            }
                        }
                    }
                }
                
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    z: -1
                }
            }
            
            // 分隔线
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.leftMargin: 15
                anchors.right: parent.right
                anchors.rightMargin: 15
                height: 1
                color: "#F0F0F0"
            }
        }
        
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }
    }
    
    // 空状态提示
    Text {
        anchors.centerIn: parent
        visible: playlistModel.count === 0
        text: "暂无播放列表"
        font.pixelSize: 16
        color: "#999999"
    }
    
    // 使用完整快照刷新播放队列，确保 UI 与 AudioService 队列顺序一致。
    function loadPlaylist(items) {
        playlistModel.clear()
        if (!items) return
        for (var i = 0; i < items.length; ++i) {
            playlistModel.append(items[i])
        }
    }

    // 保留兼容入口，避免旧调用链崩溃；新逻辑应优先使用 loadPlaylist。
    function addSong(filePath, title, artist, cover) {
        var foundIndex = -1
        for (var i = 0; i < playlistModel.count; ++i) {
            if (playlistModel.get(i).filePath === filePath) {
                foundIndex = i
                break
            }
        }

        var payload = {
            filePath: filePath,
            title: title,
            artist: artist,
            cover: cover,
            isCurrent: filePath === currentPlayingPath
        }

        if (foundIndex >= 0) {
            playlistModel.set(foundIndex, payload)
        } else {
            playlistModel.append(payload)
        }
    }

    function clearAll() {
        playlistModel.clear()
    }
}

