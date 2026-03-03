import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// 喜欢音乐列表
Rectangle {
    id: root
    color: "#f5f5f5"

    property string userAccount: ""
    property string currentPlayingPath: ""  // 当前播放的音乐路径

    signal playMusic(string filename)
    signal removeFavorite(var selectedPaths)
    signal refreshRequested()

    function _looksUnreadable(value) {
        if (value === undefined || value === null) return true
        var text = String(value).trim()
        if (text.length === 0) return true
        if (/^[\?？\s]+$/.test(text)) return true
        var suspicious = text.match(/[鍙鍚鍛鍜鍝鎵鎺鏄鏃鏂鏈鏉鏋鏌鏍鐨缁璁妫娓绛鎻锛]/g)
        return suspicious && suspicious.length >= 3
    }

    function _baseNameFromPath(path) {
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
        if (_looksUnreadable(value)) return fallbackText
        return String(value)
    }

    function displayTitle(item) {
        var title = normalizeText(item.title, "")
        if (title.length > 0) return title
        var fromPath = normalizeText(_baseNameFromPath(item.path), "")
        return fromPath.length > 0 ? fromPath : "未知歌曲"
    }

    function displayArtist(item) {
        return normalizeText(item.artist, "未知艺术家")
    }

    // 喜欢音乐数据模型
    ListModel {
        id: favoriteModel
    }

    // 选中的项目
    property var selectedItems: []

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        // 标题栏和操作按钮
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            spacing: 10

            Text {
                text: "我喜欢的音乐"
                font.pixelSize: 20
                font.bold: true
                color: "#333333"
                Layout.fillWidth: true
            }

            Button {
                text: "刷新"
                
                background: Rectangle {
                    color: parent.pressed ? "#3a8ee6" : 
                           parent.hovered ? "#66b1ff" : "#409EFF"
                    radius: 4
                }

                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    root.refreshRequested()
                }
            }
        }

        // 列表表头（固定显示）
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            color: "#F0F2F5"
            radius: 4

            Row {
                anchors.fill: parent
                anchors.leftMargin: 15
                anchors.rightMargin: 15
                spacing: 10

                Text {
                    text: "序号"
                    width: 50
                    font.pixelSize: 12
                    color: "#666666"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item {
                    width: 44
                    height: 1
                }

                Text {
                    text: "音频信息"
                    width: 240
                    font.pixelSize: 12
                    color: "#666666"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item { width: 10 }

                Text {
                    text: "时长"
                    width: 80
                    font.pixelSize: 12
                    color: "#666666"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: "添加时间"
                    width: 130
                    font.pixelSize: 12
                    color: "#666666"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item {
                    width: 74
                    height: 1
                }
            }
        }

        // 音乐列表
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            spacing: 2
            clip: true

            model: favoriteModel

            delegate: Rectangle {
                id: itemRoot
                width: listView.width
                height: 60
                color: itemRoot.containsMouse ? "#E8F4FF" : (index % 2 === 0 ? "#FFFFFF" : "#FAFAFA")

                property bool containsMouse: false
                property bool isPlaying: root.currentPlayingPath === model.path

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

                    // 封面（真实封面或默认音乐图标）
                    Rectangle {
                        width: 44
                        height: 44
                        anchors.verticalCenter: parent.verticalCenter
                        radius: 4
                        color: "#E0E0E0"

                        Image {
                            id: coverImage
                            anchors.fill: parent
                            anchors.margins: 2
                            source: {
                                var url = model.cover_art_url || ""
                                if (url && url.length > 0) {
                                    return url
                                }
                                return "qrc:/new/prefix1/icon/Music.png"
                            }
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            cache: true
                            sourceSize.width: 44
                            sourceSize.height: 44
                            
                            onStatusChanged: {
                                if (status === Image.Error) {
                                    console.log("[FavoriteMusic] Failed to load cover:", source)
                                    source = "qrc:/new/prefix1/icon/Music.png"
                                }
                            }
                        }
                    }

                    // 歌曲信息列
                    Column {
                        width: 240
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 4

                        Text {
                            text: root.displayTitle(model)
                            font.pixelSize: 14
                            font.bold: itemRoot.isPlaying
                            color: itemRoot.isPlaying ? "#4A90E2" : "#333333"
                            elide: Text.ElideRight
                            width: parent.width
                        }

                        Text {
                            text: root.displayArtist(model)
                            font.pixelSize: 11
                            color: "#888888"
                        }
                    }

                    Item { width: 10 }

                    // 时长
                    Text {
                        text: model.duration || "--:--"
                        width: 80
                        font.pixelSize: 12
                        color: "#666666"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    // 添加时间
                    Text {
                        text: model.added_at || ""
                        width: 130
                        font.pixelSize: 12
                        color: "#999999"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Item { width: 10 }

                    // 操作按钮（hover时显示）
                    Row {
                        spacing: 8
                        anchors.verticalCenter: parent.verticalCenter
                        opacity: itemRoot.containsMouse ? 1.0 : 0.0
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
                                text: itemRoot.isPlaying ? "⏸" : "▶"
                                font.pixelSize: 14
                                color: playBtnArea.containsMouse ? "white" : "#333333"
                            }

                            MouseArea {
                                id: playBtnArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.playMusic(model.path || "")
                                }
                            }
                        }

                        // 取消喜欢按钮（红色心形）
                        Rectangle {
                            width: 32
                            height: 32
                            radius: 16
                            color: removeBtnArea.containsMouse ? "#ffe0e6" : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: "♥"
                                font.pixelSize: 16
                                color: removeBtnArea.containsMouse ? "#ff0000" : "#ff4d4f"
                            }

                            MouseArea {
                                id: removeBtnArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.removeFavorite([model.path])
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
                    onDoubleClicked: {
                        root.playMusic(model.path || "")
                    }
                }
            }

            // 空状态
            Label {
                anchors.centerIn: parent
                text: "暂无喜欢的音乐"
                font.pixelSize: 16
                color: "#AAAAAA"
                visible: listView.count === 0
            }
        }
    }

    // 公共函数：加载喜欢音乐
    function loadFavorites(favoritesData) {
        favoriteModel.clear()
        root.selectedItems = []
        
        console.log("[FavoriteMusicList] Loading", favoritesData.length, "items")
        
        for (var i = 0; i < favoritesData.length; i++) {
            var item = favoritesData[i]
            item.uniqueId = i  // 添加唯一ID
            item.title = root.displayTitle(item)
            item.artist = root.displayArtist(item)
            
            // 调试输出
            console.log("[FavoriteMusicList] Item", i, "- title:", item.title, "cover_art_url:", item.cover_art_url)
            
            favoriteModel.append(item)
        }
    }

    // 清空列表
    function clearFavorites() {
        favoriteModel.clear()
        root.selectedItems = []
    }
}

