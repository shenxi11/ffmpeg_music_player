import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// 已下载音乐列表
Rectangle {
    id: root
    color: "#f5f5f5"
    property string listIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/list/"
    property string playerIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/player/"
    property int colCoverWidth: 44
    property int colSizeWidth: width >= 1280 ? 130 : (width >= 960 ? 110 : 96)
    property int colTimeWidth: width >= 1280 ? 170 : (width >= 960 ? 142 : 120)
    property int colActionWidth: width >= 1280 ? 170 : 150
    property int rowContentWidth: Math.max(320, width - 70)
    property int colNameWidth: Math.max(150,
                                        rowContentWidth - colCoverWidth - colSizeWidth
                                        - colTimeWidth - colActionWidth - 40)
    property var availablePlaylists: []
    property var favoritePaths: []
    property string currentPlayingPath: ""
    property bool isPlaying: false

    signal playMusic(string filename)
    signal deleteMusic(string filename)
    signal addToFavorite(string path, string title, string artist, string duration)
    signal songActionRequested(string action, var song)

    function normalizePath(path) {
        if (!path) return ""
        var value = String(path).trim()
        if (value.indexOf("file:///") === 0)
            value = value.substring(8)
        try {
            value = decodeURIComponent(value)
        } catch (e) {
        }
        return value
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

    function isSameTrack(pathA, pathB) {
        return normalizePath(pathA) === normalizePath(pathB)
    }

    function buildSongPayload(item) {
        return {
            path: item.savePath || "",
            playPath: item.savePath || "",
            title: item.songName || item.filename || "",
            artist: item.artist || "未知艺术家",
            duration: item.duration || "0:00",
            cover: item.coverUrl || "",
            isLocal: true,
            isFavorite: isFavoritePath(item.savePath || ""),
            sourceType: "downloaded"
        }
    }
    
    // 当可见性改变时刷新
    onVisibleChanged: {
        if (visible) {
            console.log("[DownloadedMusicList] Became visible, refreshing completed tasks...")
            if (typeof downloadTaskModel !== 'undefined') {
                downloadTaskModel.refresh(true)
            }
        }
    }

    Component.onCompleted: {
        console.log("[DownloadedMusicList] Component completed")
        // 刷新已下载的任务
        if (typeof downloadTaskModel !== 'undefined') {
            console.log("[DownloadedMusicList] Initial refresh")
            downloadTaskModel.refresh(true)
        }
    }
    
    // 添加连接监听模型变化
    Connections {
        target: typeof downloadTaskModel !== 'undefined' ? downloadTaskModel : null
        function onRowsInserted() {
            console.log("[DownloadedMusicList] Rows inserted, count:", listView.count)
        }
        function onModelReset() {
            console.log("[DownloadedMusicList] Model reset, count:", listView.count)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        // 标题栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "#ffffff"
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 15
                anchors.rightMargin: 15
                spacing: 10

                Text {
                    text: "\u5c01\u9762"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.preferredWidth: root.colCoverWidth
                }

                Text {
                    text: "文件名"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.preferredWidth: root.colNameWidth
                }

                Text {
                    text: "大小"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.preferredWidth: root.colSizeWidth
                }

                Text {
                    text: "下载时间"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.preferredWidth: root.colTimeWidth
                }

                Text {
                    text: "操作"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.preferredWidth: root.colActionWidth
                }
            }
        }

        // 音乐列表
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8
            clip: true

            model: typeof downloadTaskModel !== 'undefined' ? downloadTaskModel : null

            delegate: Rectangle {
                property bool currentTrack: root.isSameTrack(root.currentPlayingPath, model.savePath || "")
                property bool playbackActive: currentTrack && root.isPlaying
                property bool rowHovered: rowHoverHandler.hovered
                                           || coverAction.interactionActive
                                           || actionStrip.interactionActive
                width: listView.width
                height: 60
                color: currentTrack
                       ? "#FDECEC"
                       : (rowHovered ? "#F8FAFF" : "#ffffff")
                radius: 4
                border.width: currentTrack ? 1 : 0
                border.color: currentTrack ? "#EC4141" : "transparent"

                HoverHandler {
                    id: rowHoverHandler
                }

                Rectangle {
                    visible: currentTrack
                    width: 3
                    radius: 2
                    color: "#EC4141"
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.margins: 10
                }

                MouseArea {
                    id: itemArea
                    anchors.fill: parent
                    propagateComposedEvents: true
                    onDoubleClicked: {
                        root.playMusic(model.savePath || "")
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    // 专辑图片
                    SongCoverAction {
                        id: coverAction
                        Layout.preferredWidth: root.colCoverWidth
                        Layout.preferredHeight: root.colCoverWidth
                        rowHovered: rowHovered
                        isCurrentTrack: currentTrack
                        isPlaying: playbackActive
                        coverSource: model.coverUrl || ""
                        fallbackSource: "qrc:/qml/assets/ai/icons/default-music-cover.svg"

                        onPlayRequested: {
                            if (currentTrack) {
                                root.songActionRequested("toggle_current_playback", root.buildSongPayload(model))
                                return
                            }
                            root.playMusic(model.savePath || "")
                        }

                        onPauseRequested: {
                            root.songActionRequested("toggle_current_playback", root.buildSongPayload(model))
                        }
                    }

                    // 文件名
                    Text {
                        Layout.preferredWidth: root.colNameWidth
                        text: model.filename || ""
                        font.pixelSize: 14
                        font.bold: currentTrack
                        color: currentTrack ? "#4A90E2" : "#333333"
                        elide: Text.ElideMiddle
                    }

                    // 文件大小
                    Text {
                        Layout.preferredWidth: root.colSizeWidth
                        text: formatSize(model.totalSize || 0)
                        font.pixelSize: 13
                        color: "#666666"
                    }

                    // 下载时间（使用创建时间）
                    Text {
                        Layout.preferredWidth: root.colTimeWidth
                        text: Qt.formatDateTime(new Date(), "yyyy-MM-dd hh:mm")
                        font.pixelSize: 13
                        color: currentTrack ? "#EC4141" : "#666666"
                    }

                    // 操作按钮
                    SongActionStrip {
                        id: actionStrip
                        Layout.preferredWidth: root.colActionWidth
                        z: 1
                        opacity: rowHovered ? 1.0 : 0.0
                        enabled: rowHovered
                        availablePlaylists: root.availablePlaylists
                        songData: root.buildSongPayload(model)
                        favoriteActive: root.isFavoritePath(model.savePath || "")
                        showDownloadButton: false
                        showRemoveAction: true
                        removeActionText: "删除"

                        onActionRequested: function(action, payload) {
                            if (action === "play") {
                                root.playMusic(model.savePath || "")
                                return
                            }
                            if (action === "add_favorite") {
                                root.addToFavorite(
                                    model.savePath || "",
                                    model.songName || "",
                                    model.artist || "",
                                    model.duration || ""
                                )
                                return
                            }
                            if (action === "remove_or_delete") {
                                root.deleteMusic(model.savePath || "")
                                if (typeof downloadTaskModel !== 'undefined') {
                                    downloadTaskModel.removeTask(model.taskId || "")
                                }
                                return
                            }
                            root.songActionRequested(action, payload)
                        }
                    }
                }
            }

            // 空状态提示
            Text {
                anchors.centerIn: parent
                text: "暂无已下载的音乐"
                font.pixelSize: 16
                color: "#999999"
                visible: listView.count === 0
            }
        }
    }

    // 格式化文件大小
    function formatSize(bytes) {
        if (bytes < 1024) return bytes + " B"
        if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(2) + " KB"
        if (bytes < 1024 * 1024 * 1024) return (bytes / (1024 * 1024)).toFixed(2) + " MB"
        return (bytes / (1024 * 1024 * 1024)).toFixed(2) + " GB"
    }
}
