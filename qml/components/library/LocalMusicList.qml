import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3

// 本地音乐列表
Rectangle {
    id: root
    color: "#f5f5f5"
    property string listIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/list/"
    property string playerIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/player/"
    property int colCoverWidth: 44
    property int colDurationWidth: width >= 1280 ? 96 : (width >= 960 ? 84 : 72)
    property int colArtistWidth: width >= 1280 ? 180 : (width >= 960 ? 140 : 110)
    property int colActionWidth: width >= 1280 ? 168 : 152
    property int rowContentWidth: Math.max(320, width - 70)
    property var availablePlaylists: []
    property var favoritePaths: []
    property int colTitleWidth: Math.max(150,
                                         rowContentWidth - colCoverWidth - colDurationWidth
                                         - colArtistWidth - colActionWidth - 40)

    signal playMusic(string filename)
    signal deleteMusic(string filename)
    signal addMusicClicked()
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

    function buildSongPayload(item) {
        return {
            path: item.filePath || "",
            playPath: item.filePath || "",
            title: item.fileName || "",
            artist: item.artist || "未知艺术家",
            duration: item.duration || "0:00",
            cover: item.coverUrl || "",
            isLocal: true,
            isFavorite: isFavoritePath(item.filePath || ""),
            sourceType: "local"
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        // 标题栏和添加按钮
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                text: "本地音乐"
                font.pixelSize: 20
                font.bold: true
                color: "#333333"
                Layout.fillWidth: true
            }

            Button {
                text: "+ 添加音乐"
                font.pixelSize: 14
                
                background: Rectangle {
                    color: parent.hovered ? "#66b1ff" : "#409EFF"
                    radius: 4
                }
                
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 14
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    if (typeof localMusicModel !== 'undefined') {
                        localMusicModel.addMusic("")
                    }
                }
            }
        }

        // 表头
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
                    text: "封面"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.preferredWidth: root.colCoverWidth
                }

                Text {
                    text: "歌曲名"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.preferredWidth: root.colTitleWidth
                }

                Text {
                    text: "时长"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.preferredWidth: root.colDurationWidth
                }

                Text {
                    text: "艺术家"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.preferredWidth: root.colArtistWidth
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

            model: typeof localMusicModel !== 'undefined' ? localMusicModel : null

            delegate: Rectangle {
                property bool rowHovered: rowHoverHandler.hovered || actionStrip.interactionActive
                width: listView.width
                height: 60
                color: rowHovered ? "#f0f0f0" : "#ffffff"
                radius: 4

                HoverHandler {
                    id: rowHoverHandler
                }

                MouseArea {
                    id: itemArea
                    anchors.fill: parent
                    propagateComposedEvents: true
                    onDoubleClicked: {
                        root.playMusic(model.filePath || "")
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10


                    // 封面
                    Rectangle {
                        Layout.preferredWidth: root.colCoverWidth
                        Layout.preferredHeight: 44
                        radius: 4
                        color: "#E0E0E0"
                        
                        Image {
                            anchors.fill: parent
                            anchors.margins: 2
                            source: model.coverUrl || "qrc:/new/prefix1/icon/Music.png"
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            cache: true
                            sourceSize.width: 44
                            sourceSize.height: 44
                        }
                    }

                    // 歌曲名
                    Text {
                        Layout.preferredWidth: root.colTitleWidth
                        text: model.fileName || ""
                        font.pixelSize: 14
                        font.bold: model.isPlaying
                        color: model.isPlaying ? "#4A90E2" : "#333333"
                        elide: Text.ElideMiddle
                    }

                    // 时长
                    Text {
                        Layout.preferredWidth: root.colDurationWidth
                        text: model.duration || "0:00"
                        font.pixelSize: 13
                        color: "#666666"
                    }

                    // 艺术家
                    Text {
                        Layout.preferredWidth: root.colArtistWidth
                        text: model.artist || "未知艺术家"
                        font.pixelSize: 13
                        color: "#666666"
                        elide: Text.ElideRight
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
                        favoriteActive: root.isFavoritePath(model.filePath || "")
                        showDownloadButton: false
                        showRemoveAction: true
                        removeActionText: "删除"

                        onActionRequested: function(action, payload) {
                            if (action === "play") {
                                root.playMusic(model.filePath || "")
                                return
                            }
                            if (action === "add_favorite") {
                                root.addToFavorite(
                                    model.filePath || "",
                                    model.fileName || "",
                                    model.artist || "",
                                    model.duration || ""
                                )
                                return
                            }
                            if (action === "remove_or_delete") {
                                root.deleteMusic(model.filePath || "")
                                if (typeof localMusicModel !== 'undefined') {
                                    localMusicModel.removeMusic(model.filePath || "")
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
                text: "暂无本地音乐\n点击右上角「添加音乐」按钮导入"
                font.pixelSize: 16
                color: "#999999"
                horizontalAlignment: Text.AlignHCenter
                visible: listView.count === 0
            }
        }
    }
}
