import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import "../../theme/Theme.js" as Theme
import "../../theme/PlayerStyle.js" as PlayerStyle

Rectangle {
    id: root
    color: "transparent"

    property int pagePadding: width >= 1200 ? 24 : (width >= 900 ? 18 : 12)
    property int sectionPadding: width >= 1200 ? 24 : (width >= 900 ? 20 : 14)
    property int chooseButtonWidth: width >= 1200 ? 94 : 82
    property int clearCacheButtonWidth: width >= 1200 ? 130 : 118
    property int refreshButtonWidth: width >= 1200 ? 104 : 92
    property int statusSpacerWidth: width >= 1200 ? 80 : 40
    property int bottomButtonWidth: width >= 1200 ? 112 : 96

    signal chooseDownloadPath()
    signal chooseAudioCachePath()
    signal chooseLogPath()
    signal clearLocalCacheRequested()
    signal refreshPresenceRequested()
    signal settingsClosed()
    signal returnToWelcomeRequested()

    property string downloadPath: ""
    property string audioCachePath: ""
    property string logPath: ""
    property bool downloadLyrics: true
    property bool downloadCover: false
    property int playerPageStyle: 0
    property string agentMode: "control"
    property string agentLocalModelPath: ""
    property string agentLocalModelBaseUrl: ""
    property string agentLocalModelName: ""
    property int agentLocalContextSize: 16384
    property int agentLocalThreadCount: 4
    property bool agentRemoteFallbackEnabled: false
    property string agentRemoteBaseUrl: ""
    property string agentRemoteModelName: ""
    property string serverHost: ""
    property int serverPort: 0
    property string presenceAccount: ""
    property string presenceSessionToken: ""
    property bool presenceOnline: false
    property int presenceHeartbeatIntervalSec: 0
    property int presenceOnlineTtlSec: 0
    property int presenceTtlRemainingSec: 0
    property string presenceStatusMessage: ""
    property string presenceLastSeen: ""
    property var playerStyleOptions: [
        { "id": 0, "title": "\u7ecf\u5178\u9ed1\u80f6", "subtitle": "\u4eae\u8272\u5361\u7247 + \u9ed1\u80f6\u5531\u7247 + \u53f3\u4fa7\u6b4c\u8bcd" },
        { "id": 1, "title": "\u7b80\u7ea6\u65b9\u5f62", "subtitle": "\u51b7\u8272\u6e10\u53d8 + \u65b9\u5f62\u5c01\u9762 + \u6d6e\u5c42\u6b4c\u8bcd" },
        { "id": 2, "title": "\u900f\u660e\u5f69\u80f6", "subtitle": "\u51b0\u900f\u80cc\u677f + \u84dd\u8272\u5f69\u80f6 + \u8f7b\u76c8\u8d28\u611f" },
        { "id": 3, "title": "\u7b80\u7ea6\u6b4c\u8bcd", "subtitle": "\u6df1\u8272\u821e\u53f0 + \u7eaf\u6b4c\u8bcd\u7126\u70b9 + \u6700\u5c11\u5e72\u6270" },
        { "id": 4, "title": "\u6b4c\u624b\u5199\u771f", "subtitle": "\u5168\u5c4f\u6d78\u6ca1 + \u5c01\u9762\u80cc\u666f + \u60c5\u7eea\u6c1b\u56f4" }
    ]

    Rectangle {
        id: titleBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: root.pagePadding
        anchors.rightMargin: root.pagePadding
        anchors.topMargin: root.pagePadding
        height: 60
        radius: 14
        color: Theme.glassLight
        border.width: 1
        border.color: Theme.glassBorder

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            Image {
                width: 24
                height: 24
                source: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/base/base_icon_settings_default.svg"
                fillMode: Image.PreserveAspectFit
            }

            Text {
                text: "\u64ad\u653e\u4e0e\u7f13\u5b58\u8bbe\u7f6e"
                font.pixelSize: 18
                font.weight: Font.DemiBold
                color: Theme.textPrimary
            }
        }
    }

    Flickable {
        id: contentFlickable
        anchors.top: titleBar.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: bottomBar.top
        anchors.leftMargin: root.pagePadding
        anchors.rightMargin: root.pagePadding
        clip: true
        contentHeight: contentColumn.height

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            width: 8
        }

        Column {
            id: contentColumn
            width: contentFlickable.width
            spacing: 10

            Rectangle {
                width: parent.width
                radius: 12
                color: Theme.bgCard
                border.width: 1
                border.color: Theme.glassBorder
                height: 198

                Column {
                    anchors.fill: parent
                    anchors.margins: root.sectionPadding
                    spacing: 10

                    Text {
                        text: "\u4e0b\u8f7d\u76ee\u5f55"
                        color: Theme.textPrimary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    Text {
                        text: "\u6b4c\u66f2\u4e0b\u8f7d\u4fdd\u5b58\u8def\u5f84"
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }

                    Row {
                        width: parent.width
                        spacing: 8

                        Rectangle {
                            width: parent.width - chooseDownloadBtn.width - parent.spacing
                            height: 34
                            radius: 8
                            color: "#F6F8FC"
                            border.width: 1
                            border.color: "#D8DFEB"

                            Text {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                verticalAlignment: Text.AlignVCenter
                                text: root.downloadPath || "D:/Music"
                                color: Theme.textSecondary
                                font.pixelSize: 12
                                elide: Text.ElideMiddle
                            }
                        }

                        Rectangle {
                            id: chooseDownloadBtn
                            width: root.chooseButtonWidth
                            height: 34
                            radius: 8
                            color: chooseDownloadArea.containsMouse ? Theme.accent : "transparent"
                            border.width: 1
                            border.color: Theme.accent

                            Text {
                                anchors.centerIn: parent
                                text: "\u9009\u62e9"
                                color: chooseDownloadArea.containsMouse ? "#FFFFFF" : Theme.accent
                                font.pixelSize: 12
                                font.weight: Font.Medium
                            }

                            MouseArea {
                                id: chooseDownloadArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.chooseDownloadPath()
                            }
                        }
                    }

                    Text {
                        text: "\u97f3\u9891\u672c\u5730\u7f13\u5b58\u76ee\u5f55"
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }

                    Row {
                        width: parent.width
                        spacing: 8

                        Rectangle {
                            width: parent.width - chooseAudioCacheBtn.width - parent.spacing
                            height: 34
                            radius: 8
                            color: "#F6F8FC"
                            border.width: 1
                            border.color: "#D8DFEB"

                            Text {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                verticalAlignment: Text.AlignVCenter
                                text: root.audioCachePath
                                color: Theme.textSecondary
                                font.pixelSize: 12
                                elide: Text.ElideMiddle
                            }
                        }

                        Rectangle {
                            id: chooseAudioCacheBtn
                            width: root.chooseButtonWidth
                            height: 34
                            radius: 8
                            color: chooseAudioCacheArea.containsMouse ? Theme.accent : "transparent"
                            border.width: 1
                            border.color: Theme.accent

                            Text {
                                anchors.centerIn: parent
                                text: "\u9009\u62e9"
                                color: chooseAudioCacheArea.containsMouse ? "#FFFFFF" : Theme.accent
                                font.pixelSize: 12
                                font.weight: Font.Medium
                            }

                            MouseArea {
                                id: chooseAudioCacheArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.chooseAudioCachePath()
                            }
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                radius: 12
                color: Theme.bgCard
                border.width: 1
                border.color: Theme.glassBorder
                visible: false
                height: 0

                Column {
                    anchors.fill: parent
                    anchors.margins: root.sectionPadding
                    spacing: 10

                    Text {
                        text: "Agent 控制设置"
                        color: Theme.textPrimary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: "control 为默认本地控制模式。assistant 只用于解释，不直接执行写操作。"
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }

                    Row {
                        spacing: 10

                        Rectangle {
                            width: 108
                            height: 34
                            radius: 17
                            color: root.agentMode === "control" ? Theme.accent : "#F6F8FC"
                            border.width: 1
                            border.color: root.agentMode === "control" ? Theme.accent : "#D8DFEB"

                            Text {
                                anchors.centerIn: parent
                                text: "控制模式"
                                color: root.agentMode === "control" ? "#FFFFFF" : Theme.textSecondary
                                font.pixelSize: 12
                                font.weight: Font.Medium
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.agentMode = "control"
                            }
                        }

                        Rectangle {
                            width: 108
                            height: 34
                            radius: 17
                            color: root.agentMode === "assistant" ? Theme.accent : "#F6F8FC"
                            border.width: 1
                            border.color: root.agentMode === "assistant" ? Theme.accent : "#D8DFEB"

                            Text {
                                anchors.centerIn: parent
                                text: "助手模式"
                                color: root.agentMode === "assistant" ? "#FFFFFF" : Theme.textSecondary
                                font.pixelSize: 12
                                font.weight: Font.Medium
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.agentMode = "assistant"
                            }
                        }
                    }

                    Text {
                        text: "本地模型文件路径"
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }

                    TextField {
                        width: parent.width
                        height: 36
                        text: root.agentLocalModelPath
                        placeholderText: "E:/models/llm/Qwen2.5-3B-Instruct/qwen2.5-3b-instruct-q4_k_m.gguf"
                        onEditingFinished: root.agentLocalModelPath = text.trim()
                    }

                    Text {
                        text: "本地模型名称"
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }

                    TextField {
                        width: parent.width
                        height: 36
                        text: root.agentLocalModelName
                        placeholderText: "Qwen2.5-3B-Instruct"
                        onEditingFinished: root.agentLocalModelName = text.trim()
                    }

                    Text {
                        text: "上下文大小"
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }

                    TextField {
                        width: parent.width
                        height: 36
                        text: String(root.agentLocalContextSize)
                        placeholderText: "16384"
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 2048; top: 131072 }
                        onEditingFinished: {
                            var parsed = parseInt(text)
                            if (isNaN(parsed)) {
                                parsed = 16384
                            }
                            root.agentLocalContextSize = parsed
                            text = String(root.agentLocalContextSize)
                        }
                    }

                    Text {
                        text: "推理线程数"
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }

                    TextField {
                        width: parent.width
                        height: 36
                        text: String(root.agentLocalThreadCount)
                        placeholderText: "4"
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 1; top: 128 }
                        onEditingFinished: {
                            var parsed = parseInt(text)
                            if (isNaN(parsed)) {
                                parsed = 4
                            }
                            root.agentLocalThreadCount = parsed
                            text = String(root.agentLocalThreadCount)
                        }
                    }

                    Text {
                        text: "兼容 HTTP Base URL（仅过渡期保留）"
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }

                    TextField {
                        width: parent.width
                        height: 36
                        text: root.agentLocalModelBaseUrl
                        placeholderText: "http://127.0.0.1:8081/v1"
                        onEditingFinished: root.agentLocalModelBaseUrl = text.trim()
                    }

                    Row {
                        spacing: 8

                        CheckBox {
                            checked: root.agentRemoteFallbackEnabled
                            onCheckedChanged: root.agentRemoteFallbackEnabled = checked
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "启用远程兜底（高级）"
                            color: Theme.textSecondary
                            font.pixelSize: 12
                        }
                    }

                    Column {
                        width: parent.width
                        spacing: 8
                        visible: root.agentRemoteFallbackEnabled

                        Text {
                            text: "远程 Base URL"
                            color: Theme.textSecondary
                            font.pixelSize: 12
                        }

                        TextField {
                            width: parent.width
                            height: 36
                            text: root.agentRemoteBaseUrl
                            placeholderText: "https://your-remote-openai-compatible-endpoint/v1"
                            onEditingFinished: root.agentRemoteBaseUrl = text.trim()
                        }

                        Text {
                            text: "远程模型名称"
                            color: Theme.textSecondary
                            font.pixelSize: 12
                        }

                        TextField {
                            width: parent.width
                            height: 36
                            text: root.agentRemoteModelName
                            placeholderText: "gpt-4.1-mini"
                            onEditingFinished: root.agentRemoteModelName = text.trim()
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                radius: 12
                color: Theme.bgCard
                border.width: 1
                border.color: Theme.glassBorder
                height: styleSectionContent.height + root.sectionPadding * 2

                Column {
                    id: styleSectionContent
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: root.sectionPadding
                    spacing: 10

                    Text {
                        text: "\u64ad\u653e\u9875\u6837\u5f0f"
                        color: Theme.textPrimary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    Text {
                        text: "\u53c2\u8003 QQ \u97f3\u4e50\u7684\u64ad\u653e\u9875\u601d\u8def\uff0c\u4e3a\u5f53\u524d\u9879\u76ee\u63d0\u4f9b 5 \u5957\u53ef\u5207\u6362\u7684\u64ad\u653e\u89c6\u89c9\u98ce\u683c\u3002"
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }

                    GridLayout {
                        width: parent.width
                        columns: width >= 1000 ? 3 : 2
                        rowSpacing: 12
                        columnSpacing: 12

                        Repeater {
                            model: root.playerStyleOptions

                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 176
                                radius: 14
                                clip: true
                                property var styleSpec: PlayerStyle.styleFor(modelData.id)
                                property bool selected: root.playerPageStyle === modelData.id
                                color: selected ? "#FFF7F7" : "#FFFFFF"
                                border.width: selected ? 2 : 1
                                border.color: selected ? Theme.accent : "#E6EAF1"

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    height: 102
                                    radius: parent.radius
                                    color: styleSpec.previewPrimary

                                    gradient: Gradient {
                                        GradientStop { position: 0.0; color: styleSpec.previewPrimary }
                                        GradientStop { position: 1.0; color: styleSpec.previewSecondary }
                                    }

                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 16
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: modelData.id === 3 ? 0 : 60
                                        height: modelData.id === 3 ? 0 : 60
                                        radius: modelData.id === 1 ? 10 : 30
                                        visible: modelData.id !== 3
                                        color: styleSpec.previewDiscColor
                                        border.width: modelData.id === 2 ? 1 : 0
                                        border.color: "#66FFFFFF"
                                        opacity: modelData.id === 2 ? 0.85 : 1.0

                                        Rectangle {
                                            anchors.centerIn: parent
                                            width: parent.width * 0.32
                                            height: width
                                            radius: width / 2
                                            color: modelData.id === 1 ? "#FFFFFF" : "#121212"
                                            opacity: 0.72
                                        }
                                    }

                                    Column {
                                        anchors.left: modelData.id === 3 ? parent.left : undefined
                                        anchors.right: parent.right
                                        anchors.leftMargin: modelData.id === 3 ? 18 : 96
                                        anchors.rightMargin: 16
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: 6

                                        Rectangle {
                                            width: Math.min(parent.width, modelData.id === 3 ? 150 : 110)
                                            height: 8
                                            radius: 4
                                            color: "#EFFFFFFF"
                                        }

                                        Rectangle {
                                            width: Math.min(parent.width, modelData.id === 3 ? 200 : 132)
                                            height: 8
                                            radius: 4
                                            color: "#C8FFFFFF"
                                        }

                                        Rectangle {
                                            width: Math.min(parent.width, modelData.id === 3 ? 170 : 96)
                                            height: 8
                                            radius: 4
                                            color: "#96FFFFFF"
                                        }
                                    }

                                    Rectangle {
                                        anchors.right: parent.right
                                        anchors.bottom: parent.bottom
                                        anchors.rightMargin: 12
                                        anchors.bottomMargin: 10
                                        width: 24
                                        height: 24
                                        radius: 12
                                        color: selected ? Theme.accent : "#99FFFFFF"

                                        Text {
                                            anchors.centerIn: parent
                                            text: selected ? "\u2713" : "\u270E"
                                            color: "#FFFFFF"
                                            font.pixelSize: 12
                                            font.weight: Font.Bold
                                        }
                                    }
                                }

                                Column {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.topMargin: 112
                                    anchors.leftMargin: 14
                                    anchors.rightMargin: 14
                                    spacing: 4

                                    Text {
                                        text: modelData.title
                                        color: Theme.textPrimary
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                    }

                                    Text {
                                        text: modelData.subtitle
                                        width: parent.width
                                        wrapMode: Text.WordWrap
                                        maximumLineCount: 2
                                        elide: Text.ElideRight
                                        color: Theme.textSecondary
                                        font.pixelSize: 11
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (root.playerPageStyle === modelData.id) {
                                            return
                                        }
                                        root.playerPageStyle = modelData.id
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                radius: 12
                color: Theme.bgCard
                border.width: 1
                border.color: Theme.glassBorder
                height: 114

                Column {
                    anchors.fill: parent
                    anchors.margins: root.sectionPadding
                    spacing: 10

                    Text {
                        text: "\u65e5\u5fd7\u8f93\u51fa"
                        color: Theme.textPrimary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    Row {
                        width: parent.width
                        spacing: 8

                        Rectangle {
                            width: parent.width - chooseLogBtn.width - parent.spacing
                            height: 34
                            radius: 8
                            color: "#F6F8FC"
                            border.width: 1
                            border.color: "#D8DFEB"

                            Text {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                verticalAlignment: Text.AlignVCenter
                                text: root.logPath || "\u6253\u5370\u65e5\u5fd7.txt"
                                color: Theme.textSecondary
                                font.pixelSize: 12
                                elide: Text.ElideMiddle
                            }
                        }

                        Rectangle {
                            id: chooseLogBtn
                            width: root.chooseButtonWidth
                            height: 34
                            radius: 8
                            color: chooseLogArea.containsMouse ? Theme.accent : "transparent"
                            border.width: 1
                            border.color: Theme.accent

                            Text {
                                anchors.centerIn: parent
                                text: "\u9009\u62e9"
                                color: chooseLogArea.containsMouse ? "#FFFFFF" : Theme.accent
                                font.pixelSize: 12
                                font.weight: Font.Medium
                            }

                            MouseArea {
                                id: chooseLogArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.chooseLogPath()
                            }
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                radius: 12
                color: Theme.bgCard
                border.width: 1
                border.color: Theme.glassBorder
                height: 112

                Row {
                    anchors.fill: parent
                    anchors.margins: root.sectionPadding
                    spacing: 10

                    Column {
                        width: parent.width - clearCacheBtn.width - parent.spacing
                        spacing: 6

                        Text {
                            text: "\u672c\u5730\u7f13\u5b58\u6e05\u7406"
                            color: Theme.textPrimary
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                        }

                        Text {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            text: "\u6e05\u7406\u97f3\u9891\u5206\u6bb5\u7f13\u5b58\u4e0e\u54cd\u5e94\u7f13\u5b58\uff0c\u4e0d\u5220\u9664\u5df2\u4e0b\u8f7d\u6587\u4ef6\u548c\u8d26\u53f7\u4fe1\u606f\u3002"
                            color: Theme.textSecondary
                            font.pixelSize: 12
                        }
                    }

                    Rectangle {
                        id: clearCacheBtn
                        width: root.clearCacheButtonWidth
                        height: 34
                        radius: 8
                        anchors.verticalCenter: parent.verticalCenter
                        color: clearCacheArea.containsMouse ? "#D83838" : "#E85A5A"

                        Text {
                            anchors.centerIn: parent
                            text: "\u6e05\u9664\u7f13\u5b58"
                            color: "#FFFFFF"
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }

                        MouseArea {
                            id: clearCacheArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.clearLocalCacheRequested()
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                radius: 12
                color: Theme.bgCard
                border.width: 1
                border.color: Theme.glassBorder
                height: 128

                Column {
                    anchors.fill: parent
                    anchors.margins: root.sectionPadding
                    spacing: 10

                    Text {
                        text: "\u4e0b\u8f7d\u9009\u9879"
                        color: Theme.textPrimary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    Row {
                        spacing: 8

                        CheckBox {
                            id: lyricsCheckBox
                            checked: root.downloadLyrics
                            onCheckedChanged: root.downloadLyrics = checked
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "\u540c\u65f6\u4e0b\u8f7d\u6b4c\u8bcd\u6587\u4ef6"
                            color: Theme.textSecondary
                            font.pixelSize: 12
                        }
                    }

                    Row {
                        spacing: 8

                        CheckBox {
                            id: coverCheckBox
                            checked: root.downloadCover
                            onCheckedChanged: root.downloadCover = checked
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "\u540c\u65f6\u4e0b\u8f7d\u4e13\u8f91\u5c01\u9762\u56fe\u7247"
                            color: Theme.textSecondary
                            font.pixelSize: 12
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                radius: 12
                color: Theme.bgCard
                border.width: 1
                border.color: Theme.glassBorder
                height: 236

                Column {
                    anchors.fill: parent
                    anchors.margins: root.sectionPadding
                    spacing: 8

                    Row {
                        width: parent.width
                        spacing: 8

                        Text {
                            text: "\u5728\u7ebf\u72b6\u6001"
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            color: Theme.textPrimary
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Rectangle {
                            width: 64
                            height: 24
                            radius: 12
                            anchors.verticalCenter: parent.verticalCenter
                            color: root.presenceOnline ? "#E8F6EC" : "#FDECEC"
                            border.width: 1
                            border.color: root.presenceOnline ? "#58AE6A" : "#D87070"

                            Text {
                                anchors.centerIn: parent
                                text: root.presenceOnline ? "\u5728\u7ebf" : "\u79bb\u7ebf"
                                color: root.presenceOnline ? "#2A7D3A" : "#B23434"
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                            }
                        }

                        Item {
                            width: root.statusSpacerWidth
                            height: 1
                        }

                        Rectangle {
                            width: root.refreshButtonWidth
                            height: 30
                            radius: 8
                            color: refreshPresenceArea.containsMouse ? Theme.accent : "transparent"
                            border.width: 1
                            border.color: Theme.accent

                            Text {
                                anchors.centerIn: parent
                                text: "\u5237\u65b0\u72b6\u6001"
                                color: refreshPresenceArea.containsMouse ? "#FFFFFF" : Theme.accent
                                font.pixelSize: 12
                                font.weight: Font.Medium
                            }

                            MouseArea {
                                id: refreshPresenceArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.refreshPresenceRequested()
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#E8EDF6"
                    }

                    Text {
                        width: parent.width
                        text: "\u670d\u52a1\u5668\uff1a" + (root.serverHost ? root.serverHost : "-") + (root.serverPort > 0 ? (":" + root.serverPort) : "")
                        color: Theme.textSecondary
                        font.pixelSize: 12
                        elide: Text.ElideMiddle
                    }
                    Text {
                        width: parent.width
                        text: "\u8d26\u53f7\uff1a" + (root.presenceAccount ? root.presenceAccount : "-")
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }
                    Text {
                        width: parent.width
                        text: "Token\uff1a" + (root.presenceSessionToken ? root.presenceSessionToken : "-")
                        color: Theme.textSecondary
                        font.pixelSize: 12
                        elide: Text.ElideMiddle
                    }
                    Text {
                        text: "\u5fc3\u8df3\u95f4\u9694\uff1a" + root.presenceHeartbeatIntervalSec + "s"
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }
                    Text {
                        text: "TTL\uff1a" + root.presenceOnlineTtlSec + "s  /  \u5269\u4f59\uff1a" + root.presenceTtlRemainingSec + "s"
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }
                    Text {
                        text: "\u6700\u8fd1\u4e0a\u62a5\uff1a" + (root.presenceLastSeen ? root.presenceLastSeen : "\u672a\u4e0a\u62a5")
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }
                    Text {
                        width: parent.width
                        text: "\u72b6\u6001\u8bf4\u660e\uff1a" + (root.presenceStatusMessage ? root.presenceStatusMessage : "-")
                        color: Theme.accent
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    Rectangle {
        id: bottomBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: root.pagePadding
        anchors.rightMargin: root.pagePadding
        anchors.bottomMargin: root.pagePadding
        height: 58
        radius: 12
        color: Theme.glassLight
        border.width: 1
        border.color: Theme.glassBorder

        Rectangle {
            id: returnWelcomeButtonWrap
            width: root.bottomButtonWidth + 26
            height: 34
            radius: 8
            anchors.right: closeButtonWrap.left
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            color: returnWelcomeArea.containsMouse ? Theme.accent : "transparent"
            border.width: 1
            border.color: Theme.accent

            Text {
                anchors.centerIn: parent
                text: "\u8fd4\u56de\u6b22\u8fce\u9875"
                color: returnWelcomeArea.containsMouse ? "#FFFFFF" : Theme.accent
                font.pixelSize: 12
                font.weight: Font.Medium
            }

            MouseArea {
                id: returnWelcomeArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.returnToWelcomeRequested()
            }
        }

        Rectangle {
            id: closeButtonWrap
            width: root.bottomButtonWidth
            height: 34
            radius: 8
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            color: closeArea.containsMouse ? Theme.accent : "transparent"
            border.width: 1
            border.color: Theme.accent

            Text {
                anchors.centerIn: parent
                text: "\u5173\u95ed"
                color: closeArea.containsMouse ? "#FFFFFF" : Theme.accent
                font.pixelSize: 12
                font.weight: Font.Medium
            }

            MouseArea {
                id: closeArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.settingsClosed()
            }
        }
    }

    function setDownloadPath(path) {
        root.downloadPath = path
    }

    function setDownloadLyrics(enable) {
        root.downloadLyrics = enable
    }

    function setDownloadCover(enable) {
        root.downloadCover = enable
    }

    function setAudioCachePath(path) {
        root.audioCachePath = path
    }

    function setLogPath(path) {
        root.logPath = path
    }

    function setPlayerPageStyle(styleId) {
        root.playerPageStyle = styleId
    }

    function setAgentMode(mode) {
        root.agentMode = mode || "control"
    }

    function setAgentLocalModelPath(modelPath) {
        root.agentLocalModelPath = modelPath || ""
    }

    function setAgentLocalModelBaseUrl(baseUrl) {
        root.agentLocalModelBaseUrl = baseUrl || ""
    }

    function setAgentLocalModelName(modelName) {
        root.agentLocalModelName = modelName || ""
    }

    function setAgentLocalContextSize(contextSize) {
        root.agentLocalContextSize = contextSize || 16384
    }

    function setAgentLocalThreadCount(threadCount) {
        root.agentLocalThreadCount = threadCount || 4
    }

    function setAgentRemoteFallbackEnabled(enabled) {
        root.agentRemoteFallbackEnabled = !!enabled
    }

    function setAgentRemoteBaseUrl(baseUrl) {
        root.agentRemoteBaseUrl = baseUrl || ""
    }

    function setAgentRemoteModelName(modelName) {
        root.agentRemoteModelName = modelName || ""
    }

    function setPresenceSnapshot(account, token, online, heartbeatIntervalSec, onlineTtlSec, ttlRemainingSec, statusMessage, lastSeenText) {
        root.presenceAccount = account || ""
        root.presenceSessionToken = token || ""
        root.presenceOnline = !!online
        root.presenceHeartbeatIntervalSec = heartbeatIntervalSec || 0
        root.presenceOnlineTtlSec = onlineTtlSec || 0
        root.presenceTtlRemainingSec = ttlRemainingSec || 0
        root.presenceStatusMessage = statusMessage || ""
        root.presenceLastSeen = lastSeenText || ""
    }

    function setServerEndpoint(host, port) {
        root.serverHost = host || ""
        root.serverPort = port || 0
    }
}
