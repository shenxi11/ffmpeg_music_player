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

    signal playMusic(string filename)
    signal playMusicWithMetadata(string filePath, string title, string artist, string cover)
    signal addToFavorite(string filePath, string title, string artist, string duration, bool isLocal)
    signal deleteHistory(var selectedPaths)
    signal loginRequested()
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

    function _normalizePath(path) {
        if (!path) return ""
        var value = String(path).trim()
        if (value.indexOf("file:///") === 0) {
            value = value.substring(8)
        }
        var qPos = value.indexOf("?")
        if (qPos > 0) {
            value = value.substring(0, qPos)
        }
        try {
            value = decodeURIComponent(value)
        } catch (e) {
        }
        return value
    }

    function _isSameTrack(pathA, pathB) {
        return _normalizePath(pathA) === _normalizePath(pathB)
    }

    function setPlayingState(filePath, playing) {
        root.currentPlayingPath = filePath || ""
        root.isPlaying = playing
    }

    ListModel {
        id: historyModel
    }

    // 当前多选项（用于批量删除等操作）
    property var selectedItems: []

    Rectangle {
        anchors.fill: parent
        visible: !root.isLoggedIn
        color: "#f5f5f5"

        Column {
            anchors.centerIn: parent
            spacing: 20

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "♪"
                font.pixelSize: 60
                color: "#409EFF"
                font.bold: true
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "登录后可显示最近播放歌曲"
                font.pixelSize: 18
                color: "#333333"
                font.bold: true
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "最近播放历史支持云漫游"
                font.pixelSize: 14
                color: "#999999"
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 160
                height: 42
                text: "立即登录"

                background: Rectangle {
                    color: parent.pressed ? "#3a8ee6" : 
                           parent.hovered ? "#66b1ff" : "#409EFF"
                    radius: 4
                }

                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 15
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    root.loginRequested()
                }
            }
        }
    }

    // 登录后展示的播放历史主内容区
    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        visible: root.isLoggedIn

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            spacing: 10

            Text {
                text: "播放历史"
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
                    text: "播放时间"
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

        // 播放历史列表
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            spacing: 2
            clip: true

            model: historyModel

            delegate: Rectangle {
                id: itemRoot
                width: listView.width
                height: 60
                color: itemRoot.containsMouse ? "#E8F4FF" : (index % 2 === 0 ? "#FFFFFF" : "#FAFAFA")

                property bool containsMouse: false
                property bool isCurrentTrack: root._isSameTrack(root.currentPlayingPath, model.path)
                property bool showPauseIcon: isCurrentTrack && root.isPlaying

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    spacing: 10

                    // 序号列
                    Text {
                        text: (index + 1).toString()
                        width: 50
                        font.pixelSize: 13
                        color: "#666666"
                        anchors.verticalCenter: parent.verticalCenter
                    }

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
                                    console.log("[PlayHistory] Failed to load cover:", source)
                                    source = "qrc:/new/prefix1/icon/Music.png"
                                }
                            }
                        }
                    }

                    // 歌曲信息列（标题 + 艺术家）
                    Column {
                        width: 240
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 4

                        Text {
                            text: root.displayTitle(model)
                            font.pixelSize: 14
                            font.bold: itemRoot.isCurrentTrack
                            color: itemRoot.isCurrentTrack ? "#4A90E2" : "#333333"
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

                    // 时长列
                    Text {
                        text: model.duration || "--:--"
                        width: 80
                        font.pixelSize: 12
                        color: "#666666"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: model.play_time || ""
                        width: 130
                        font.pixelSize: 12
                        color: "#999999"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Item { width: 10 }

                    // 操作按钮区（悬停时显示）
                    Row {
                        spacing: 8
                        anchors.verticalCenter: parent.verticalCenter
                        opacity: itemRoot.containsMouse ? 1.0 : 0.0
                        visible: opacity > 0

                        Behavior on opacity { NumberAnimation { duration: 150 } }

                        Rectangle {
                            width: 32
                            height: 32
                            radius: 16
                            color: favBtnArea.containsMouse ? "#ffe0e6" : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: "♥"
                                font.pixelSize: 16
                                color: favBtnArea.containsMouse ? "#ff0000" : "#999999"
                            }

                            MouseArea {
                                id: favBtnArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    var filePath = model.path || ""
                                    var title = root.displayTitle(model)
                                    var artist = root.displayArtist(model)
                                    var duration = model.duration || "0:00"
                                    var isLocal = !(filePath.indexOf("http://") === 0 || filePath.indexOf("https://") === 0)
                                    root.addToFavorite(filePath, title, artist, duration, isLocal)
                                }
                            }
                        }

                        Rectangle {
                            width: 32
                            height: 32
                            radius: 16
                            color: playBtnArea.containsMouse ? "#4A90E2" : "#DDDDDD"

                            Text {
                                anchors.centerIn: parent
                                text: itemRoot.showPauseIcon ? "⏸" : "▶"
                                font.pixelSize: 13
                                color: playBtnArea.containsMouse ? "white" : "#333333"
                            }

                            MouseArea {
                                id: playBtnArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    var filePath = model.path || ""
                                    var title = root.displayTitle(model)
                                    var artist = root.displayArtist(model)
                                    var cover = model.cover_art_url || ""
                                    root.playMusicWithMetadata(filePath, title, artist, cover)
                                }
                            }
                        }

                        // 删除按钮
                        Rectangle {
                            width: 60
                            height: 28
                            radius: 4
                            color: deleteBtnArea.containsMouse ? "#E74C3C" : "#F0F0F0"

                            Text {
                                anchors.centerIn: parent
                                text: "删除"
                                font.pixelSize: 12
                                color: deleteBtnArea.containsMouse ? "white" : "#666666"
                            }

                            MouseArea {
                                id: deleteBtnArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.deleteHistory([model.path])
                                }
                            }
                        }
                    }
                }

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
                        var filePath = model.path || ""
                        var title = root.displayTitle(model)
                        var artist = root.displayArtist(model)
                        var cover = model.cover_art_url || ""
                        root.playMusicWithMetadata(filePath, title, artist, cover)
                    }
                }
            }

            // 空状态提示
            Label {
                anchors.centerIn: parent
                text: "暂无播放历史"
                font.pixelSize: 16
                color: "#AAAAAA"
                visible: listView.count === 0
            }
        }
    }

    function loadHistory(historyData) {
        historyModel.clear()
        root.selectedItems = []
        
        console.log("[PlayHistoryList] Loading", historyData.length, "items")
        
        for (var i = 0; i < historyData.length; i++) {
            var item = historyData[i]
            if (!item.artist || item.artist.length === 0) {
                item.artist = item.singer || item.author || item.artist_name || ""
            }
            item.title = root.displayTitle(item)
            item.artist = root.displayArtist(item)
            // 统一归一化字段，避免后端字段不一致导致显示异常
            console.log("[PlayHistoryList] Item", i, "- title:", item.title, "cover_art_url:", item.cover_art_url)
            
            historyModel.append(item)
        }
    }

    // 清空本地列表展示（不触发服务端删除）
    function clearHistory() {
        historyModel.clear()
        root.selectedItems = []
    }
}

