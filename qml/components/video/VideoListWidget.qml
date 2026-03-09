import QtQuick 2.14
import QtQuick.Controls 2.14
import "../../theme/Theme.js" as Theme

Rectangle {
    id: root
    color: "transparent"

    property int pageMargin: width >= 1200 ? 20 : (width >= 900 ? 14 : 10)
    property int topBarHeight: width >= 1200 ? 62 : 56
    property int refreshButtonWidth: width >= 1200 ? 94 : 82
    property int refreshButtonHeight: width >= 1200 ? 34 : 32
    property int listSpacing: width >= 1200 ? 10 : 8
    property int itemHeight: width >= 1200 ? 84 : (width >= 900 ? 78 : 70)
    property int itemPadding: width >= 1200 ? 14 : 12
    property int iconSize: width >= 1200 ? 50 : (width >= 900 ? 46 : 40)

    // 对外暴露属性
    property int currentSelectedIndex: -1

    signal videoSelected(string videoPath, string videoName)
    signal refreshRequested()

    ListModel {
        id: videoListModel
    }

    Rectangle {
        id: topBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.pageMargin
        anchors.rightMargin: root.pageMargin
        anchors.top: parent.top
        height: root.topBarHeight
        radius: 12
        color: Theme.glassLight
        border.width: 1
        border.color: Theme.glassBorder

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            Text {
                text: "\u89c6\u9891\u5217\u8868"
                color: Theme.textPrimary
                font.pixelSize: 17
                font.weight: Font.DemiBold
            }
            Text {
                text: "(" + videoListModel.count + ")"
                color: Theme.textSecondary
                font.pixelSize: 13
            }
        }

        Rectangle {
            id: refreshButton
            width: root.refreshButtonWidth
            height: root.refreshButtonHeight
            radius: 16
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            color: refreshArea.containsMouse ? Theme.accent : "transparent"
            border.width: 1
            border.color: Theme.accent

            Text {
                anchors.centerIn: parent
                text: "\u5237\u65b0"
                color: refreshArea.containsMouse ? "#FFFFFF" : Theme.accent
                font.pixelSize: 13
                font.weight: Font.Medium
            }

            MouseArea {
                id: refreshArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.refreshRequested()
            }
        }
    }

    ListView {
        id: videoListView
        anchors.top: topBar.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: root.pageMargin
        anchors.rightMargin: root.pageMargin
        clip: true
        spacing: root.listSpacing
        model: videoListModel

        delegate: Rectangle {
            id: videoItem
            width: videoListView.width
            height: root.itemHeight
            radius: 10
            property bool hovered: mouseArea.containsMouse

            color: index === root.currentSelectedIndex
                   ? "#FDECEC"
                   : (hovered ? "#F8FAFF" : Theme.bgCard)
            border.width: 1
            border.color: index === root.currentSelectedIndex ? Theme.accent : Theme.glassBorder

            Row {
                anchors.fill: parent
                anchors.margins: root.itemPadding
                spacing: 12

                Rectangle {
                    width: root.iconSize
                    height: root.iconSize
                    radius: 8
                    anchors.verticalCenter: parent.verticalCenter
                    color: "#EEF2FA"
                    border.width: 1
                    border.color: "#D9DFEA"

                    Text {
                        anchors.centerIn: parent
                        text: "\u25b6"
                        color: Theme.accent
                        font.pixelSize: root.iconSize * 0.45
                        font.weight: Font.Bold
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4
                    width: parent.width - root.iconSize - 12

                    Text {
                        width: parent.width
                        text: model.name
                        elide: Text.ElideRight
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: Theme.textPrimary
                    }

                    Text {
                        width: parent.width
                        text: model.path
                        elide: Text.ElideMiddle
                        font.pixelSize: 12
                        color: Theme.textSecondary
                    }

                    Text {
                        text: formatFileSize(model.size)
                        font.pixelSize: 12
                        color: Theme.textSecondary
                    }
                }
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.currentSelectedIndex = index
                    root.videoSelected(model.path, model.name)
                }
            }
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            width: 8
        }

        Label {
            anchors.centerIn: parent
            visible: videoListModel.count === 0
            text: "\u6682\u65e0\u89c6\u9891"
            color: Theme.textSecondary
            font.pixelSize: 16
        }
    }

    function addVideo(name, path, size) {
        videoListModel.append({
            "name": name,
            "path": path,
            "size": size
        })
    }

    function clearAll() {
        videoListModel.clear()
        root.currentSelectedIndex = -1
    }

    function getCount() {
        return videoListModel.count
    }

    function formatFileSize(bytes) {
        if (bytes < 1024) {
            return bytes + " B"
        } else if (bytes < 1024 * 1024) {
            return (bytes / 1024).toFixed(2) + " KB"
        } else if (bytes < 1024 * 1024 * 1024) {
            return (bytes / (1024 * 1024)).toFixed(2) + " MB"
        }
        return (bytes / (1024 * 1024 * 1024)).toFixed(2) + " GB"
    }
}