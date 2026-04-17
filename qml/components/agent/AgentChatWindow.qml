import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../theme/AiChatTheme.js" as AiTheme

Rectangle {
    id: root
    color: AiTheme.colors.windowBg
    property bool followLatest: true
    property string editingSessionId: ""
    property string editingTitleText: ""
    property string pendingDeleteSessionId: ""
    property string pendingDeleteTitle: ""

    function blockType(block) { return block && block.type ? block.type : "paragraph" }
    function blockText(block) { return block && block.rawText ? block.rawText : "" }
    function headingPixelSize(level) { return level <= 1 ? 24 : (level === 2 ? 21 : (level === 3 ? 19 : 16)) }
    function mapValue(meta, key, fallbackValue) {
        if (!meta || meta[key] === undefined || meta[key] === null) return fallbackValue
        return meta[key]
    }
    function mapString(meta, key, fallbackValue) {
        const v = mapValue(meta, key, fallbackValue || "")
        return (v === undefined || v === null) ? (fallbackValue || "") : String(v)
    }
    function mapList(meta, key) {
        const v = mapValue(meta, key, [])
        return v && v.length !== undefined ? v : []
    }
    function isNearBottom() { return (listView.contentY + listView.height) >= (listView.contentHeight - 24) }
    function requestScrollToBottom() { if (followLatest) { scrollTimer.restart() } }
    function formatTime(ts) { if (!ts) return ""; const d = new Date(ts); return isNaN(d.getTime()) ? "" : Qt.formatTime(d, "hh:mm") }
    function doSearch() { if (agentChatVM) agentChatVM.searchSessions(searchField.text.trim()) }
    function requestDeleteSession(sessionId, title) {
        pendingDeleteSessionId = sessionId
        pendingDeleteTitle = title
        deleteConfirmDialog.open()
    }
    function copyTextToClipboard(value) {
        const text = value === undefined || value === null ? "" : String(value)
        if (text.length === 0)
            return
        messageCopyProxy.text = text
        messageCopyProxy.selectAll()
        messageCopyProxy.copy()
        messageCopyProxy.deselect()
    }
    function sendCurrentMessage() {
        if (!agentChatVM) return
        const raw = inputArea.text
        if (!raw || raw.trim().length === 0) return
        agentChatVM.sendMessage(raw)
        inputArea.clear()
        followLatest = true
        requestScrollToBottom()
    }

    Component.onCompleted: {
        if (agentChatVM) {
            agentChatVM.initialize()
            agentChatVM.loadSessions("")
        }
    }

    Timer {
        id: scrollTimer
        interval: 50
        repeat: false
        onTriggered: listView.positionViewAtEnd()
    }

    Timer {
        id: searchDebounceTimer
        interval: 260
        repeat: false
        onTriggered: root.doSearch()
    }

    TextEdit {
        id: messageCopyProxy
        visible: false
        textFormat: TextEdit.PlainText
    }

    Dialog {
        id: deleteConfirmDialog
        modal: true
        title: "删除会话"
        width: 380
        implicitWidth: 380
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            if (agentChatVM && root.pendingDeleteSessionId.length > 0) {
                agentChatVM.deleteSession(root.pendingDeleteSessionId)
            }
            root.pendingDeleteSessionId = ""
            root.pendingDeleteTitle = ""
        }
        onRejected: {
            root.pendingDeleteSessionId = ""
            root.pendingDeleteTitle = ""
        }
        contentItem: Item {
            implicitWidth: 340
            implicitHeight: 72

            Label {
                anchors.fill: parent
                anchors.margins: 8
                text: "确认删除会话“" + (root.pendingDeleteTitle || "未命名会话") + "”？"
                wrapMode: Text.Wrap
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: AiTheme.spacing.outer
        spacing: AiTheme.spacing.outer

        Rectangle {
            Layout.preferredWidth: 280
            Layout.fillHeight: true
            radius: AiTheme.radius.panel
            color: AiTheme.colors.sidebarBg
            border.color: AiTheme.colors.border

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                Label {
                    text: "云音乐 AI 助手"
                    font.pixelSize: 17
                    font.bold: true
                    color: AiTheme.colors.textPrimary
                }

                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: "搜索会话"
                    leftPadding: 30
                    onTextChanged: searchDebounceTimer.restart()

                    background: Rectangle {
                        radius: 10
                        color: "#F3F4F6"
                        border.color: AiTheme.colors.border
                    }

                    Image {
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        width: 15
                        height: 15
                        source: "qrc:/qml/assets/ai/icons/search.svg"
                    }
                }

                ListView {
                    id: sessionListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 6
                    model: agentChatVM ? agentChatVM.sessionModel : null

                    delegate: Rectangle {
                        id: sessionDelegate
                        width: ListView.view.width
                        implicitHeight: 68
                        radius: 10
                        border.width: 1
                        color: selected ? "#ECFDF5" : "transparent"
                        border.color: selected ? "#A7F3D0" : "transparent"

                        property bool hovered: false
                        property bool editing: root.editingSessionId === sessionId

                        Column {
                            z: 1
                            anchors.left: parent.left
                            anchors.right: opRow.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.margins: 10
                            spacing: 2

                            Loader {
                                width: parent.width
                                sourceComponent: sessionDelegate.editing ? editTitleComponent : viewTitleComponent
                            }

                            Text {
                                width: parent.width
                                text: lastPreview && lastPreview.length > 0 ? lastPreview : "等待首条消息"
                                elide: Text.ElideRight
                                color: AiTheme.colors.textSecondary
                                font.pixelSize: 12
                            }

                            Text {
                                width: parent.width
                                text: updatedAtText
                                elide: Text.ElideRight
                                color: "#94A3B8"
                                font.pixelSize: 11
                            }
                        }

                        Component {
                            id: viewTitleComponent
                            Text {
                                width: parent.width
                                text: title
                                elide: Text.ElideRight
                                color: AiTheme.colors.textPrimary
                                font.pixelSize: 13
                                font.bold: selected
                            }
                        }

                        Component {
                            id: editTitleComponent
                            TextField {
                                id: renameField
                                width: parent.width
                                text: root.editingTitleText
                                selectByMouse: true
                                background: Rectangle {
                                    radius: 8
                                    color: "#FFFFFF"
                                    border.color: "#D1D5DB"
                                }
                                onTextChanged: root.editingTitleText = text
                                Component.onCompleted: {
                                    renameField.forceActiveFocus()
                                    renameField.selectAll()
                                }
                                onAccepted: {
                                    const newTitle = text.trim()
                                    if (newTitle.length > 0 && agentChatVM) {
                                        agentChatVM.renameSession(sessionId, newTitle)
                                    }
                                    root.editingSessionId = ""
                                    root.editingTitleText = ""
                                }
                                onActiveFocusChanged: {
                                    if (activeFocus) return
                                    const newTitle = text.trim()
                                    if (newTitle.length > 0 && newTitle !== title && agentChatVM) {
                                        agentChatVM.renameSession(sessionId, newTitle)
                                    }
                                    root.editingSessionId = ""
                                    root.editingTitleText = ""
                                }
                                Keys.onEscapePressed: {
                                    root.editingSessionId = ""
                                    root.editingTitleText = ""
                                }
                            }
                        }

                        Row {
                            id: opRow
                            z: 2
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 4
                            visible: hovered || selected || sessionDelegate.editing

                            ToolButton {
                                visible: !sessionDelegate.editing
                                text: "改"
                                onClicked: {
                                    root.editingSessionId = sessionId
                                    root.editingTitleText = title
                                }
                            }
                            ToolButton {
                                visible: !sessionDelegate.editing
                                text: "删"
                                onClicked: root.requestDeleteSession(sessionId, title)
                            }
                        }

                        MouseArea {
                            z: 0
                            anchors.fill: parent
                            enabled: !sessionDelegate.editing
                            hoverEnabled: true
                            acceptedButtons: Qt.LeftButton
                            onEntered: sessionDelegate.hovered = true
                            onExited: sessionDelegate.hovered = false
                            onClicked: {
                                if (sessionDelegate.editing) {
                                    return
                                }
                                if (agentChatVM) {
                                    agentChatVM.selectSession(sessionId)
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 56
                    color: "transparent"
                    visible: sessionListView.count === 0

                    Label {
                        anchors.centerIn: parent
                        text: agentChatVM && agentChatVM.loadingSessions ? "会话加载中..." : "暂无会话，点击下方新建"
                        color: AiTheme.colors.textSecondary
                        font.pixelSize: 12
                    }
                }

                Button {
                    Layout.fillWidth: true
                    text: "新建会话"
                    icon.source: "qrc:/qml/assets/ai/icons/plus.svg"
                    onClicked: {
                        root.editingSessionId = ""
                        root.editingTitleText = ""
                        searchField.text = ""
                        if (agentChatVM) {
                            agentChatVM.createSession("新建会话")
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: AiTheme.radius.panel
            color: AiTheme.colors.panelBg
            border.color: AiTheme.colors.border

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 56
                    radius: AiTheme.radius.card
                    color: AiTheme.colors.softBg
                    border.color: AiTheme.colors.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Row {
                            spacing: 8

                            Rectangle {
                                width: 84
                                height: 32
                                radius: 16
                                color: agentChatVM && agentChatVM.agentMode === "control"
                                       ? AiTheme.colors.accent
                                       : "#F3F4F6"
                                border.width: 1
                                border.color: agentChatVM && agentChatVM.agentMode === "control"
                                              ? AiTheme.colors.accent
                                              : AiTheme.colors.border

                                Text {
                                    anchors.centerIn: parent
                                    text: "控制模式"
                                    color: agentChatVM && agentChatVM.agentMode === "control"
                                           ? "#FFFFFF"
                                           : AiTheme.colors.textSecondary
                                    font.pixelSize: 12
                                    font.weight: Font.Medium
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: if (agentChatVM) agentChatVM.setAgentMode("control")
                                }
                            }

                            Rectangle {
                                width: 84
                                height: 32
                                radius: 16
                                color: agentChatVM && agentChatVM.agentMode === "assistant"
                                       ? AiTheme.colors.accent
                                       : "#F3F4F6"
                                border.width: 1
                                border.color: agentChatVM && agentChatVM.agentMode === "assistant"
                                              ? AiTheme.colors.accent
                                              : AiTheme.colors.border

                                Text {
                                    anchors.centerIn: parent
                                    text: "助手模式"
                                    color: agentChatVM && agentChatVM.agentMode === "assistant"
                                           ? "#FFFFFF"
                                           : AiTheme.colors.textSecondary
                                    font.pixelSize: 12
                                    font.weight: Font.Medium
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: if (agentChatVM) agentChatVM.setAgentMode("assistant")
                                }
                            }
                        }

                        Label {
                            text: agentChatVM ? agentChatVM.connectionStateText : "AI 助手未就绪"
                            color: "#166534"
                            font.bold: true
                            font.pixelSize: 12
                        }

                        Label {
                            Layout.fillWidth: true
                            text: agentChatVM && agentChatVM.currentSessionTitle.length > 0
                                  ? ("当前会话：" + agentChatVM.currentSessionTitle)
                                  : "当前会话：未选择"
                            color: AiTheme.colors.textSecondary
                            font.pixelSize: 12
                            elide: Text.ElideMiddle
                        }

                        Column {
                            spacing: 2

                            Label {
                                text: agentChatVM && agentChatVM.localModelName.length > 0
                                      ? ("本地模型：" + agentChatVM.localModelName)
                                      : "本地模型：未配置"
                                color: AiTheme.colors.textSecondary
                                font.pixelSize: 12
                            }

                            Label {
                                text: agentChatVM && agentChatVM.agentMode === "assistant"
                                      ? (agentChatVM.remoteFallbackEnabled
                                         ? "助手模式：仅解释，不直接执行写操作"
                                         : "助手模式：远程兜底未启用，仅可查看解释性说明")
                                      : (agentChatVM && agentChatVM.remoteFallbackEnabled
                                         ? "远程兜底：已启用"
                                         : "远程兜底：未启用")
                                color: agentChatVM && agentChatVM.agentMode === "assistant"
                                       ? "#B45309"
                                       : AiTheme.colors.textSecondary
                                font.pixelSize: 11
                            }
                        }

                        ToolButton {
                            icon.source: "qrc:/qml/assets/ai/icons/sync.svg"
                            onClicked: if (agentChatVM) agentChatVM.retryConnection()
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: AiTheme.radius.card
                    color: AiTheme.colors.softBg
                    border.color: AiTheme.colors.border

                    ListView {
                        id: listView
                        anchors.fill: parent
                        anchors.margins: 12
                        clip: true
                        spacing: 12
                        reuseItems: true
                        cacheBuffer: 400
                        model: agentChatVM ? agentChatVM.messageModel : null

                        onMovementStarted: root.followLatest = false
                        onMovementEnded: root.followLatest = root.isNearBottom()

                        delegate: Item {
                            width: listView.width
                            implicitHeight: metaLine.implicitHeight + bubble.implicitHeight + 10
                            readonly property bool isUser: role === "user"
                            readonly property bool isError: role === "error"
                            readonly property bool isSystem: role === "system"
                            readonly property bool isAssistant: role === "assistant"
                            readonly property bool isStreaming: status === "streaming"
                            readonly property bool isPlanPreview: messageType === "plan_preview"
                            readonly property bool isApprovalRequest: messageType === "approval_request"
                            readonly property bool hasStructuredCard: isPlanPreview || isApprovalRequest
                            readonly property bool hasBlocks: blocks && blocks.length > 0
                            readonly property string fallbackText: (isAssistant && isStreaming && (!text || text.length === 0)) ? "正在输入..." : text
                            readonly property string copySourceText: rawText && rawText.length > 0 ? rawText : fallbackText
                            readonly property int maxBubbleWidth: Math.max(220, listView.width - 72)
                            readonly property int assistantBubbleWidth: Math.min(maxBubbleWidth, 940)
                            property bool hovered: false
                            property bool copied: false

                            Text { id: userMeasure; visible: false; text: fallbackText; font.pixelSize: 14 }

                            Timer {
                                id: copyFeedbackTimer
                                interval: 1200
                                repeat: false
                                onTriggered: copied = false
                            }

                            Rectangle {
                                id: bubble
                                width: isUser ? Math.min(maxBubbleWidth, Math.max(120, userMeasure.implicitWidth + 30)) : assistantBubbleWidth
                                x: isUser ? (parent.width - width - 4) : 4
                                y: metaLine.implicitHeight + 2
                                radius: AiTheme.radius.bubble
                                border.width: 1
                                color: isUser ? AiTheme.colors.userBubble : (isError ? "#FFF1F2" : (isSystem ? "#F3F4F6" : AiTheme.colors.assistantBubble))
                                border.color: isUser ? AiTheme.colors.userBorder : (isError ? "#FECACA" : (isSystem ? "#E5E7EB" : AiTheme.colors.assistantBorder))
                                implicitHeight: contentColumn.implicitHeight + 20

                                ToolButton {
                                    anchors.top: parent.top
                                    anchors.right: parent.right
                                    anchors.topMargin: 6
                                    anchors.rightMargin: 8
                                    z: 2
                                    visible: (hovered || copied) && copySourceText.length > 0
                                    text: copied ? "已复制" : "复制"
                                    onClicked: {
                                        root.copyTextToClipboard(copySourceText)
                                        copied = true
                                        copyFeedbackTimer.restart()
                                    }
                                }

                                Column {
                                    id: contentColumn
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 10
                                    spacing: 8

                                    Text {
                                        visible: isUser
                                        width: parent.width
                                        wrapMode: Text.Wrap
                                        text: fallbackText
                                        color: "#1F2937"
                                        font.pixelSize: 14
                                    }

                                    Loader {
                                        visible: !isUser && hasStructuredCard
                                        width: contentColumn.width
                                        sourceComponent: isPlanPreview ? planPreviewCardComponent : approvalRequestCardComponent
                                        property var cardMeta: meta
                                        property string cardText: fallbackText
                                        onLoaded: {
                                            if (!item) return
                                            item.cardMeta = cardMeta
                                            item.cardText = cardText
                                        }
                                    }

                                    Repeater {
                                        visible: !isUser && !hasStructuredCard && hasBlocks
                                        model: hasBlocks ? blocks : []
                                        delegate: Loader {
                                            width: contentColumn.width
                                            property var blockData: modelData
                                            asynchronous: true
                                            sourceComponent: root.blockType(blockData) === "code" ? codeBlockComponent : (root.blockType(blockData) === "heading" ? headingComponent : markdownTextComponent)
                                            onLoaded: if (item) item.blockData = blockData
                                        }
                                    }

                                    Text {
                                        visible: !isUser && !hasStructuredCard && !hasBlocks
                                        width: parent.width
                                        wrapMode: Text.Wrap
                                        text: fallbackText
                                        color: isError ? "#B91C1C" : (isSystem ? "#6B7280" : "#1F2937")
                                        font.pixelSize: 14
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.NoButton
                                    onEntered: hovered = true
                                    onExited: hovered = false
                                }
                            }

                            Component {
                                id: markdownTextComponent
                                Text {
                                    property var blockData: null
                                    width: contentColumn.width
                                    text: root.blockText(blockData)
                                    wrapMode: Text.Wrap
                                    textFormat: Text.MarkdownText
                                    color: isError ? "#B91C1C" : (isSystem ? "#6B7280" : "#1F2937")
                                    font.pixelSize: 14
                                    lineHeight: 1.34
                                }
                            }

                            Component {
                                id: headingComponent
                                Text {
                                    property var blockData: null
                                    width: contentColumn.width
                                    text: root.blockText(blockData)
                                    wrapMode: Text.Wrap
                                    color: "#111827"
                                    font.bold: true
                                    font.pixelSize: root.headingPixelSize(blockData && blockData.level ? blockData.level : 1)
                                }
                            }

                            Component {
                                id: codeBlockComponent
                                MarkdownCodeBlock {
                                    property var blockData: null
                                    width: contentColumn.width
                                    codeText: root.blockText(blockData)
                                    codeHtml: blockData && blockData.html ? blockData.html : ""
                                    language: blockData && blockData.language ? blockData.language : ""
                                }
                            }

                            Component {
                                id: planPreviewCardComponent
                                Column {
                                    property var cardMeta: ({})
                                    property string cardText: ""
                                    width: contentColumn.width
                                    spacing: 8

                                    Text {
                                        width: parent.width
                                        text: "执行计划预览"
                                        font.pixelSize: 13
                                        font.bold: true
                                        color: "#111827"
                                    }

                                    Text {
                                        width: parent.width
                                        wrapMode: Text.Wrap
                                        text: cardText
                                        font.pixelSize: 13
                                        color: "#374151"
                                    }

                                    Rectangle {
                                        width: parent.width
                                        height: 1
                                        color: "#E5E7EB"
                                    }

                                    Text {
                                        width: parent.width
                                        text: "计划ID: " + root.mapString(cardMeta, "planId", "-")
                                        font.pixelSize: 12
                                        color: "#6B7280"
                                    }

                                    Text {
                                        width: parent.width
                                        text: "风险等级: " + root.mapString(cardMeta, "riskLevel", "unknown")
                                        font.pixelSize: 12
                                        color: "#6B7280"
                                    }

                                    Column {
                                        width: parent.width
                                        spacing: 4

                                        Repeater {
                                            model: root.mapList(cardMeta, "steps")
                                            delegate: Text {
                                                width: parent.width
                                                wrapMode: Text.Wrap
                                                readonly property var stepData: modelData || ({})
                                                readonly property string stepTitle: root.mapString(stepData, "title", "")
                                                readonly property string stepId: root.mapString(stepData, "stepId", "")
                                                text: (index + 1) + ". " + (stepTitle.length > 0 ? stepTitle : stepId)
                                                font.pixelSize: 12
                                                color: "#374151"
                                            }
                                        }
                                    }
                                }
                            }

                            Component {
                                id: approvalRequestCardComponent
                                Column {
                                    property var cardMeta: ({})
                                    property string cardText: ""
                                    width: contentColumn.width
                                    spacing: 10

                                    Text {
                                        width: parent.width
                                        text: "执行确认"
                                        font.pixelSize: 13
                                        font.bold: true
                                        color: "#111827"
                                    }

                                    Text {
                                        width: parent.width
                                        wrapMode: Text.Wrap
                                        text: cardText
                                        font.pixelSize: 13
                                        color: "#374151"
                                    }

                                    Row {
                                        spacing: 8
                                        Button {
                                            text: "同意执行"
                                            enabled: agentChatVM && root.mapString(cardMeta, "planId", "").length > 0
                                            onClicked: {
                                                if (!agentChatVM) return
                                                const pid = root.mapString(cardMeta, "planId", "")
                                                if (pid.length === 0) return
                                                agentChatVM.sendApprovalResponse(pid, true, "用户确认执行")
                                            }
                                        }
                                        Button {
                                            text: "拒绝执行"
                                            enabled: agentChatVM && root.mapString(cardMeta, "planId", "").length > 0
                                            onClicked: {
                                                if (!agentChatVM) return
                                                const pid = root.mapString(cardMeta, "planId", "")
                                                if (pid.length === 0) return
                                                agentChatVM.sendApprovalResponse(pid, false, "用户拒绝执行")
                                            }
                                        }
                                    }
                                }
                            }

                            Row {
                                id: metaLine
                                x: bubble.x + 2
                                spacing: 8
                                Label { text: isUser ? "你" : (isAssistant ? "AI 助手" : (isSystem ? "系统" : "提示")); color: AiTheme.colors.textSecondary; font.pixelSize: 11 }
                                Label { text: root.formatTime(timestamp); color: AiTheme.colors.textSecondary; font.pixelSize: 11 }
                                Label { visible: status && status.length > 0; text: status; color: status === "error" ? AiTheme.colors.danger : AiTheme.colors.textSecondary; font.pixelSize: 11 }
                            }
                        }

                        Connections {
                            target: agentChatVM ? agentChatVM.messageModel : null
                            function onRowsInserted(parent, first, last) {
                                if (!root.followLatest || listView.moving) return
                                if (last < listView.count - 2) return
                                root.requestScrollToBottom()
                            }
                            function onDataChanged(topLeft, bottomRight, roles) {
                                if (!root.followLatest || listView.moving) return
                                if (!bottomRight || bottomRight.row < listView.count - 2) return
                                root.requestScrollToBottom()
                            }
                            function onModelReset() {
                                root.followLatest = true
                                root.requestScrollToBottom()
                            }
                        }
                    }

                    RoundButton {
                        visible: !root.followLatest
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 14
                        text: "到底部"
                        onClicked: {
                            root.followLatest = true
                            listView.positionViewAtEnd()
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 176
                    radius: AiTheme.radius.card
                    color: "#FFFFFF"
                    border.color: AiTheme.colors.border

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        Label {
                            Layout.fillWidth: true
                            visible: agentChatVM && agentChatVM.lastError && agentChatVM.lastError.length > 0
                            color: AiTheme.colors.danger
                            wrapMode: Text.Wrap
                            text: agentChatVM ? agentChatVM.lastError : ""
                        }

                        TextArea {
                            id: inputArea
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            wrapMode: TextArea.Wrap
                            color: "#111827"
                            placeholderText: "给云音乐 AI 助手发送消息（Enter 发送，Shift+Enter 换行）"
                            placeholderTextColor: "#9CA3AF"
                            background: Rectangle {
                                radius: 10
                                color: "#FFFFFF"
                                border.color: "#E5E7EB"
                            }
                            Keys.onPressed: function(event) {
                                if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && !(event.modifiers & Qt.ShiftModifier)) {
                                    event.accepted = true
                                    root.sendCurrentMessage()
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Button { text: "深度思考"; enabled: false }
                            Button { text: "联网搜索"; enabled: false }
                            Item { Layout.fillWidth: true }
                            Button {
                                id: sendBtn
                                text: "发送"
                                enabled: agentChatVM
                                         ? (agentChatVM.ready && agentChatVM.currentSessionId.length > 0)
                                         : false
                                onClicked: root.sendCurrentMessage()
                            }
                        }
                    }
                }
            }
        }
    }
}
