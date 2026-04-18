import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../theme/Theme.js" as Theme

Rectangle {
    id: root
    color: "#FAFAFA"
    radius: 0

    property var trackContext: ({})
    property var threadMeta: ({})
    property string displayMode: "drawer"
    property bool commentEnabled: false
    property bool loggedIn: false
    property bool commentsLoading: false
    property bool repliesLoading: false
    property bool submitting: false
    property var commentItems: []
    property int commentPage: 1
    property bool commentHasMore: false
    property string commentErrorMessage: ""
    property var expandedRootCommentId: 0
    property var replyItems: []
    property int replyPage: 1
    property bool replyHasMore: false
    property int replyTotal: 0
    property string replyErrorMessage: ""
    property var replyRootCommentId: 0
    property var replyTargetCommentId: 0
    property string replyTargetUsername: ""
    property string activeTab: "comments"

    readonly property bool pageMode: displayMode === "page"
    readonly property int horizontalPadding: pageMode ? 42 : 18
    readonly property int topPadding: pageMode ? 28 : 18
    readonly property int contentMaxWidth: pageMode ? 860 : Math.max(360, width - 24)

    signal closeRequested()
    signal loadMoreCommentsRequested()
    signal toggleRepliesRequested(var rootCommentId, bool expanded)
    signal loadMoreRepliesRequested(var rootCommentId)
    signal submitMainCommentRequested(string content)
    signal submitReplyRequested(var rootCommentId, var targetCommentId, string content)
    signal deleteCommentRequested(var commentId)
    signal startReplyRequested(var rootCommentId, var targetCommentId, string username)
    signal loginRequested()

    function updateComments(items, page, hasMore, errorMessage) {
        root.commentItems = items
        root.commentPage = page
        root.commentHasMore = hasMore
        root.commentErrorMessage = errorMessage || ""
    }

    function updateReplies(rootCommentId, items, page, hasMore, total, errorMessage) {
        root.expandedRootCommentId = rootCommentId
        root.replyItems = items
        root.replyPage = page
        root.replyHasMore = hasMore
        root.replyTotal = total
        root.replyErrorMessage = errorMessage || ""
    }

    function setReplyTarget(rootCommentId, targetCommentId, username) {
        root.replyRootCommentId = rootCommentId
        root.replyTargetCommentId = targetCommentId
        root.replyTargetUsername = username || ""
        composer.forceActiveFocus()
    }

    function clearReplyTarget() {
        root.replyRootCommentId = 0
        root.replyTargetCommentId = 0
        root.replyTargetUsername = ""
    }

    function clearComposer() {
        composer.text = ""
    }

    function displayContent(item) {
        if (!item)
            return ""
        if (item.display_content)
            return item.display_content
        return item.content || ""
    }

    function replyPrefix(item) {
        if (!item || !item.reply_to || !item.reply_to.username)
            return ""
        return "回复 @" + item.reply_to.username + " "
    }

    function normalizedTitle() {
        return trackContext.title || "当前歌曲"
    }

    function normalizedArtist() {
        return trackContext.artist || "未知歌手"
    }

    function normalizedAlbum() {
        return trackContext.album || threadMeta.album || "暂无专辑信息"
    }

    function normalizedDescription() {
        if (!commentEnabled)
            return "当前歌曲不是在线评论线程，本地文件与 file:/// 路径不会接入评论服务。"
        if (threadMeta.description)
            return threadMeta.description
        if ((trackContext.music_path || "").indexOf("[jamendo-") >= 0)
            return "当前歌曲来自 Jamendo 虚拟路径，评论线程将直接绑定原始音乐路径。"
        return "当前页面展示在线歌曲评论与基础信息，评论与回复顺序遵循服务端返回。"
    }

    function sourceLabel() {
        var path = trackContext.music_path || ""
        if (!commentEnabled)
            return "本地歌曲"
        if (path.indexOf("[jamendo-") >= 0)
            return "Jamendo 在线曲库"
        return "在线音乐"
    }

    function commentCountValue() {
        return threadMeta.root_comment_count || commentItems.length || 0
    }

    function composerPlaceholder() {
        if (!commentEnabled)
            return "仅在线歌曲支持评论"
        if (!loggedIn)
            return "登录后可发表评论"
        if (replyTargetUsername.length > 0)
            return "回复 @" + replyTargetUsername
        return "期待你的神评论"
    }

    function submitComposer() {
        var text = composer.text.trim()
        if (text.length === 0 || submitting)
            return
        if (!loggedIn) {
            loginRequested()
            return
        }
        if (replyRootCommentId > 0) {
            submitReplyRequested(replyRootCommentId, replyTargetCommentId, text)
            return
        }
        submitMainCommentRequested(text)
    }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: contentColumn.implicitHeight + root.topPadding * 2
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: contentColumn
            x: Math.max(root.horizontalPadding,
                        Math.round((root.width - Math.min(root.contentMaxWidth,
                                                          root.width - root.horizontalPadding * 2)) / 2))
            y: root.topPadding
            width: Math.min(root.contentMaxWidth, root.width - root.horizontalPadding * 2)
            spacing: 24

            Item {
                width: parent.width
                height: infoRow.implicitHeight

                RowLayout {
                    id: infoRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 22

                    Rectangle {
                        Layout.preferredWidth: root.pageMode ? 124 : 88
                        Layout.preferredHeight: root.pageMode ? 124 : 88
                        radius: root.pageMode ? 18 : 14
                        color: "#FFFFFF"
                        border.width: 1
                        border.color: "#ECECEC"
                        clip: true

                        Image {
                            anchors.fill: parent
                            source: trackContext.cover || "qrc:/qml/assets/ai/icons/default-music-cover.svg"
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Text {
                            Layout.fillWidth: true
                            text: root.normalizedTitle()
                            color: "#1F1F1F"
                            font.pixelSize: root.pageMode ? 30 : 24
                            font.weight: Font.Bold
                            elide: Text.ElideRight
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 18

                            Text {
                                text: "歌手："
                                color: "#666666"
                                font.pixelSize: 13
                            }

                            Text {
                                text: root.normalizedArtist()
                                color: "#222222"
                                font.pixelSize: 13
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 18

                            Text {
                                text: "专辑："
                                color: "#666666"
                                font.pixelSize: 13
                            }

                            Text {
                                text: root.normalizedAlbum()
                                color: "#222222"
                                font.pixelSize: 13
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }
                    }

                    Rectangle {
                        visible: !root.pageMode
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: 68
                        Layout.preferredHeight: 30
                        radius: 15
                        color: "#F0F0F0"
                        border.width: 1
                        border.color: "#E4E4E4"

                        Text {
                            anchors.centerIn: parent
                            text: "关闭"
                            color: "#707070"
                            font.pixelSize: 12
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.closeRequested()
                        }
                    }
                }
            }

            Item {
                width: parent.width
                height: 44

                RowLayout {
                    anchors.fill: parent
                    anchors.bottomMargin: 1
                    spacing: 28

                    Rectangle {
                        Layout.preferredWidth: commentTabText.implicitWidth + 4
                        Layout.fillHeight: true
                        color: "transparent"

                        Text {
                            id: commentTabText
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 8
                            text: "评论 " + root.commentCountValue()
                            color: root.activeTab === "comments" ? "#1ECE9B" : "#222222"
                            font.pixelSize: 17
                            font.weight: Font.Medium
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 2
                            color: root.activeTab === "comments" ? "#1ECE9B" : "transparent"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.activeTab = "comments"
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: detailTabText.implicitWidth + 4
                        Layout.fillHeight: true
                        color: "transparent"

                        Text {
                            id: detailTabText
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 8
                            text: "详情"
                            color: root.activeTab === "details" ? "#1ECE9B" : "#222222"
                            font.pixelSize: 17
                            font.weight: Font.Medium
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 2
                            color: root.activeTab === "details" ? "#1ECE9B" : "transparent"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.activeTab = "details"
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: "#E8E8E8"
                }
            }

            Column {
                width: parent.width
                spacing: 18
                visible: root.activeTab === "comments"

                Rectangle {
                    width: parent.width
                    color: "#F2F2F2"
                    radius: 12
                    implicitHeight: composerColumn.implicitHeight + 24

                    Column {
                        id: composerColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 14
                        spacing: 10

                        Rectangle {
                            visible: root.replyTargetUsername.length > 0
                            width: parent.width
                            height: visible ? 32 : 0
                            radius: 16
                            color: "#FFFFFF"
                            border.width: 1
                            border.color: "#E1E1E1"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 8
                                spacing: 8

                                Text {
                                    Layout.fillWidth: true
                                    text: "正在回复 @" + root.replyTargetUsername
                                    color: "#5B5B5B"
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                }

                                Text {
                                    text: "取消"
                                    color: "#1ECE9B"
                                    font.pixelSize: 12

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: root.clearReplyTarget()
                                    }
                                }
                            }
                        }

                        TextArea {
                            id: composer
                            width: parent.width
                            height: root.pageMode ? 86 : 72
                            wrapMode: TextEdit.Wrap
                            color: "#333333"
                            selectByMouse: true
                            readOnly: !root.commentEnabled || !root.loggedIn || root.submitting
                            placeholderText: root.composerPlaceholder()
                            placeholderTextColor: "#A7A7A7"
                            font.pixelSize: 14
                            background: Rectangle {
                                color: "transparent"
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: !root.loggedIn && root.commentEnabled
                                onClicked: root.loginRequested()
                            }
                        }

                        RowLayout {
                            width: parent.width
                            spacing: 12

                            Item { Layout.fillWidth: true }

                            Rectangle {
                                width: 76
                                height: 34
                                radius: 17
                                color: root.commentEnabled ? "#FFFFFF" : "#F7F7F7"
                                border.width: 1
                                border.color: "#E0E0E0"

                                Text {
                                    anchors.centerIn: parent
                                    text: !root.commentEnabled ? "不可用" : (!root.loggedIn ? "登录" : (root.submitting ? "提交中" : "发布"))
                                    color: !root.commentEnabled ? "#B0B0B0" : "#5C5C5C"
                                    font.pixelSize: 13
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    enabled: root.commentEnabled && !root.submitting
                                    onClicked: root.submitComposer()
                                }
                            }
                        }
                    }
                }

                Text {
                    visible: root.commentErrorMessage.length > 0 && !root.commentsLoading
                    width: parent.width
                    text: root.commentErrorMessage
                    color: Theme.accent
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }

                BusyIndicator {
                    visible: root.commentsLoading
                    anchors.horizontalCenter: parent.horizontalCenter
                    running: visible
                }

                Column {
                    width: parent.width
                    spacing: 22

                    Text {
                        visible: !root.commentsLoading
                        text: root.commentItems.length > 0 ? "近期评论" : "还没有评论"
                        color: "#222222"
                        font.pixelSize: 17
                        font.weight: Font.Bold
                    }

                    Repeater {
                        model: root.commentItems

                        delegate: Item {
                            property var itemData: modelData
                            width: parent ? parent.width : 0
                            height: commentBody.implicitHeight + 26

                            Column {
                                id: commentBody
                                width: parent.width
                                spacing: 12

                                Row {
                                    width: parent.width
                                    spacing: 12

                                    Rectangle {
                                        width: 40
                                        height: 40
                                        radius: 20
                                        color: "#F1F1F1"
                                        clip: true

                                        Image {
                                            anchors.fill: parent
                                            source: itemData.avatar_url || "qrc:/qml/assets/ai/icons/default-user-avatar.svg"
                                            fillMode: Image.PreserveAspectCrop
                                            asynchronous: true
                                        }
                                    }

                                    Column {
                                        width: parent.width - 52
                                        spacing: 4

                                        Text {
                                            width: parent.width
                                            text: itemData.username || "匿名用户"
                                            color: "#D89E53"
                                            font.pixelSize: 14
                                            font.weight: Font.Medium
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            width: parent.width
                                            text: itemData.created_at || ""
                                            color: "#A0A0A0"
                                            font.pixelSize: 12
                                            elide: Text.ElideRight
                                        }
                                    }
                                }

                                Text {
                                    width: parent.width - 52
                                    x: 52
                                    text: root.displayContent(itemData)
                                    color: itemData.is_deleted ? "#A3A3A3" : "#2A2A2A"
                                    font.pixelSize: 14
                                    wrapMode: Text.WordWrap
                                    lineHeight: 1.4
                                }

                                Row {
                                    x: 52
                                    spacing: 18

                                    Text {
                                        text: "回复"
                                        color: "#7A7A7A"
                                        font.pixelSize: 13

                                        MouseArea {
                                            anchors.fill: parent
                                            enabled: !itemData.is_deleted
                                            onClicked: {
                                                if (!root.loggedIn) {
                                                    root.loginRequested()
                                                    return
                                                }
                                                root.startReplyRequested(itemData.comment_id, itemData.comment_id, itemData.username || "")
                                            }
                                        }
                                    }

                                    Text {
                                        text: root.expandedRootCommentId === itemData.comment_id
                                              ? "收起回复"
                                              : ("查看回复" + ((itemData.reply_count || 0) > 0 ? " " + itemData.reply_count : ""))
                                        color: "#7A7A7A"
                                        font.pixelSize: 13

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: {
                                                var expanding = root.expandedRootCommentId !== itemData.comment_id
                                                root.toggleRepliesRequested(itemData.comment_id, expanding)
                                            }
                                        }
                                    }

                                    Text {
                                        visible: itemData.can_delete === true
                                        text: "删除"
                                        color: Theme.accent
                                        font.pixelSize: 13

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: {
                                                if (!root.loggedIn) {
                                                    root.loginRequested()
                                                    return
                                                }
                                                root.deleteCommentRequested(itemData.comment_id)
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    visible: root.expandedRootCommentId === itemData.comment_id
                                    width: parent.width - 52
                                    x: 52
                                    color: "#F5F5F5"
                                    radius: 12
                                    border.width: 1
                                    border.color: "#EBEBEB"
                                    implicitHeight: repliesColumn.implicitHeight + 20

                                    Column {
                                        id: repliesColumn
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 12
                                        spacing: 10

                                        BusyIndicator {
                                            visible: root.repliesLoading
                                            running: visible
                                            anchors.horizontalCenter: parent.horizontalCenter
                                        }

                                        Text {
                                            visible: root.replyErrorMessage.length > 0 && !root.repliesLoading
                                            width: parent.width
                                            text: root.replyErrorMessage
                                            color: Theme.accent
                                            font.pixelSize: 12
                                            wrapMode: Text.WordWrap
                                        }

                                        Repeater {
                                            model: root.expandedRootCommentId === itemData.comment_id ? root.replyItems : []

                                            delegate: Item {
                                                property var replyData: modelData
                                                width: parent ? parent.width : 0
                                                height: replyColumn.implicitHeight

                                                Column {
                                                    id: replyColumn
                                                    width: parent.width
                                                    spacing: 4

                                                    Text {
                                                        width: parent.width
                                                        text: (replyData.username || "匿名用户") + "  " + (replyData.created_at || "")
                                                        color: "#7B7B7B"
                                                        font.pixelSize: 12
                                                        elide: Text.ElideRight
                                                    }

                                                    Text {
                                                        width: parent.width
                                                        text: root.replyPrefix(replyData) + root.displayContent(replyData)
                                                        color: replyData.is_deleted ? "#A3A3A3" : "#323232"
                                                        font.pixelSize: 13
                                                        wrapMode: Text.WordWrap
                                                        lineHeight: 1.4
                                                    }

                                                    Row {
                                                        spacing: 14

                                                        Text {
                                                            text: "回复"
                                                            color: "#7A7A7A"
                                                            font.pixelSize: 12

                                                            MouseArea {
                                                                anchors.fill: parent
                                                                enabled: !replyData.is_deleted
                                                                onClicked: {
                                                                    if (!root.loggedIn) {
                                                                        root.loginRequested()
                                                                        return
                                                                    }
                                                                    root.startReplyRequested(itemData.comment_id, replyData.comment_id, replyData.username || "")
                                                                }
                                                            }
                                                        }

                                                        Text {
                                                            visible: replyData.can_delete === true
                                                            text: "删除"
                                                            color: Theme.accent
                                                            font.pixelSize: 12

                                                            MouseArea {
                                                                anchors.fill: parent
                                                                onClicked: {
                                                                    if (!root.loggedIn) {
                                                                        root.loginRequested()
                                                                        return
                                                                    }
                                                                    root.deleteCommentRequested(replyData.comment_id)
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        Text {
                                            visible: !root.repliesLoading && root.replyItems.length === 0 &&
                                                     root.replyErrorMessage.length === 0
                                            width: parent.width
                                            text: "暂无回复"
                                            color: "#A0A0A0"
                                            font.pixelSize: 12
                                        }

                                        Text {
                                            visible: root.replyHasMore && !root.repliesLoading
                                            text: "加载更多回复"
                                            color: "#1ECE9B"
                                            font.pixelSize: 12

                                            MouseArea {
                                                anchors.fill: parent
                                                onClicked: root.loadMoreRepliesRequested(itemData.comment_id)
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    width: parent.width
                                    height: 1
                                    color: "#EFEFEF"
                                }
                            }
                        }
                    }

                    Text {
                        visible: !root.commentsLoading && root.commentItems.length === 0 &&
                                 root.commentErrorMessage.length === 0
                        width: parent.width
                        text: root.commentEnabled ? "还没有人发表评论，来留下第一条评论。" : "当前歌曲不支持评论。"
                        color: "#9D9D9D"
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        visible: root.commentHasMore && !root.commentsLoading
                        text: "加载更多评论"
                        color: "#1ECE9B"
                        font.pixelSize: 13

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.loadMoreCommentsRequested()
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                visible: root.activeTab === "details"
                color: "transparent"
                implicitHeight: detailsColumn.implicitHeight

                Column {
                    id: detailsColumn
                    width: parent.width
                    spacing: 20

                    Repeater {
                        model: [
                            { label: "演唱者", value: root.normalizedArtist() },
                            { label: "专辑", value: root.normalizedAlbum() },
                            { label: "来源", value: root.sourceLabel() },
                            { label: "评论数", value: String(root.commentCountValue()) },
                            { label: "评论路径", value: trackContext.music_path || "暂无可用路径" },
                            { label: "简介", value: root.normalizedDescription() }
                        ]

                        delegate: Row {
                            width: parent ? parent.width : 0
                            spacing: 18

                            Text {
                                width: 86
                                text: modelData.label + "："
                                color: "#1F1F1F"
                                font.pixelSize: 14
                                font.weight: Font.Medium
                            }

                            Text {
                                width: parent.width - 104
                                text: modelData.value
                                color: "#666666"
                                font.pixelSize: 14
                                wrapMode: Text.WordWrap
                                lineHeight: 1.45
                            }
                        }
                    }
                }
            }
        }
    }
}
