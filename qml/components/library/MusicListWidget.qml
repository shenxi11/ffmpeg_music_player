import QtQuick 2.14
import QtQuick.Controls 2.14
import "../../theme/Theme.js" as Theme

Rectangle {
    id: root
    color: "transparent"
    radius: 14

    // 对外暴露属性
    property bool isNetMusic: false
    property int currentPlayingIndex: -1
    property int sideMargin: width >= 1200 ? 20 : 14
    property int innerMargin: width >= 1200 ? 16 : 12
    property int colGap: width >= 1200 ? 12 : 8
    property int colIndexWidth: 36
    property int colCoverWidth: width >= 1200 ? 46 : 42
    property int colDurationWidth: width >= 1280 ? 88 : (width >= 960 ? 74 : 64)
    property int colSizeWidth: width >= 1280 ? 96 : (width >= 960 ? 82 : 70)
    property int colActionWidth: width >= 1200 ? 156 : 144
    property string listIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/list/"
    property string playerIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/player/"
    property int contentWidth: Math.max(360, width - sideMargin * 2 - innerMargin * 2)
    property int colTitleWidth: Math.max(140,
                                         contentWidth - colIndexWidth - colCoverWidth
                                         - colDurationWidth - colSizeWidth - colActionWidth
                                         - colGap * 5)

    signal playRequested(string filePath)
    signal removeRequested(string filePath)
    signal downloadRequested(string filePath)
    signal addButtonClicked()

    Rectangle {
        id: toolBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.sideMargin
        anchors.rightMargin: root.sideMargin
        anchors.top: parent.top
        height: 54
        radius: 12
        color: Theme.glassLight
        border.width: 1
        border.color: Theme.glassBorder

        Rectangle {
            id: addButton
            width: 104
            height: 34
            radius: 17
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            color: addArea.containsMouse ? Theme.accent : "transparent"
            border.width: 1
            border.color: Theme.accent

            Text {
                anchors.centerIn: parent
                text: "\u6dfb\u52a0\u6b4c\u66f2"
                color: addArea.containsMouse ? "#FFFFFF" : Theme.accent
                font.pixelSize: 13
                font.weight: Font.DemiBold
            }

            MouseArea {
                id: addArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.addButtonClicked()
            }
        }
    }

    Rectangle {
        id: headerBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.sideMargin
        anchors.rightMargin: root.sideMargin
        anchors.top: toolBar.bottom
        anchors.topMargin: 10
        height: 40
        radius: 10
        color: Theme.glassLight
        border.width: 1
        border.color: Theme.glassBorder

        Row {
            anchors.fill: parent
            anchors.leftMargin: root.innerMargin
            anchors.rightMargin: root.innerMargin
            spacing: root.colGap

            Text {
                width: root.colIndexWidth
                anchors.verticalCenter: parent.verticalCenter
                text: "#"
                font.pixelSize: 12
                color: Theme.textSecondary
            }
            Text {
                width: root.colCoverWidth
                anchors.verticalCenter: parent.verticalCenter
                text: "\u5c01\u9762"
                font.pixelSize: 12
                color: Theme.textSecondary
            }
            Text {
                width: root.colTitleWidth
                anchors.verticalCenter: parent.verticalCenter
                text: "\u6807\u9898"
                font.pixelSize: 12
                color: Theme.textSecondary
            }
            Text {
                width: root.colDurationWidth
                anchors.verticalCenter: parent.verticalCenter
                text: "\u65f6\u957f"
                font.pixelSize: 12
                color: Theme.textSecondary
            }
            Text {
                width: root.colSizeWidth
                anchors.verticalCenter: parent.verticalCenter
                text: "\u5927\u5c0f"
                font.pixelSize: 12
                color: Theme.textSecondary
            }
            Text {
                width: root.colActionWidth
                anchors.verticalCenter: parent.verticalCenter
                text: "\u64cd\u4f5c"
                font.pixelSize: 12
                color: Theme.textSecondary
            }
        }
    }

    ListView {
        id: listView
        anchors.top: headerBar.bottom
        anchors.topMargin: 8
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: root.sideMargin
        anchors.rightMargin: root.sideMargin
        clip: true
        spacing: 6
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.VerticalFlick
        cacheBuffer: 520
        model: musicListModel
        delegate: musicItemDelegate

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            width: 8
            interactive: true
        }

        Label {
            anchors.centerIn: parent
            text: "\u6682\u65e0\u97f3\u4e50"
            visible: listView.count === 0
            font.pixelSize: 16
            color: Theme.textSecondary
        }
    }

    ListModel {
        id: musicListModel
    }

    Component {
        id: musicItemDelegate

        Rectangle {
            id: itemRoot
            width: listView.width
            height: 62
            radius: 10
            property bool containsMouse: false
            color: model.isPlaying
                   ? "#FDECEC"
                   : (itemRoot.containsMouse ? "#F8FAFF" : (index % 2 === 0 ? Theme.bgCard : "#FCFCFD"))
            border.width: model.isPlaying ? 1 : 0
            border.color: model.isPlaying ? Theme.accent : "transparent"

            Rectangle {
                visible: model.isPlaying
                width: 3
                radius: 2
                color: Theme.accent
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.margins: 10
            }

            Row {
                anchors.fill: parent
                anchors.leftMargin: root.innerMargin
                anchors.rightMargin: root.innerMargin
                spacing: root.colGap

                Text {
                    width: root.colIndexWidth
                    anchors.verticalCenter: parent.verticalCenter
                    text: (index + 1).toString()
                    font.pixelSize: 13
                    color: Theme.textSecondary
                }

                Rectangle {
                    width: root.colCoverWidth
                    height: root.colCoverWidth
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 6
                    color: "#E9ECF5"
                    border.width: 1
                    border.color: "#D9DFEA"

                    Image {
                        anchors.fill: parent
                        anchors.margins: 2
                        source: model.cover !== "" ? model.cover : "qrc:/qml/assets/ai/icons/default-music-cover.svg"
                        fillMode: Image.PreserveAspectCrop
                        smooth: true
                        cache: true
                    }
                }

                Column {
                    width: root.colTitleWidth
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4

                    Text {
                        width: parent.width
                        text: model.songName
                        elide: Text.ElideRight
                        font.pixelSize: 14
                        font.weight: model.isPlaying ? Font.DemiBold : Font.Normal
                        color: model.isPlaying ? Theme.accent : Theme.textPrimary
                    }

                    Text {
                        width: parent.width
                        text: model.artist || "\u672a\u77e5\u827a\u672f\u5bb6"
                        elide: Text.ElideRight
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }
                }

                Text {
                    width: root.colDurationWidth
                    anchors.verticalCenter: parent.verticalCenter
                    text: model.duration || "0:00"
                    font.pixelSize: 12
                    color: Theme.textSecondary
                }

                Text {
                    width: root.colSizeWidth
                    anchors.verticalCenter: parent.verticalCenter
                    text: model.fileSize || "-"
                    font.pixelSize: 12
                    color: Theme.textSecondary
                }

                Row {
                    width: root.colActionWidth
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    Rectangle {
                        id: playBtn
                        width: 32
                        height: 32
                        radius: 16
                        color: model.isPlaying ? Theme.accent : (playBtnArea.containsMouse ? "#ECEFF8" : "transparent")
                        border.width: 1
                        border.color: model.isPlaying ? Theme.accent : "#D6DCE8"

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
                            onClicked: root.playRequested(model.filePath)
                        }
                    }

                    Rectangle {
                        width: 32
                        height: 32
                        radius: 16
                        visible: !root.isNetMusic
                        opacity: itemRoot.containsMouse ? 1 : 0.02
                        color: removeArea.containsMouse ? Theme.accentSoft : "transparent"
                        border.width: 1
                        border.color: removeArea.containsMouse ? Theme.accent : "#D6DCE8"

                        Behavior on opacity {
                            NumberAnimation { duration: 120 }
                        }

                        Image {
                            anchors.centerIn: parent
                            width: 18
                            height: 18
                            source: removeArea.containsMouse
                                    ? root.listIconPrefix + "list_icon_delete_hover.svg"
                                    : root.listIconPrefix + "list_icon_delete_default.svg"
                            fillMode: Image.PreserveAspectFit
                        }

                        MouseArea {
                            id: removeArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.removeRequested(model.filePath)
                        }
                    }

                    Rectangle {
                        width: 32
                        height: 32
                        radius: 16
                        visible: root.isNetMusic
                        opacity: itemRoot.containsMouse ? 1 : 0.02
                        color: downloadArea.containsMouse ? Theme.accentSoft : "transparent"
                        border.width: 1
                        border.color: downloadArea.containsMouse ? Theme.accent : "#D6DCE8"

                        Behavior on opacity {
                            NumberAnimation { duration: 120 }
                        }

                        Image {
                            anchors.centerIn: parent
                            width: 18
                            height: 18
                            source: downloadArea.containsMouse
                                    ? root.listIconPrefix + "list_icon_download_hover.svg"
                                    : root.listIconPrefix + "list_icon_download_default.svg"
                            fillMode: Image.PreserveAspectFit
                        }

                        MouseArea {
                            id: downloadArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.downloadRequested(model.filePath)
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
            }
        }
    }

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
                if (root.currentPlayingIndex === i) {
                    root.currentPlayingIndex = -1
                } else if (root.currentPlayingIndex > i) {
                    root.currentPlayingIndex -= 1
                }
                return
            }
        }
    }

    function clearAll() {
        musicListModel.clear()
        root.currentPlayingIndex = -1
    }

    function getCount() {
        return musicListModel.count
    }

    function getAllFilePaths() {
        var paths = []
        for (var i = 0; i < musicListModel.count; i++) {
            paths.push(musicListModel.get(i).filePath)
        }
        return paths
    }

    function getIndexByFilePath(filePath) {
        for (var i = 0; i < musicListModel.count; i++) {
            if (musicListModel.get(i).filePath === filePath) {
                return i
            }
        }
        return -1
    }

    function normalizePath(path) {
        if (!path) {
            return ""
        }
        if (path.indexOf("http") !== 0) {
            return path
        }
        var uploadsIndex = path.indexOf("/uploads/")
        if (uploadsIndex >= 0) {
            return path.substring(uploadsIndex + 9)
        }
        return path
    }

    function normalizeArtistText(artist) {
        if (!artist) {
            return ""
        }
        var trimmed = artist.toString().trim()
        if (trimmed.length === 0) {
            return ""
        }
        var lower = trimmed.toLowerCase()
        if (trimmed === "未知艺术家" || trimmed === "未知歌手"
                || lower === "unknown artist" || lower === "unknown"
                || lower === "<unknown>") {
            return ""
        }
        return trimmed
    }

    function clearPlayingState() {
        if (root.currentPlayingIndex >= 0 && root.currentPlayingIndex < musicListModel.count) {
            musicListModel.setProperty(root.currentPlayingIndex, "isPlaying", false)
        }
        root.currentPlayingIndex = -1
    }

    function setPlayingState(filePath, playing) {
        if (filePath === "") {
            clearPlayingState()
            return
        }

        var pathToMatch = normalizePath(filePath)
        for (var i = 0; i < musicListModel.count; i++) {
            if (musicListModel.get(i).filePath !== pathToMatch) {
                continue
            }

            if (playing) {
                if (root.currentPlayingIndex >= 0 && root.currentPlayingIndex !== i) {
                    musicListModel.setProperty(root.currentPlayingIndex, "isPlaying", false)
                }
                musicListModel.setProperty(i, "isPlaying", true)
                root.currentPlayingIndex = i
            } else {
                musicListModel.setProperty(i, "isPlaying", false)
                if (root.currentPlayingIndex === i) {
                    root.currentPlayingIndex = -1
                }
            }
            return
        }

        if (playing) {
            clearPlayingState()
        }
    }

    function playNext(songName) {
        if (musicListModel.count <= 1) {
            return
        }
        for (var i = 0; i < musicListModel.count; i++) {
            if (musicListModel.get(i).songName !== songName) {
                continue
            }
            var nextIndex = (i + 1) % musicListModel.count
            musicListModel.setProperty(i, "isPlaying", false)
            musicListModel.setProperty(nextIndex, "isPlaying", true)
            root.currentPlayingIndex = nextIndex
            root.playRequested(musicListModel.get(nextIndex).filePath)
            return
        }
    }

    function playLast(songName) {
        if (musicListModel.count <= 1) {
            return
        }
        for (var i = 0; i < musicListModel.count; i++) {
            if (musicListModel.get(i).songName !== songName) {
                continue
            }
            var lastIndex = i === 0 ? (musicListModel.count - 1) : (i - 1)
            musicListModel.setProperty(i, "isPlaying", false)
            musicListModel.setProperty(lastIndex, "isPlaying", true)
            root.currentPlayingIndex = lastIndex
            root.playRequested(musicListModel.get(lastIndex).filePath)
            return
        }
    }

    function updateSongMetadata(filePath, coverUrl, duration, artist) {
        for (var i = 0; i < musicListModel.count; i++) {
            if (musicListModel.get(i).filePath !== filePath) {
                continue
            }
            if (coverUrl && coverUrl.length > 0) {
                musicListModel.setProperty(i, "cover", coverUrl)
            }
            if (duration && duration.length > 0) {
                musicListModel.setProperty(i, "duration", duration)
            }
            var normalizedArtist = normalizeArtistText(artist)
            if (normalizedArtist.length > 0) {
                musicListModel.setProperty(i, "artist", normalizedArtist)
            }
            return
        }
    }
}
