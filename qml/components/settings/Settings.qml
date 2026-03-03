import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtQuick.Dialogs 1.3

Rectangle {
    id: root
    color: "#F5F5F5"

    signal chooseDownloadPath()
    signal chooseAudioCachePath()
    signal chooseLogPath()
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
        width: parent.width
        height: 60
        color: "#FFFFFF"

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            Image {
                width: 32
                height: 32
                source: "qrc:/new/prefix1/icon/settings.png"
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: "\u8bbe\u7f6e"
                font.pixelSize: 20
                font.bold: true
                color: "#333333"
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: "#E0E0E0"
            anchors.bottom: parent.bottom
        }
    }

    Flickable {
        id: contentFlickable
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: buttonBar.top
        anchors.margins: 0
        contentHeight: contentColumn.height
        clip: true

        Column {
            id: contentColumn
            width: parent.width
            spacing: 0

            Rectangle {
                width: parent.width
                height: 50
                color: "transparent"

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 20
                    anchors.verticalCenter: parent.verticalCenter
                    text: "\u4e0b\u8f7d\u8bbe\u7f6e"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#2196F3"
                }
            }

            Rectangle {
                width: parent.width
                height: 80
                color: "#FFFFFF"

                Column {
                    anchors.fill: parent
                    anchors.leftMargin: 40
                    anchors.rightMargin: 40
                    anchors.topMargin: 15
                    anchors.bottomMargin: 15
                    spacing: 8

                    Text {
                        text: "\u6b4c\u66f2\u4e0b\u8f7d\u4fdd\u5b58\u8def\u5f84"
                        font.pixelSize: 14
                        color: "#333333"
                    }

                    Row {
                        width: parent.width
                        spacing: 10

                        Rectangle {
                            width: parent.width - chooseDirBtn.width - parent.spacing
                            height: 36
                            border.width: 1
                            border.color: "#CCCCCC"
                            radius: 4
                            color: "#F9F9F9"

                            Text {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                verticalAlignment: Text.AlignVCenter
                                text: root.downloadPath || "D:/Music"
                                font.pixelSize: 13
                                color: "#666666"
                                elide: Text.ElideMiddle
                            }
                        }

                        Button {
                            id: chooseDirBtn
                            width: 80
                            height: 36
                            text: "\u9009\u62e9..."

                            background: Rectangle {
                                color: parent.pressed ? "#1976D2" : (parent.hovered ? "#2196F3" : "#42A5F5")
                                radius: 4
                            }

                            contentItem: Text {
                                text: parent.text
                                color: "#FFFFFF"
                                font.pixelSize: 13
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            onClicked: root.chooseDownloadPath()
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 80
                color: "#FFFFFF"

                Column {
                    anchors.fill: parent
                    anchors.leftMargin: 40
                    anchors.rightMargin: 40
                    anchors.topMargin: 15
                    anchors.bottomMargin: 15
                    spacing: 8

                    Text {
                        text: "\u97f3\u9891\u672c\u5730\u7f13\u5b58\u76ee\u5f55"
                        font.pixelSize: 14
                        color: "#333333"
                    }

                    Row {
                        width: parent.width
                        spacing: 10

                        Rectangle {
                            width: parent.width - chooseAudioCacheBtn.width - parent.spacing
                            height: 36
                            border.width: 1
                            border.color: "#CCCCCC"
                            radius: 4
                            color: "#F9F9F9"

                            Text {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                verticalAlignment: Text.AlignVCenter
                                text: root.audioCachePath
                                font.pixelSize: 13
                                color: "#666666"
                                elide: Text.ElideMiddle
                            }
                        }

                        Button {
                            id: chooseAudioCacheBtn
                            width: 80
                            height: 36
                            text: "\u9009\u62e9..."

                            background: Rectangle {
                                color: parent.pressed ? "#1976D2" : (parent.hovered ? "#2196F3" : "#42A5F5")
                                radius: 4
                            }

                            contentItem: Text {
                                text: parent.text
                                color: "#FFFFFF"
                                font.pixelSize: 13
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            onClicked: root.chooseAudioCachePath()
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 80
                color: "#FFFFFF"

                Column {
                    anchors.fill: parent
                    anchors.leftMargin: 40
                    anchors.rightMargin: 40
                    anchors.topMargin: 15
                    anchors.bottomMargin: 15
                    spacing: 8

                    Text {
                        text: "\u6253\u5370\u65e5\u5fd7\u6587\u4ef6\u8def\u5f84"
                        font.pixelSize: 14
                        color: "#333333"
                    }

                    Row {
                        width: parent.width
                        spacing: 10

                        Rectangle {
                            width: parent.width - chooseLogBtn.width - parent.spacing
                            height: 36
                            border.width: 1
                            border.color: "#CCCCCC"
                            radius: 4
                            color: "#F9F9F9"

                            Text {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                verticalAlignment: Text.AlignVCenter
                                text: root.logPath || "\u6253\u5370\u65e5\u5fd7.txt"
                                font.pixelSize: 13
                                color: "#666666"
                                elide: Text.ElideMiddle
                            }
                        }

                        Button {
                            id: chooseLogBtn
                            width: 80
                            height: 36
                            text: "\u9009\u62e9..."

                            background: Rectangle {
                                color: parent.pressed ? "#1976D2" : (parent.hovered ? "#2196F3" : "#42A5F5")
                                radius: 4
                            }

                            contentItem: Text {
                                text: parent.text
                                color: "#FFFFFF"
                                font.pixelSize: 13
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            onClicked: root.chooseLogPath()
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: "#E0E0E0"
            }

            Rectangle {
                width: parent.width
                height: 120
                color: "#FFFFFF"

                Column {
                    anchors.fill: parent
                    anchors.leftMargin: 40
                    anchors.rightMargin: 40
                    anchors.topMargin: 15
                    anchors.bottomMargin: 15
                    spacing: 15

                    Text {
                        text: "\u4e0b\u8f7d\u9009\u9879"
                        font.pixelSize: 14
                        color: "#333333"
                    }

                    Row {
                        spacing: 10

                        CheckBox {
                            id: lyricsCheckBox
                            checked: root.downloadLyrics

                            indicator: Rectangle {
                                implicitWidth: 20
                                implicitHeight: 20
                                radius: 3
                                border.width: 2
                                border.color: parent.checked ? "#2196F3" : "#CCCCCC"
                                color: parent.checked ? "#2196F3" : "#FFFFFF"

                                Image {
                                    anchors.centerIn: parent
                                    width: 12
                                    height: 12
                                    source: "qrc:/new/prefix1/icon/check.png"
                                    visible: parent.parent.checked
                                }
                            }

                            onCheckedChanged: {
                                root.downloadLyrics = checked
                            }
                        }

                        Text {
                            text: "\u540c\u65f6\u4e0b\u8f7d\u6b4c\u8bcd\u6587\u4ef6"
                            font.pixelSize: 13
                            color: "#666666"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Row {
                        spacing: 10

                        CheckBox {
                            id: coverCheckBox
                            checked: root.downloadCover

                            indicator: Rectangle {
                                implicitWidth: 20
                                implicitHeight: 20
                                radius: 3
                                border.width: 2
                                border.color: parent.checked ? "#2196F3" : "#CCCCCC"
                                color: parent.checked ? "#2196F3" : "#FFFFFF"

                                Image {
                                    anchors.centerIn: parent
                                    width: 12
                                    height: 12
                                    source: "qrc:/new/prefix1/icon/check.png"
                                    visible: parent.parent.checked
                                }
                            }

                            onCheckedChanged: {
                                root.downloadCover = checked
                            }
                        }

                        Text {
                            text: "\u540c\u65f6\u4e0b\u8f7d\u4e13\u8f91\u5c01\u9762\u56fe\u7247"
                            font.pixelSize: 13
                            color: "#666666"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: "#E0E0E0"
            }

            Rectangle {
                width: parent.width
                height: 210
                color: "#FFFFFF"

                Column {
                    anchors.fill: parent
                    anchors.leftMargin: 40
                    anchors.rightMargin: 40
                    anchors.topMargin: 15
                    anchors.bottomMargin: 15
                    spacing: 10

                    Row {
                        width: parent.width
                        spacing: 10

                        Text {
                            text: "在线状态"
                            font.pixelSize: 14
                            color: "#333333"
                            verticalAlignment: Text.AlignVCenter
                        }

                        Rectangle {
                            width: 64
                            height: 24
                            radius: 12
                            color: root.presenceOnline ? "#E8F5E9" : "#FDECEA"
                            border.width: 1
                            border.color: root.presenceOnline ? "#66BB6A" : "#EF9A9A"

                            Text {
                                anchors.centerIn: parent
                                text: root.presenceOnline ? "在线" : "离线"
                                color: root.presenceOnline ? "#2E7D32" : "#C62828"
                                font.pixelSize: 12
                                font.bold: true
                            }
                        }

                        Item { width: 120; height: 1 }

                        Button {
                            width: 90
                            height: 30
                            text: "刷新状态"

                            background: Rectangle {
                                color: parent.pressed ? "#1976D2" : (parent.hovered ? "#2196F3" : "#42A5F5")
                                radius: 4
                            }

                            contentItem: Text {
                                text: parent.text
                                color: "#FFFFFF"
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            onClicked: root.refreshPresenceRequested()
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#EEF1F5"
                    }

                    Text {
                        text: "账号：" + (root.presenceAccount ? root.presenceAccount : "-")
                        font.pixelSize: 13
                        color: "#666666"
                    }
                    Text {
                        text: "会话 Token：" + (root.presenceSessionToken ? root.presenceSessionToken : "-")
                        font.pixelSize: 13
                        color: "#666666"
                        elide: Text.ElideMiddle
                        width: parent.width
                    }
                    Text {
                        text: "心跳间隔：" + root.presenceHeartbeatIntervalSec + " 秒"
                        font.pixelSize: 13
                        color: "#666666"
                    }
                    Text {
                        text: "在线 TTL：" + root.presenceOnlineTtlSec + " 秒"
                        font.pixelSize: 13
                        color: "#666666"
                    }
                    Text {
                        text: "TTL 剩余：" + root.presenceTtlRemainingSec + " 秒"
                        font.pixelSize: 13
                        color: "#666666"
                    }
                    Text {
                        text: "最近上报：" + (root.presenceLastSeen ? root.presenceLastSeen : "未上报")
                        font.pixelSize: 13
                        color: "#666666"
                    }
                    Text {
                        text: "状态说明：" + (root.presenceStatusMessage ? root.presenceStatusMessage : "-")
                        font.pixelSize: 13
                        color: "#2196F3"
                    }
                }
            }
        }
    }

    Rectangle {
        id: buttonBar
        width: parent.width
        height: 70
        color: "#FFFFFF"
        anchors.bottom: parent.bottom

        Rectangle {
            width: parent.width
            height: 1
            color: "#E0E0E0"
            anchors.top: parent.top
        }

        Row {
            anchors.centerIn: parent
            spacing: 20

            Button {
                width: 100
                height: 36
                text: "\u5173\u95ed"

                background: Rectangle {
                    color: parent.pressed ? "#E0E0E0" : (parent.hovered ? "#F0F0F0" : "#FFFFFF")
                    border.width: 1
                    border.color: "#CCCCCC"
                    radius: 4
                }

                contentItem: Text {
                    text: parent.text
                    color: "#666666"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

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
