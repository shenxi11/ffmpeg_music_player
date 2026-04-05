import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../theme/AiChatTheme.js" as AiTheme

Rectangle {
    id: root
    property string codeText: ""
    property string codeHtml: ""
    property string language: ""
    property bool copied: false
    property bool expanded: true
    readonly property int lineCount: Math.max(1, (codeText || "").split("\n").length)
    readonly property int expandedHeight: Math.min(Math.max(lineCount * 20 + 20, 96), 420)
    readonly property int collapsedHeight: Math.min(Math.max(8 * 20 + 20, 160), expandedHeight)
    readonly property int bodyHeight: expanded ? expandedHeight : collapsedHeight
    readonly property string renderedCodeHtml: codeHtml && codeHtml.length > 0
                                            ? "<html><head><style>"
                                              + "body{margin:0;padding:0;background:transparent;}"
                                              + "pre{margin:0;white-space:pre;}"
                                              + "</style></head><body>" + codeHtml + "</body></html>"
                                            : ""

    radius: AiTheme.radius.code
    color: "#F8FAFC"
    border.color: "#CBD5E1"
    border.width: 1
    clip: true
    implicitHeight: header.height + body.height + 2
    height: implicitHeight

    onCodeTextChanged: expanded = lineCount <= 10
    Component.onCompleted: expanded = lineCount <= 10

    Timer {
        id: copyFeedbackTimer
        interval: 1200
        repeat: false
        onTriggered: root.copied = false
    }

    Rectangle {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 40
        color: "#E2E8F0"

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: "#CBD5E1"
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 8
            spacing: 8

            Label {
                text: root.language && root.language.length > 0 ? root.language : "code"
                color: "#334155"
                font.pixelSize: 12
                font.bold: true
            }
            Item { Layout.fillWidth: true }

            ToolButton {
                icon.source: "qrc:/qml/assets/ai/icons/download.svg"
                icon.width: 14
                icon.height: 14
                ToolTip.visible: hovered
                ToolTip.text: "\u4e0b\u8f7d\uff08\u9884\u7559\uff09"
            }
            ToolButton {
                text: root.copied ? "\u5df2\u590d\u5236" : "\u590d\u5236"
                onClicked: {
                    copyProxy.selectAll()
                    copyProxy.copy()
                    copyProxy.deselect()
                    root.copied = true
                    copyFeedbackTimer.restart()
                }
            }
            ToolButton {
                icon.source: "qrc:/qml/assets/ai/icons/run.svg"
                icon.width: 14
                icon.height: 14
                ToolTip.visible: hovered
                ToolTip.text: "\u8fd0\u884c\uff08\u9884\u7559\uff09"
            }
            ToolButton {
                visible: root.lineCount > 10
                text: root.expanded ? "\u6536\u8d77" : "\u5c55\u5f00"
                onClicked: {
                    root.expanded = !root.expanded
                    codeFlick.contentY = 0
                    codeFlick.contentX = 0
                }
            }
        }
    }

    Rectangle {
        id: body
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        height: root.bodyHeight + 12
        color: "#0F172A"
        clip: true

        Flickable {
            id: codeFlick
            anchors.fill: parent
            anchors.margins: 6
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: Math.max(width, codeTextItem.paintedWidth + 16)
            contentHeight: Math.max(height, codeTextItem.paintedHeight + 16)

            ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            Text {
                id: codeTextItem
                x: 8
                y: 8
                text: root.renderedCodeHtml.length > 0 ? root.renderedCodeHtml : root.codeText
                textFormat: root.renderedCodeHtml.length > 0 ? Text.RichText : Text.PlainText
                wrapMode: Text.NoWrap
                color: "#E2E8F0"
                font.family: "Consolas"
                font.pixelSize: 13
                renderType: Text.NativeRendering
            }
        }
    }

    TextEdit {
        id: copyProxy
        visible: false
        text: root.codeText
        textFormat: TextEdit.PlainText
    }
}
