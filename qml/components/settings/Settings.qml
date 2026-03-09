import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import "../../theme/Theme.js" as Theme

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

    property string downloadPath: ""
    property string audioCachePath: ""
    property string logPath: ""
    property bool downloadLyrics: true
    property bool downloadCover: false
    property string presenceAccount: ""
    property string presenceSessionToken: ""
    property bool presenceOnline: false
    property int presenceHeartbeatIntervalSec: 0
    property int presenceOnlineTtlSec: 0
    property int presenceTtlRemainingSec: 0
    property string presenceStatusMessage: ""
    property string presenceLastSeen: ""

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
}
