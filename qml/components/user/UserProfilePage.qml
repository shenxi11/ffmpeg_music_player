import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15
import "../../theme/Theme.js" as Theme

Rectangle {
    id: root
    color: "#FCFDFE"
    radius: 0
    border.width: 0
    border.color: "transparent"

    property bool busy: false
    property bool usernameSaving: false
    property bool avatarUploading: false
    property bool sessionExpired: false

    property string username: "未登录"
    property string account: ""
    property string avatarSource: "qrc:/qml/assets/ai/icons/default-user-avatar.svg"
    property string committedAvatarSource: "qrc:/qml/assets/ai/icons/default-user-avatar.svg"
    property string createdAt: ""
    property string updatedAt: ""
    property string statusKind: "info"
    property string statusText: ""
    property int favoritesCount: 0
    property int historyCount: 0
    property int playlistsCount: 0
    property bool editingUsername: false
    property string activeTab: "favorites"
    property var favoritesPreview: []
    property var historyPreview: []
    property var playlistsPreview: []

    signal refreshRequested()
    signal saveUsernameRequested(string username)
    signal chooseAvatarRequested()
    signal favoritesShortcutRequested()
    signal historyShortcutRequested()
    signal playlistsShortcutRequested()
    signal reloginRequested()

    function normalizeText(value, fallback) {
        var text = value === undefined || value === null ? "" : String(value).trim()
        return text.length > 0 ? text : fallback
    }

    function normalizeArray(items) {
        return items && items.length !== undefined ? items : []
    }

    function avatarDisplaySource(value) {
        var source = normalizeText(value, "qrc:/qml/assets/ai/icons/default-user-avatar.svg")
        if (source.indexOf("qrc:/") === 0 || source.indexOf("file:") === 0)
            return source

        if (source.indexOf("http://") === 0 || source.indexOf("https://") === 0) {
            var separator = source.indexOf("?") >= 0 ? "&" : "?"
            return source + separator + "t=" + Date.now()
        }

        return source
    }

    function songTitle(item) {
        var title = normalizeText(item.title, "")
        if (title.length > 0)
            return title

        var path = normalizeText(item.path, "")
        if (path.length === 0)
            return "未知歌曲"

        var qPos = path.indexOf("?")
        if (qPos >= 0)
            path = path.substring(0, qPos)
        var slash = path.lastIndexOf("/")
        var name = slash >= 0 ? path.substring(slash + 1) : path
        var dot = name.lastIndexOf(".")
        if (dot > 0)
            name = name.substring(0, dot)
        try {
            name = decodeURIComponent(name)
        } catch (e) {
        }
        return normalizeText(name, "未知歌曲")
    }

    function songArtist(item) {
        return normalizeText(item.artist, "未知艺术家")
    }

    function songCover(item) {
        return normalizeText(item.cover_art_url, "qrc:/qml/assets/ai/icons/default-music-cover.svg")
    }

    function songTime(item) {
        return normalizeText(item.duration, "--:--")
    }

    function historyTime(item) {
        var played = normalizeText(item.played_at, "")
        if (played.length > 0)
            return played
        return normalizeText(item.updated_at, "最近播放")
    }

    function playlistCover(item) {
        return normalizeText(item.cover_url, "qrc:/qml/assets/ai/icons/default-music-cover.svg")
    }

    function playlistName(item) {
        return normalizeText(item.name, "未命名歌单")
    }

    function playlistDesc(item) {
        var description = normalizeText(item.description, "")
        if (description.length > 0)
            return description
        return "继续整理你的常听音乐"
    }

    function playlistMeta(item) {
        var trackCount = Number(item.track_count || 0)
        var updated = normalizeText(item.updated_at, "")
        return trackCount + " 首" + (updated.length > 0 ? " · " + updated : "")
    }

    function currentTabCount() {
        if (activeTab === "favorites")
            return favoritesCount
        if (activeTab === "history")
            return historyCount
        return playlistsCount
    }

    function currentTabLabel() {
        if (activeTab === "favorites")
            return "喜欢"
        if (activeTab === "history")
            return "最近播放"
        return "创建的歌单"
    }

    function openCurrentTabPage() {
        if (activeTab === "favorites") {
            favoritesShortcutRequested()
        } else if (activeTab === "history") {
            historyShortcutRequested()
        } else {
            playlistsShortcutRequested()
        }
    }

    function setProfileData(profile) {
        if (!profile)
            return

        username = normalizeText(profile.username, "未命名用户")
        account = normalizeText(profile.account, "--")
        committedAvatarSource = avatarDisplaySource(profile.avatar_url)
        avatarSource = committedAvatarSource
        createdAt = normalizeText(profile.created_at, "未知")
        updatedAt = normalizeText(profile.updated_at, "未知")
        sessionExpired = false
        if (!editingUsername) {
            usernameEdit.text = username
        }
    }

    function setStatsData(favorites, history, playlists) {
        favoritesCount = Math.max(0, favorites || 0)
        historyCount = Math.max(0, history || 0)
        playlistsCount = Math.max(0, playlists || 0)
    }

    function setPreviewData(favorites, history, playlists) {
        favoritesPreview = normalizeArray(favorites)
        historyPreview = normalizeArray(history)
        playlistsPreview = normalizeArray(playlists)
    }

    function setBusyState(value) {
        busy = value
    }

    function setUsernameSavingState(value) {
        usernameSaving = value
    }

    function setAvatarUploadingState(value) {
        avatarUploading = value
    }

    function setStatusMessage(kind, text) {
        statusKind = kind || "info"
        statusText = text || ""
    }

    function setSessionExpiredState(expired) {
        sessionExpired = expired
    }

    function setPendingAvatarPreview(source) {
        if (source && source.length > 0) {
            avatarSource = source
        }
    }

    function clearPendingAvatarPreview() {
        avatarSource = committedAvatarSource
    }

    Flickable {
        anchors.fill: parent
        anchors.margins: 28
        contentWidth: width
        contentHeight: pageColumn.implicitHeight
        clip: true

        Column {
            id: pageColumn
            width: parent.width
            spacing: 18

            Rectangle {
                width: parent.width
                height: visible ? statusRow.implicitHeight + 20 : 0
                radius: 14
                visible: sessionExpired || (statusKind === "error" && statusText.length > 0)
                color: sessionExpired
                       ? "#FFF4F3"
                       : (statusKind === "success" ? "#F2FBF5"
                                                   : (statusKind === "error" ? "#FFF5F3" : "#F6F9FE"))
                border.width: 1
                border.color: sessionExpired
                              ? "#F7C8C0"
                              : (statusKind === "success" ? "#C7E9D1"
                                                          : (statusKind === "error" ? "#F6C7BF" : "#DEE8F6"))

                RowLayout {
                    id: statusRow
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 12

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: sessionExpired
                               ? "#D33A31"
                               : (statusKind === "success" ? "#26A35E"
                                                           : (statusKind === "error" ? "#D33A31" : "#6B7EA6"))
                    }

                    Text {
                        Layout.fillWidth: true
                        text: sessionExpired ? "登录已过期，请重新登录后继续编辑个人资料。" : statusText
                        color: sessionExpired
                               ? "#B93830"
                               : (statusKind === "success" ? "#247F50"
                                                           : (statusKind === "error" ? "#B93830" : "#546888"))
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                    }

                    Rectangle {
                        visible: sessionExpired
                        implicitWidth: reloginText.implicitWidth + 22
                        height: 30
                        radius: 15
                        color: reloginArea.containsMouse ? Theme.accent : "transparent"
                        border.width: 1
                        border.color: Theme.accent

                        Text {
                            id: reloginText
                            anchors.centerIn: parent
                            text: "重新登录"
                            font.pixelSize: 12
                            color: reloginArea.containsMouse ? "#FFFFFF" : Theme.accent
                        }

                        MouseArea {
                            id: reloginArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.reloginRequested()
                        }
                    }
                }
            }

            Item {
                width: parent.width
                height: 208

                RowLayout {
                    anchors.fill: parent
                    spacing: 26

                    Item {
                        Layout.preferredWidth: 172
                        Layout.preferredHeight: 172

                        Rectangle {
                            anchors.fill: parent
                            radius: width / 2
                            color: "#EDF3F8"
                            border.width: 1
                            border.color: "#E2EAF2"

                            Item {
                                anchors.fill: parent
                                anchors.margins: 6

                                Image {
                                    id: profileAvatarImage
                                    anchors.fill: parent
                                    source: root.avatarSource
                                    fillMode: Image.PreserveAspectCrop
                                    smooth: true
                                    cache: false

                                    layer.enabled: true
                                    layer.effect: OpacityMask {
                                        maskSource: Rectangle {
                                            width: profileAvatarImage.width
                                            height: profileAvatarImage.height
                                            radius: width / 2
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.leftMargin: 16
                                anchors.rightMargin: 16
                                anchors.bottomMargin: 16
                                height: 34
                                radius: 17
                                color: avatarArea.containsMouse ? "#1A20263A" : "#1420263A"
                                visible: true

                                Text {
                                    anchors.centerIn: parent
                                    text: avatarUploading ? "上传中..." : "更换头像"
                                    font.pixelSize: 12
                                    color: "#FFFFFF"
                                }

                                MouseArea {
                                    id: avatarArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    enabled: !sessionExpired && !avatarUploading
                                    onClicked: root.chooseAvatarRequested()
                                }
                            }

                            Rectangle {
                                anchors.fill: parent
                                radius: width / 2
                                color: avatarUploading ? Qt.rgba(1, 1, 1, 0.68) : "transparent"
                                visible: avatarUploading

                                BusyIndicator {
                                    anchors.centerIn: parent
                                    running: avatarUploading
                                }
                            }
                        }
                    }

                    Column {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 10

                        Row {
                            spacing: 8

                            Text {
                                text: root.username
                                color: Theme.textPrimary
                                font.pixelSize: 30
                                font.weight: Font.DemiBold
                            }

                            Rectangle {
                                width: editText.implicitWidth + 20
                                height: 28
                                radius: 14
                                color: editArea.containsMouse ? "#FFF6F5" : "#FFFFFF"
                                border.width: 1
                                border.color: editArea.containsMouse ? "#F2C9C5" : "#E6ECF3"

                                Text {
                                    id: editText
                                    anchors.centerIn: parent
                                    text: root.editingUsername ? "收起编辑" : "编辑资料"
                                    font.pixelSize: 11
                                    color: editArea.containsMouse ? Theme.accent : "#7A8499"
                                }

                                MouseArea {
                                    id: editArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    enabled: !sessionExpired
                                    onClicked: {
                                        root.editingUsername = !root.editingUsername
                                        if (root.editingUsername) {
                                            usernameEdit.text = root.username
                                            usernameEdit.forceActiveFocus()
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                width: refreshText.implicitWidth + 20
                                height: 28
                                radius: 14
                                color: refreshHeadArea.containsMouse ? Theme.accentSoft : "#FFFFFF"
                                border.width: 1
                                border.color: refreshHeadArea.containsMouse ? "#F2C9C5" : "#E6ECF3"

                                Text {
                                    id: refreshText
                                    anchors.centerIn: parent
                                    text: root.busy ? "同步中..." : "刷新资料"
                                    font.pixelSize: 11
                                    color: refreshHeadArea.containsMouse ? Theme.accent : "#7A8499"
                                }

                                MouseArea {
                                    id: refreshHeadArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    enabled: !root.busy
                                    onClicked: root.refreshRequested()
                                }
                            }
                        }

                        Text {
                            text: "账号：" + root.account
                            color: "#667085"
                            font.pixelSize: 13
                        }

                        Text {
                            width: parent.width
                            text: root.sessionExpired
                                  ? "当前登录已失效，请重新登录后继续管理头像和用户名。"
                                  : "把喜欢的歌、最近播放和自己创建的歌单都整理在这里。"
                            color: root.sessionExpired ? "#B93830" : "#7A8499"
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                        }

                        Row {
                            spacing: 24

                            Repeater {
                                model: [
                                    { value: root.favoritesCount, label: "喜欢" },
                                    { value: root.playlistsCount, label: "歌单" },
                                    { value: root.historyCount, label: "最近播放" }
                                ]

                                delegate: Column {
                                    spacing: 4

                                    Text {
                                        text: modelData.value
                                        color: Theme.textPrimary
                                        font.pixelSize: 17
                                        font.weight: Font.DemiBold
                                    }

                                    Text {
                                        text: modelData.label
                                        color: "#98A2B3"
                                        font.pixelSize: 11
                                    }
                                }
                            }
                        }

                        Item {
                            width: parent.width
                            height: root.editingUsername ? 42 : 0
                            visible: root.editingUsername

                            RowLayout {
                                anchors.fill: parent
                                spacing: 10

                                TextField {
                                    id: usernameEdit
                                    Layout.preferredWidth: 280
                                    Layout.fillWidth: true
                                    placeholderText: "输入新的用户名"
                                    enabled: !sessionExpired && !usernameSaving
                                    selectByMouse: true

                                    background: Rectangle {
                                        radius: 12
                                        color: "#FFFFFF"
                                        border.width: 1
                                        border.color: usernameEdit.activeFocus ? Theme.accent : "#E2E8F0"
                                    }
                                }

                                Rectangle {
                                    Layout.preferredWidth: 74
                                    height: 36
                                    radius: 18
                                    color: saveArea.containsMouse ? Theme.accent : Theme.accentSoft
                                    border.width: 0

                                    Text {
                                        anchors.centerIn: parent
                                        text: root.usernameSaving ? "保存中" : "保存"
                                        font.pixelSize: 12
                                        color: saveArea.containsMouse ? "#FFFFFF" : Theme.accent
                                    }

                                    MouseArea {
                                        id: saveArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        enabled: !sessionExpired && !usernameSaving
                                        onClicked: root.saveUsernameRequested(usernameEdit.text)
                                    }
                                }

                                Rectangle {
                                    Layout.preferredWidth: 74
                                    height: 36
                                    radius: 18
                                    color: cancelArea.containsMouse ? "#F1F5F9" : "transparent"
                                    border.width: 1
                                    border.color: "#D8E0EA"

                                    Text {
                                        anchors.centerIn: parent
                                        text: "取消"
                                        font.pixelSize: 12
                                        color: "#667085"
                                    }

                                    MouseArea {
                                        id: cancelArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        enabled: !usernameSaving
                                        onClicked: {
                                            usernameEdit.text = root.username
                                            root.editingUsername = false
                                        }
                                    }
                                }
                            }
                        }

                        Row {
                            spacing: 16

                            Text {
                                text: "注册于 " + root.createdAt
                                color: "#98A2B3"
                                font.pixelSize: 11
                            }

                            Text {
                                text: "最近更新 " + root.updatedAt
                                color: "#98A2B3"
                                font.pixelSize: 11
                            }
                        }
                    }
                }
            }

            Row {
                width: parent.width
                spacing: 34

                Repeater {
                    model: [
                        { key: "favorites", label: "喜欢" },
                        { key: "playlists", label: "创建的歌单" },
                        { key: "history", label: "最近播放" }
                    ]

                    delegate: Item {
                        width: tabText.implicitWidth
                        height: 36
                        property bool active: root.activeTab === modelData.key

                        Text {
                            id: tabText
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            text: modelData.label
                            color: parent.active ? Theme.accent : "#4F5665"
                            font.pixelSize: 17
                            font.weight: parent.active ? Font.DemiBold : Font.Normal
                        }

                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottom: parent.bottom
                            width: parent.active ? Math.max(22, tabText.implicitWidth - 6) : 0
                            height: 3
                            radius: 2
                            color: Theme.accent
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.activeTab = modelData.key
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                radius: 18
                color: "#FFFFFF"
                border.width: 1
                border.color: "#EDF1F6"
                implicitHeight: contentColumn.implicitHeight + 28

                Column {
                    id: contentColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 22
                    spacing: 16

                    RowLayout {
                        width: parent.width

                        Text {
                            text: root.currentTabLabel()
                            color: Theme.textPrimary
                            font.pixelSize: 20
                            font.weight: Font.DemiBold
                            Layout.fillWidth: true
                        }

                        Text {
                            text: root.currentTabCount() + (root.activeTab === "playlists" ? " 个" : " 首")
                            color: "#98A2B3"
                            font.pixelSize: 13
                        }

                        Rectangle {
                            width: 74
                            height: 30
                            radius: 15
                            color: viewAllArea.containsMouse ? Theme.accentSoft : "transparent"
                            border.width: 0

                            Text {
                                anchors.centerIn: parent
                                text: "查看全部"
                                font.pixelSize: 12
                                color: Theme.accent
                            }

                            MouseArea {
                                id: viewAllArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.openCurrentTabPage()
                            }
                        }
                    }

                    StackLayout {
                        width: parent.width
                        currentIndex: root.activeTab === "favorites" ? 0 : (root.activeTab === "playlists" ? 1 : 2)

                        Item {
                            implicitHeight: favoritesColumn.implicitHeight

                            Column {
                                id: favoritesColumn
                                width: parent.width
                                spacing: 0

                                Rectangle {
                                    width: parent.width
                                    height: 40
                                    radius: 10
                                    color: "#F8FAFD"

                                    Row {
                                        anchors.fill: parent
                                        anchors.leftMargin: 14
                                        anchors.rightMargin: 14
                                        spacing: 12

                                        Text {
                                            width: 300
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: "歌曲"
                                            color: "#98A2B3"
                                            font.pixelSize: 12
                                        }

                                        Text {
                                            width: 180
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: "歌手"
                                            color: "#98A2B3"
                                            font.pixelSize: 12
                                        }

                                        Text {
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: "时长"
                                            color: "#98A2B3"
                                            font.pixelSize: 12
                                        }
                                    }
                                }

                                Repeater {
                                    model: Math.min(root.favoritesPreview.length, 8)

                                    delegate: Rectangle {
                                        width: favoritesColumn.width
                                        height: 62
                                        color: favoritesArea.containsMouse ? "#FAFCFE" : "transparent"
                                        radius: 10

                                        Row {
                                            anchors.fill: parent
                                            anchors.leftMargin: 14
                                            anchors.rightMargin: 14
                                            spacing: 12

                                            Row {
                                                width: 300
                                                height: parent.height
                                                spacing: 12

                                                Image {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    width: 38
                                                    height: 38
                                                    source: root.songCover(root.favoritesPreview[index])
                                                    fillMode: Image.PreserveAspectCrop
                                                    smooth: true
                                                }

                                                Column {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    spacing: 4

                                                    Text {
                                                        text: root.songTitle(root.favoritesPreview[index])
                                                        color: Theme.textPrimary
                                                        font.pixelSize: 14
                                                        elide: Text.ElideRight
                                                        width: 240
                                                    }

                                                    Text {
                                                        text: "我喜欢的音乐"
                                                        color: "#98A2B3"
                                                        font.pixelSize: 12
                                                    }
                                                }
                                            }

                                            Text {
                                                width: 180
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: root.songArtist(root.favoritesPreview[index])
                                                color: "#667085"
                                                font.pixelSize: 13
                                                elide: Text.ElideRight
                                            }

                                            Text {
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: root.songTime(root.favoritesPreview[index])
                                                color: "#667085"
                                                font.pixelSize: 13
                                            }
                                        }

                                        MouseArea {
                                            id: favoritesArea
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.favoritesShortcutRequested()
                                        }
                                    }
                                }

                                Text {
                                    visible: root.favoritesPreview.length === 0
                                    text: "你还没有收藏歌曲，先去挑几首喜欢的歌吧。"
                                    color: "#98A2B3"
                                    font.pixelSize: 13
                                    padding: 18
                                }
                            }
                        }

                        Item {
                            implicitHeight: playlistsColumn.implicitHeight

                            Column {
                                id: playlistsColumn
                                width: parent.width
                                spacing: 10

                                Repeater {
                                    model: Math.min(root.playlistsPreview.length, 8)

                                    delegate: Rectangle {
                                        width: playlistsColumn.width
                                        height: 82
                                        radius: 14
                                        color: playlistArea.containsMouse ? "#FAFCFE" : "#FFFFFF"
                                        border.width: 1
                                        border.color: playlistArea.containsMouse ? "#E4EBF4" : "#EDF1F6"

                                        Row {
                                            anchors.fill: parent
                                            anchors.margins: 14
                                            spacing: 14

                                            Image {
                                                width: 54
                                                height: 54
                                                source: root.playlistCover(root.playlistsPreview[index])
                                                fillMode: Image.PreserveAspectCrop
                                                smooth: true
                                                anchors.verticalCenter: parent.verticalCenter
                                            }

                                            Column {
                                                anchors.verticalCenter: parent.verticalCenter
                                                spacing: 6
                                                width: parent.width - 68

                                                Text {
                                                    text: root.playlistName(root.playlistsPreview[index])
                                                    color: Theme.textPrimary
                                                    font.pixelSize: 15
                                                    font.weight: Font.Medium
                                                    elide: Text.ElideRight
                                                    width: parent.width
                                                }

                                                Text {
                                                    text: root.playlistDesc(root.playlistsPreview[index])
                                                    color: "#7A8499"
                                                    font.pixelSize: 12
                                                    elide: Text.ElideRight
                                                    width: parent.width
                                                }

                                                Text {
                                                    text: root.playlistMeta(root.playlistsPreview[index])
                                                    color: "#98A2B3"
                                                    font.pixelSize: 12
                                                }
                                            }
                                        }

                                        MouseArea {
                                            id: playlistArea
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.playlistsShortcutRequested()
                                        }
                                    }
                                }

                                Text {
                                    visible: root.playlistsPreview.length === 0
                                    text: "还没有创建歌单，先为常听音乐整理一个主题吧。"
                                    color: "#98A2B3"
                                    font.pixelSize: 13
                                    padding: 18
                                }
                            }
                        }

                        Item {
                            implicitHeight: historyColumn.implicitHeight

                            Column {
                                id: historyColumn
                                width: parent.width
                                spacing: 0

                                Rectangle {
                                    width: parent.width
                                    height: 40
                                    radius: 10
                                    color: "#F8FAFD"

                                    Row {
                                        anchors.fill: parent
                                        anchors.leftMargin: 14
                                        anchors.rightMargin: 14
                                        spacing: 12

                                        Text {
                                            width: 300
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: "歌曲"
                                            color: "#98A2B3"
                                            font.pixelSize: 12
                                        }

                                        Text {
                                            width: 180
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: "歌手"
                                            color: "#98A2B3"
                                            font.pixelSize: 12
                                        }

                                        Text {
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: "最近播放"
                                            color: "#98A2B3"
                                            font.pixelSize: 12
                                        }
                                    }
                                }

                                Repeater {
                                    model: Math.min(root.historyPreview.length, 8)

                                    delegate: Rectangle {
                                        width: historyColumn.width
                                        height: 62
                                        color: historyArea.containsMouse ? "#FAFCFE" : "transparent"
                                        radius: 10

                                        Row {
                                            anchors.fill: parent
                                            anchors.leftMargin: 14
                                            anchors.rightMargin: 14
                                            spacing: 12

                                            Row {
                                                width: 300
                                                height: parent.height
                                                spacing: 12

                                                Image {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    width: 38
                                                    height: 38
                                                    source: root.songCover(root.historyPreview[index])
                                                    fillMode: Image.PreserveAspectCrop
                                                    smooth: true
                                                }

                                                Column {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    spacing: 4

                                                    Text {
                                                        text: root.songTitle(root.historyPreview[index])
                                                        color: Theme.textPrimary
                                                        font.pixelSize: 14
                                                        elide: Text.ElideRight
                                                        width: 240
                                                    }

                                                    Text {
                                                        text: root.songTime(root.historyPreview[index])
                                                        color: "#98A2B3"
                                                        font.pixelSize: 12
                                                    }
                                                }
                                            }

                                            Text {
                                                width: 180
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: root.songArtist(root.historyPreview[index])
                                                color: "#667085"
                                                font.pixelSize: 13
                                                elide: Text.ElideRight
                                            }

                                            Text {
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: root.historyTime(root.historyPreview[index])
                                                color: "#667085"
                                                font.pixelSize: 13
                                            }
                                        }

                                        MouseArea {
                                            id: historyArea
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.historyShortcutRequested()
                                        }
                                    }
                                }

                                Text {
                                    visible: root.historyPreview.length === 0
                                    text: "最近还没有播放记录，去听点音乐吧。"
                                    color: "#98A2B3"
                                    font.pixelSize: 13
                                    padding: 18
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
