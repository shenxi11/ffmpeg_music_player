import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../theme/Theme.js" as Theme

Rectangle {
    id: root
    color: Theme.bgBase

    property bool isLoggedIn: false
    property string userAccount: ""
    property string currentPlayingPath: ""
    property bool isPlaying: false
    property var favoritePaths: []
    property int selectedPlaylistId: -1
    property var currentPlaylistDetail: ({})
    property int sideMargin: width >= 1200 ? 20 : 14
    property int leftPanelWidth: width >= 1360 ? 280 : (width >= 1100 ? 250 : 220)
    property bool ownedGroupExpanded: true
    property bool subscribedGroupExpanded: true
    property string listIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/list/"
    property string playerIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/player/"
    property string defaultCover: "qrc:/new/prefix1/icon/Music.png"

    signal loginRequested()
    signal refreshRequested()
    signal openPlaylistRequested(var playlistId)
    signal createPlaylistRequested(string name, string description)
    signal updatePlaylistRequested(var playlistId, string name, string description)
    signal deletePlaylistRequested(var playlistId)
    signal removePlaylistItemsRequested(var playlistId, var musicPaths)
    signal reorderPlaylistItemsRequested(var playlistId, var orderedItems)
    signal addCurrentSongRequested(var playlistId)
    signal playMusicWithMetadata(string filePath, string title, string artist, string cover)
    signal songActionRequested(string action, var song)

    ListModel {
        id: playlistModel
    }

    ListModel {
        id: ownedPlaylistModel
    }

    ListModel {
        id: subscribedPlaylistModel
    }

    ListModel {
        id: songModel
    }

    function normalizeText(value, fallbackText) {
        if (value === undefined || value === null) return fallbackText
        var text = String(value).trim()
        var lower = text.toLowerCase()
        if (lower === "null" || lower === "undefined" || lower === "none" || lower === "(null)")
            return fallbackText
        return text.length > 0 ? text : fallbackText
    }

    function normalizeCoverSource(value) {
        var text = normalizeText(value, "")
        if (text.length === 0) return root.defaultCover
        text = text.replace("/uploads/uploads/", "/uploads/")
        if (text.toLowerCase().indexOf("uploads/uploads/") === 0) {
            text = "uploads/" + text.substring("uploads/uploads/".length)
        }
        return text
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

    function normalizePath(path) {
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

    function isSameTrack(pathA, pathB) {
        return normalizePath(pathA) === normalizePath(pathB)
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

    function formatTrackCount(count) {
        var c = Number(count)
        if (isNaN(c) || c < 0) c = 0
        return c + " 首"
    }

    function normalizeOwnership(value) {
        var text = normalizeText(value, "").toLowerCase()
        return text === "subscribed" ? "subscribed" : "owned"
    }

    function buildSongPayload(item) {
        return {
            path: item.path || "",
            playPath: item.path || "",
            title: item.title || "未知歌曲",
            artist: item.artist || "未知艺术家",
            duration: item.duration || "0:00",
            cover: item.cover_art_url || "",
            isLocal: !!item.is_local,
            isFavorite: isFavoritePath(item.path || ""),
            sourceType: "playlist",
            playlistId: Number(root.selectedPlaylistId)
        }
    }

    function buildPlaylistEntry(item) {
        return {
            id: Number(item.id || 0),
            name: normalizeText(item.name, "未命名歌单"),
            description: normalizeText(item.description, ""),
            cover_url: normalizeText(item.cover_url, ""),
            track_count: Number(item.track_count || 0),
            total_duration: normalizeText(item.total_duration, "0:00"),
            updated_at: normalizeText(item.updated_at, ""),
            ownership: normalizeOwnership(item.ownership)
        }
    }

    function selectPlaylist(playlistId, requestDetail) {
        var idValue = Number(playlistId)
        if (isNaN(idValue) || idValue <= 0) {
            return
        }
        selectedPlaylistId = idValue
        if (requestDetail) {
            openPlaylistRequested(idValue)
        }
    }

    function setPlayingState(filePath, playing) {
        currentPlayingPath = filePath || ""
        isPlaying = playing
    }

    function loadPlaylists(playlists, page, pageSize, total) {
        playlistModel.clear()
        ownedPlaylistModel.clear()
        subscribedPlaylistModel.clear()
        for (var i = 0; i < playlists.length; ++i) {
            var entry = buildPlaylistEntry(playlists[i])
            playlistModel.append(entry)
            if (entry.ownership === "subscribed") {
                subscribedPlaylistModel.append(entry)
            } else {
                ownedPlaylistModel.append(entry)
            }
        }

        if (playlistModel.count === 0) {
            selectedPlaylistId = -1
            songModel.clear()
            currentPlaylistDetail = ({})
            return
        }

        var existed = false
        for (var idx = 0; idx < playlistModel.count; ++idx) {
            if (Number(playlistModel.get(idx).id) === Number(selectedPlaylistId)) {
                existed = true
                break
            }
        }
        if (!existed) {
            selectedPlaylistId = Number(playlistModel.get(0).id)
            openPlaylistRequested(selectedPlaylistId)
        }
    }

    function loadPlaylistDetail(detail) {
        if (!detail || detail.id === undefined || detail.id === null) {
            return
        }

        currentPlaylistDetail = detail
        selectedPlaylistId = Number(detail.id)

        songModel.clear()
        var items = detail.items || []
        for (var i = 0; i < items.length; ++i) {
            var item = items[i]
            var path = normalizeText(item.path, normalizeText(item.music_path, ""))
            var title = normalizeText(item.title, normalizeText(item.music_title, baseNameFromPath(path)))
            songModel.append({
                                 id: Number(item.id || 0),
                                 position: Number(item.position || (i + 1)),
                                 path: path,
                                 title: normalizeText(title, "未知歌曲"),
                                 artist: normalizeText(item.artist, "未知艺术家"),
                                 album: normalizeText(item.album, ""),
                                 duration: normalizeText(item.duration, "0:00"),
                                 duration_sec: Number(item.duration_sec || 0),
                                 is_local: !!item.is_local,
                                 added_at: normalizeText(item.added_at, ""),
                                 cover_art_url: normalizeText(item.cover_art_url, "")
                             })
        }
    }

    function clearData() {
        playlistModel.clear()
        ownedPlaylistModel.clear()
        subscribedPlaylistModel.clear()
        songModel.clear()
        selectedPlaylistId = -1
        currentPlaylistDetail = ({})
        currentPlayingPath = ""
        isPlaying = false
    }

    function openCreatePlaylistDialog() {
        createDialog.open()
    }

    function submitReorder() {
        if (selectedPlaylistId <= 0 || songModel.count <= 0) return
        var ordered = []
        for (var i = 0; i < songModel.count; ++i) {
            var row = songModel.get(i)
            ordered.push({
                             "music_path": row.path || "",
                             "position": i + 1
                         })
        }
        root.reorderPlaylistItemsRequested(selectedPlaylistId, ordered)
    }

    function moveSongByStep(rowIndex, step) {
        var fromIndex = Number(rowIndex)
        var delta = Number(step)
        if (isNaN(fromIndex) || isNaN(delta) || delta === 0) return

        var toIndex = fromIndex + delta
        if (fromIndex < 0 || fromIndex >= songModel.count) return
        if (toIndex < 0 || toIndex >= songModel.count) return

        var moving = songModel.get(fromIndex)
        songModel.remove(fromIndex)
        songModel.insert(toIndex, moving)
        for (var i = 0; i < songModel.count; ++i) {
            songModel.setProperty(i, "position", i + 1)
        }
        submitReorder()
    }

    Popup {
        id: createDialog
        modal: true
        focus: true
        width: Math.min(420, root.width - 40)
        height: 250
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            radius: 14
            color: "#FFFFFF"
            border.width: 1
            border.color: Theme.glassBorder
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Text {
                text: "新建歌单"
                font.pixelSize: 18
                font.weight: Font.DemiBold
                color: Theme.textPrimary
            }

            TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: "请输入歌单名称"
                font.pixelSize: 14
                color: Theme.textPrimary
                selectByMouse: true
                background: Rectangle {
                    radius: 8
                    border.width: 1
                    border.color: nameField.activeFocus ? Theme.accent : Theme.glassBorder
                    color: "#FFFFFF"
                }
            }

            TextArea {
                id: descField
                Layout.fillWidth: true
                Layout.fillHeight: true
                placeholderText: "请输入歌单简介（可选）"
                wrapMode: TextEdit.Wrap
                font.pixelSize: 13
                color: Theme.textPrimary
                selectByMouse: true
                background: Rectangle {
                    radius: 8
                    border.width: 1
                    border.color: descField.activeFocus ? Theme.accent : Theme.glassBorder
                    color: "#FFFFFF"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Item { Layout.fillWidth: true }

                Rectangle {
                    width: 88
                    height: 34
                    radius: 17
                    border.width: 1
                    border.color: Theme.glassBorder
                    color: cancelArea.containsMouse ? Theme.glassHover : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "取消"
                        font.pixelSize: 13
                        color: Theme.textPrimary
                    }

                    MouseArea {
                        id: cancelArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: createDialog.close()
                    }
                }

                Rectangle {
                    width: 88
                    height: 34
                    radius: 17
                    color: confirmArea.containsMouse ? "#ff5757" : Theme.accent

                    Text {
                        anchors.centerIn: parent
                        text: "创建"
                        font.pixelSize: 13
                        color: "#FFFFFF"
                    }

                    MouseArea {
                        id: confirmArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            var nameText = nameField.text.trim()
                            if (nameText.length === 0) {
                                return
                            }
                            root.createPlaylistRequested(nameText, descField.text.trim())
                            createDialog.close()
                        }
                    }
                }
            }
        }

        onOpened: {
            nameField.text = ""
            descField.text = ""
            nameField.forceActiveFocus()
        }
    }

    Popup {
        id: editDialog
        modal: true
        focus: true
        width: Math.min(420, root.width - 40)
        height: 250
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            radius: 14
            color: "#FFFFFF"
            border.width: 1
            border.color: Theme.glassBorder
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Text {
                text: "编辑歌单"
                font.pixelSize: 18
                font.weight: Font.DemiBold
                color: Theme.textPrimary
            }

            TextField {
                id: editNameField
                Layout.fillWidth: true
                placeholderText: "请输入歌单名称"
                font.pixelSize: 14
                color: Theme.textPrimary
                selectByMouse: true
                background: Rectangle {
                    radius: 8
                    border.width: 1
                    border.color: editNameField.activeFocus ? Theme.accent : Theme.glassBorder
                    color: "#FFFFFF"
                }
            }

            TextArea {
                id: editDescField
                Layout.fillWidth: true
                Layout.fillHeight: true
                placeholderText: "请输入歌单简介（可选）"
                wrapMode: TextEdit.Wrap
                font.pixelSize: 13
                color: Theme.textPrimary
                selectByMouse: true
                background: Rectangle {
                    radius: 8
                    border.width: 1
                    border.color: editDescField.activeFocus ? Theme.accent : Theme.glassBorder
                    color: "#FFFFFF"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Item { Layout.fillWidth: true }

                Rectangle {
                    width: 88
                    height: 34
                    radius: 17
                    border.width: 1
                    border.color: Theme.glassBorder
                    color: editCancelArea.containsMouse ? Theme.glassHover : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "取消"
                        font.pixelSize: 13
                        color: Theme.textPrimary
                    }

                    MouseArea {
                        id: editCancelArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: editDialog.close()
                    }
                }

                Rectangle {
                    width: 88
                    height: 34
                    radius: 17
                    color: editConfirmArea.containsMouse ? "#ff5757" : Theme.accent

                    Text {
                        anchors.centerIn: parent
                        text: "保存"
                        font.pixelSize: 13
                        color: "#FFFFFF"
                    }

                    MouseArea {
                        id: editConfirmArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            var nameText = editNameField.text.trim()
                            if (nameText.length === 0 || root.selectedPlaylistId <= 0) {
                                return
                            }
                            root.updatePlaylistRequested(root.selectedPlaylistId,
                                                        nameText,
                                                        editDescField.text.trim())
                            editDialog.close()
                        }
                    }
                }
            }
        }
    }

    Component {
        id: playlistCardDelegate

        Rectangle {
            width: ListView.view ? ListView.view.width : 0
            height: 54
            radius: 12
            property bool selected: Number(model.id) === Number(root.selectedPlaylistId)
            property bool hover: playlistArea.containsMouse
            color: selected ? "#E5E7EB" : (hover ? "#F1F3F5" : "transparent")
            border.width: 0

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 8

                Rectangle {
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    radius: 8
                    color: "#ECEFF4"
                    border.width: 1
                    border.color: "#DCE3EE"

                    Image {
                        anchors.fill: parent
                        anchors.margins: 1
                        source: normalizeCoverSource(model.cover_url)
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: true
                        onStatusChanged: {
                            if (status === Image.Error && source !== root.defaultCover) {
                                source = root.defaultCover
                            }
                        }
                    }
                }

                Column {
                    Layout.fillWidth: true
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2

                    Text {
                        text: model.name || "未命名歌单"
                        width: parent.width
                        elide: Text.ElideRight
                        font.pixelSize: 13
                        font.weight: selected ? Font.DemiBold : Font.Normal
                        color: Theme.textPrimary
                    }

                    Text {
                        text: formatTrackCount(model.track_count)
                        width: parent.width
                        elide: Text.ElideRight
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }
                }
            }

            MouseArea {
                id: playlistArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.selectPlaylist(model.id, true)
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: !root.isLoggedIn
        color: "transparent"

        Column {
            anchors.centerIn: parent
            spacing: 14

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "登录后查看和管理我的歌单"
                font.pixelSize: 20
                font.weight: Font.DemiBold
                color: Theme.textPrimary
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "支持新建歌单、添加歌曲、删除歌曲"
                font.pixelSize: 13
                color: Theme.textSecondary
            }

            Rectangle {
                width: 140
                height: 38
                radius: 19
                anchors.horizontalCenter: parent.horizontalCenter
                color: loginArea.containsMouse ? Theme.accent : "transparent"
                border.width: 1
                border.color: Theme.accent

                Text {
                    anchors.centerIn: parent
                    text: "立即登录"
                    font.pixelSize: 14
                    color: loginArea.containsMouse ? "#FFFFFF" : Theme.accent
                }

                MouseArea {
                    id: loginArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.loginRequested()
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: sideMargin
        spacing: 10
        visible: root.isLoggedIn

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 172
            radius: 14
            color: Theme.glassLight
            border.width: 1
            border.color: Theme.glassBorder

            RowLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 14

                Rectangle {
                    Layout.preferredWidth: 130
                    Layout.preferredHeight: 130
                    radius: 10
                    color: "#ECEFF4"
                    border.width: 1
                    border.color: "#DCE3EE"

                    Image {
                        anchors.fill: parent
                        anchors.margins: 2
                        source: normalizeCoverSource(currentPlaylistDetail.cover_url)
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: true
                        onStatusChanged: {
                            if (status === Image.Error && source !== root.defaultCover) {
                                source = root.defaultCover
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 8

                    Text {
                        text: normalizeText(currentPlaylistDetail.name, "我的歌单")
                        font.pixelSize: 24
                        font.weight: Font.DemiBold
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Text {
                        text: normalizeText(currentPlaylistDetail.description, "添加歌曲，打造你的私域歌单")
                        font.pixelSize: 13
                        color: Theme.textSecondary
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        Text {
                            text: "歌曲 " + formatTrackCount(currentPlaylistDetail.track_count || songModel.count)
                            font.pixelSize: 12
                            color: Theme.textSecondary
                        }
                        Text {
                            text: "总时长 " + normalizeText(currentPlaylistDetail.total_duration, "0:00")
                            font.pixelSize: 12
                            color: Theme.textSecondary
                        }
                        Text {
                            text: "更新时间 " + normalizeText(currentPlaylistDetail.updated_at, "-")
                            font.pixelSize: 12
                            color: Theme.textSecondary
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Rectangle {
                            width: 90
                            height: 34
                            radius: 17
                            border.width: 1
                            border.color: Theme.glassBorder
                            color: editPlaylistArea.containsMouse ? Theme.glassHover : "transparent"
                            enabled: selectedPlaylistId > 0
                            opacity: enabled ? 1.0 : 0.45

                            Text {
                                anchors.centerIn: parent
                                text: "编辑信息"
                                font.pixelSize: 13
                                color: Theme.textPrimary
                            }

                            MouseArea {
                                id: editPlaylistArea
                                anchors.fill: parent
                                hoverEnabled: true
                                enabled: parent.enabled
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    editNameField.text = normalizeText(currentPlaylistDetail.name, "")
                                    editDescField.text = normalizeText(currentPlaylistDetail.description, "")
                                    editDialog.open()
                                }
                            }
                        }

                        Rectangle {
                            width: 90
                            height: 34
                            radius: 17
                            color: playAllArea.containsMouse ? "#d93636" : Theme.accent
                            enabled: songModel.count > 0
                            opacity: enabled ? 1.0 : 0.45

                            Text {
                                anchors.centerIn: parent
                                text: "播放全部"
                                font.pixelSize: 13
                                color: "#FFFFFF"
                            }

                            MouseArea {
                                id: playAllArea
                                anchors.fill: parent
                                hoverEnabled: true
                                enabled: parent.enabled
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (songModel.count <= 0) return
                                    var first = songModel.get(0)
                                    root.playMusicWithMetadata(
                                                first.path || "",
                                                first.title || "未知歌曲",
                                                first.artist || "未知艺术家",
                                                first.cover_art_url || "")
                                }
                            }
                        }

                        Rectangle {
                            width: 116
                            height: 34
                            radius: 17
                            border.width: 1
                            border.color: Theme.glassBorder
                            color: addCurrentArea.containsMouse ? Theme.glassHover : "transparent"
                            enabled: selectedPlaylistId > 0
                            opacity: enabled ? 1.0 : 0.45

                            Text {
                                anchors.centerIn: parent
                                text: "添加当前播放"
                                font.pixelSize: 13
                                color: Theme.textPrimary
                            }

                            MouseArea {
                                id: addCurrentArea
                                anchors.fill: parent
                                hoverEnabled: true
                                enabled: parent.enabled
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.addCurrentSongRequested(root.selectedPlaylistId)
                            }
                        }

                        Rectangle {
                            width: 90
                            height: 34
                            radius: 17
                            border.width: 1
                            border.color: Theme.glassBorder
                            color: refreshArea.containsMouse ? Theme.glassHover : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: "刷新"
                                font.pixelSize: 13
                                color: Theme.textPrimary
                            }

                            MouseArea {
                                id: refreshArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.refreshRequested()
                            }
                        }

                        Rectangle {
                            width: 90
                            height: 34
                            radius: 17
                            border.width: 1
                            border.color: "#F0B8B8"
                            color: deletePlaylistArea.containsMouse ? "#FDECEC" : "transparent"
                            enabled: selectedPlaylistId > 0
                            opacity: enabled ? 1.0 : 0.45

                            Text {
                                anchors.centerIn: parent
                                text: "删除歌单"
                                font.pixelSize: 13
                                color: "#D54A4A"
                            }

                            MouseArea {
                                id: deletePlaylistArea
                                anchors.fill: parent
                                hoverEnabled: true
                                enabled: parent.enabled
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.deletePlaylistRequested(root.selectedPlaylistId)
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            Rectangle {
                Layout.preferredWidth: root.leftPanelWidth
                Layout.fillHeight: true
                radius: 12
                color: Theme.glassLight
                border.width: 1
                border.color: Theme.glassBorder

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: "我的歌单"
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                            color: Theme.textPrimary
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            width: 28
                            height: 28
                            radius: 14
                            color: createArea.containsMouse ? "#EEF1F5" : "transparent"
                            border.width: 1
                            border.color: createArea.containsMouse ? "#D8DCE3" : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: "+"
                                font.pixelSize: 16
                                font.weight: Font.DemiBold
                                color: Theme.textPrimary
                            }

                            MouseArea {
                                id: createArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: createDialog.open()
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 10
                        color: "#FAFBFC"
                        border.width: 1
                        border.color: "#E7EAF0"

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 28
                                color: "transparent"

                                RowLayout {
                                    anchors.fill: parent

                                    Text {
                                        text: ownedGroupExpanded ? "▾" : "▸"
                                        font.pixelSize: 14
                                        color: Theme.textSecondary
                                    }

                                    Text {
                                        text: "自建歌单"
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                        color: Theme.textPrimary
                                        Layout.fillWidth: true
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.ownedGroupExpanded = !root.ownedGroupExpanded
                                }
                            }

                            ListView {
                                id: ownedPlaylistListView
                                Layout.fillWidth: true
                                Layout.preferredHeight: root.ownedGroupExpanded
                                                        ? Math.min(contentHeight, 280)
                                                        : 0
                                visible: root.ownedGroupExpanded
                                model: ownedPlaylistModel
                                spacing: 4
                                clip: true
                                interactive: contentHeight > height
                                delegate: playlistCardDelegate
                                ScrollBar.vertical: ScrollBar {
                                    policy: ScrollBar.AsNeeded
                                    width: 6
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                visible: root.ownedGroupExpanded && ownedPlaylistModel.count === 0
                                text: "暂无自建歌单"
                                font.pixelSize: 12
                                color: Theme.textSecondary
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: "#E7EAF0"
                                visible: root.ownedGroupExpanded || root.subscribedGroupExpanded
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 28
                                color: "transparent"

                                RowLayout {
                                    anchors.fill: parent

                                    Text {
                                        text: subscribedGroupExpanded ? "▾" : "▸"
                                        font.pixelSize: 14
                                        color: Theme.textSecondary
                                    }

                                    Text {
                                        text: "收藏歌单"
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                        color: Theme.textPrimary
                                        Layout.fillWidth: true
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.subscribedGroupExpanded = !root.subscribedGroupExpanded
                                }
                            }

                            ListView {
                                id: subscribedPlaylistListView
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredHeight: root.subscribedGroupExpanded
                                                        ? Math.min(contentHeight, 280)
                                                        : 0
                                visible: root.subscribedGroupExpanded
                                model: subscribedPlaylistModel
                                spacing: 4
                                clip: true
                                interactive: contentHeight > height
                                delegate: playlistCardDelegate
                                ScrollBar.vertical: ScrollBar {
                                    policy: ScrollBar.AsNeeded
                                    width: 6
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                visible: root.subscribedGroupExpanded && subscribedPlaylistModel.count === 0
                                text: "暂无收藏歌单"
                                font.pixelSize: 12
                                color: Theme.textSecondary
                            }

                            Item {
                                Layout.fillHeight: true
                            }

                            Label {
                                Layout.fillWidth: true
                                visible: playlistModel.count === 0
                                text: "暂无歌单"
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: 13
                                color: Theme.textSecondary
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 12
                color: Theme.glassLight
                border.width: 1
                border.color: Theme.glassBorder

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        radius: 8
                        color: Theme.glassHover

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8

                            Text {
                                width: 40
                                text: "#"
                                font.pixelSize: 12
                                color: Theme.textSecondary
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Text {
                                width: Math.max(140, parent.width - 40 - 90 - 150 - 160 - 8 * 3)
                                text: "音频信息"
                                font.pixelSize: 12
                                color: Theme.textSecondary
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Text {
                                width: 90
                                text: "时长"
                                font.pixelSize: 12
                                color: Theme.textSecondary
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Text {
                                width: 150
                                text: "添加时间"
                                font.pixelSize: 12
                                color: Theme.textSecondary
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Item {
                                width: 160
                                height: 1
                            }
                        }
                    }

                    ListView {
                        id: songListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: songModel
                        spacing: 6
                        clip: true

                        delegate: Rectangle {
                            id: songRow
                            width: songListView.width
                            height: 62
                            radius: 10
                            property bool rowHovered: rowHoverHandler.hovered || actionStrip.interactionActive
                            property bool currentTrack: root.isSameTrack(root.currentPlayingPath, model.path)
                            property bool showPauseIcon: currentTrack && root.isPlaying
                            color: currentTrack
                                   ? "#FDECEC"
                                   : (rowHovered ? "#F8FAFF" : (index % 2 === 0 ? Theme.bgCard : "#FCFCFD"))
                            border.width: currentTrack ? 1 : 0
                            border.color: currentTrack ? Theme.accent : "transparent"

                            HoverHandler {
                                id: rowHoverHandler
                            }

                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 8

                                Text {
                                    width: 40
                                    text: Number(model.position || index + 1).toString()
                                    font.pixelSize: 12
                                    color: Theme.textSecondary
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Row {
                                    width: Math.max(140, parent.width - 40 - 90 - 150 - 160 - 8 * 3)
                                    spacing: 8
                                    anchors.verticalCenter: parent.verticalCenter

                                    Rectangle {
                                        width: 40
                                        height: 40
                                        radius: 6
                                        color: "#E9ECF5"
                                        border.width: 1
                                        border.color: "#D9DFEA"

                                        Image {
                                            anchors.fill: parent
                                            anchors.margins: 1
                                            source: normalizeCoverSource(model.cover_art_url)
                                            fillMode: Image.PreserveAspectCrop
                                            asynchronous: true
                                            cache: true
                                            onStatusChanged: {
                                                if (status === Image.Error && source !== root.defaultCover) {
                                                    source = root.defaultCover
                                                }
                                            }
                                        }
                                    }

                                    Column {
                                        width: parent.width - 48
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: 2

                                        Text {
                                            width: parent.width
                                            text: model.title || "未知歌曲"
                                            font.pixelSize: 14
                                            font.weight: songRow.currentTrack ? Font.DemiBold : Font.Normal
                                            color: songRow.currentTrack ? Theme.accent : Theme.textPrimary
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            width: parent.width
                                            text: model.artist || "未知艺术家"
                                            font.pixelSize: 11
                                            color: Theme.textSecondary
                                            elide: Text.ElideRight
                                        }
                                    }
                                }

                                Text {
                                    width: 90
                                    text: model.duration || "0:00"
                                    font.pixelSize: 12
                                    color: Theme.textSecondary
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Text {
                                    width: 150
                                    text: model.added_at || ""
                                    font.pixelSize: 12
                                    color: Theme.textSecondary
                                    anchors.verticalCenter: parent.verticalCenter
                                    elide: Text.ElideRight
                                }

                                SongActionStrip {
                                    id: actionStrip
                                    width: 160
                                    anchors.verticalCenter: parent.verticalCenter
                                    z: 1
                                    opacity: songRow.rowHovered ? 1.0 : 0.0
                                    enabled: songRow.rowHovered
                                    availablePlaylists: ownedPlaylistModel
                                    songData: root.buildSongPayload(model)
                                    favoriteActive: root.isFavoritePath(model.path || "")
                                    showDownloadButton: !model.is_local
                                    showRemoveAction: true
                                    removeActionText: "从歌单移除"
                                    onActionRequested: function(action, payload) {
                                        if (action === "play") {
                                            root.playMusicWithMetadata(
                                                        model.path || "",
                                                        model.title || "未知歌曲",
                                                        model.artist || "未知艺术家",
                                                        model.cover_art_url || "")
                                            return
                                        }
                                        if (action === "remove_or_delete") {
                                            root.removePlaylistItemsRequested(
                                                        root.selectedPlaylistId,
                                                        [model.path])
                                            return
                                        }
                                        root.songActionRequested(action, payload)
                                    }
                                }
                            }

                            MouseArea {
                                id: rowArea
                                anchors.fill: parent
                                propagateComposedEvents: true
                                onPressed: mouse.accepted = false
                                onReleased: mouse.accepted = false
                                onClicked: mouse.accepted = false
                                onDoubleClicked: root.playMusicWithMetadata(
                                                     model.path || "",
                                                     model.title || "未知歌曲",
                                                     model.artist || "未知艺术家",
                                                     model.cover_art_url || "")
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: songListView.count === 0
                            text: root.selectedPlaylistId > 0 ? "歌单还没有歌曲" : "请选择左侧歌单"
                            font.pixelSize: 16
                            color: Theme.textSecondary
                        }

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                            width: 8
                        }
                    }
                }
            }
        }
    }
}
