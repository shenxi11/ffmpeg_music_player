import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import "theme/Theme.js" as Theme

Rectangle {
    id: root
    width: 1200
    height: 700
    color: Theme.bgBase

    signal minimizeClicked()
    signal maximizeClicked()
    signal closeClicked()
    signal menuClicked()
    signal localMusicClicked()
    signal onlineMusicClicked()
    signal playHistoryClicked()
    signal favoriteMusicClicked()

    property bool isMaximized: false
    property string currentView: "local"
    property bool isUserLoggedIn: false

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.bgBase }
            GradientStop { position: 1.0; color: Theme.bgSoft }
        }
    }

    Rectangle {
        width: Math.max(260, parent.width * 0.34)
        height: width
        radius: width / 2
        x: -width * 0.32
        y: -width * 0.3
        color: Theme.accentSoft
    }

    Rectangle {
        width: Math.max(220, parent.width * 0.24)
        height: width
        radius: width / 2
        x: parent.width - width * 0.62
        y: parent.height - width * 0.52
        color: Theme.blueSoft
    }

    Rectangle {
        id: titleBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 70
        color: Theme.glassLight
        border.width: 1
        border.color: Theme.glassBorder

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 18
            anchors.rightMargin: 14
            spacing: 12

            Item {
                Layout.preferredWidth: 240
                Layout.fillHeight: true

                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 10

                    Rectangle {
                        width: 42
                        height: 42
                        radius: 14
                        color: Theme.accent

                        Rectangle {
                            width: 24
                            height: 24
                            radius: 12
                            anchors.centerIn: parent
                            color: "#ffffff"
                            opacity: 0.16
                        }

                        Text {
                            anchors.centerIn: parent
                            text: "\u266b"
                            color: "#ffffff"
                            font.pixelSize: 18
                            font.bold: true
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 1

                        Text {
                            text: "\u4e91\u97f3\u4e50"
                            color: Theme.textPrimary
                            font.family: "Microsoft YaHei UI"
                            font.pixelSize: 20
                            font.bold: true
                        }

                        Text {
                            text: "\u79c1\u57df\u6d41\u5a92\u4f53\u5ba2\u6237\u7aef"
                            color: Theme.textMuted
                            font.family: "Microsoft YaHei UI"
                            font.pixelSize: 12
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                id: menuBubble
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                radius: 20
                color: menuMouse.containsMouse ? Theme.glassHover : "transparent"
                border.width: 1
                border.color: Theme.glassBorder

                Behavior on color { ColorAnimation { duration: 120 } }

                Text {
                    anchors.centerIn: parent
                    text: "\u2261"
                    color: Theme.textPrimary
                    font.pixelSize: 18
                }

                MouseArea {
                    id: menuMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.menuClicked()
                }
            }

            Row {
                Layout.preferredWidth: 126
                Layout.preferredHeight: 40
                spacing: 6
                Layout.alignment: Qt.AlignVCenter

                Rectangle {
                    width: 36
                    height: 36
                    radius: 18
                    color: minBtn.containsMouse ? Theme.glassHover : "transparent"
                    border.width: 1
                    border.color: Theme.glassBorder
                    Text {
                        anchors.centerIn: parent
                        text: "\u2014"
                        color: Theme.textPrimary
                        font.pixelSize: 15
                    }
                    MouseArea {
                        id: minBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.minimizeClicked()
                    }
                }

                Rectangle {
                    width: 36
                    height: 36
                    radius: 18
                    color: maxBtn.containsMouse ? Theme.glassHover : "transparent"
                    border.width: 1
                    border.color: Theme.glassBorder
                    Text {
                        anchors.centerIn: parent
                        text: root.isMaximized ? "\u2750" : "\u25a1"
                        color: Theme.textPrimary
                        font.pixelSize: 12
                    }
                    MouseArea {
                        id: maxBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.maximizeClicked()
                    }
                }

                Rectangle {
                    width: 36
                    height: 36
                    radius: 18
                    color: closeBtn.containsMouse ? Theme.accent : "transparent"
                    border.width: 1
                    border.color: Theme.glassBorder
                    Text {
                        anchors.centerIn: parent
                        text: "\u2715"
                        color: closeBtn.containsMouse ? "#ffffff" : Theme.textPrimary
                        font.pixelSize: 12
                    }
                    MouseArea {
                        id: closeBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.closeClicked()
                    }
                }
            }
        }
    }

    Item {
        id: mainContent
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: controlBar.top

        Rectangle {
            id: leftPanel
            width: 236
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            color: Theme.glassStrong
            border.width: 1
            border.color: Theme.glassBorder

            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 20
                spacing: 8

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    height: 46
                    radius: 12
                    color: root.currentView === "local" ? Theme.accent : (localMusicBtn.containsMouse ? Theme.glassHover : "transparent")
                    border.width: root.currentView === "local" ? 0 : 1
                    border.color: Theme.glassBorder
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Row {
                        anchors.centerIn: parent
                        spacing: 10
                        Text { text: "\u266b"; font.pixelSize: 16; color: root.currentView === "local" ? "#ffffff" : Theme.textPrimary }
                        Text { text: "\u672c\u5730\u97f3\u4e50"; font.family: "Microsoft YaHei UI"; font.pixelSize: 15; color: root.currentView === "local" ? "#ffffff" : Theme.textPrimary }
                    }
                    MouseArea {
                        id: localMusicBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.currentView = "local"
                            root.localMusicClicked()
                        }
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    height: 46
                    radius: 12
                    color: root.currentView === "online" ? Theme.accent : (onlineMusicBtn.containsMouse ? Theme.glassHover : "transparent")
                    border.width: root.currentView === "online" ? 0 : 1
                    border.color: Theme.glassBorder
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Row {
                        anchors.centerIn: parent
                        spacing: 10
                        Text { text: "\u2601"; font.pixelSize: 16; color: root.currentView === "online" ? "#ffffff" : Theme.textPrimary }
                        Text { text: "\u5728\u7ebf\u97f3\u4e50"; font.family: "Microsoft YaHei UI"; font.pixelSize: 15; color: root.currentView === "online" ? "#ffffff" : Theme.textPrimary }
                    }
                    MouseArea {
                        id: onlineMusicBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.currentView = "online"
                            root.onlineMusicClicked()
                        }
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    height: 46
                    radius: 12
                    color: root.currentView === "history" ? Theme.accent : (historyBtn.containsMouse ? Theme.glassHover : "transparent")
                    border.width: root.currentView === "history" ? 0 : 1
                    border.color: Theme.glassBorder
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Row {
                        anchors.centerIn: parent
                        spacing: 10
                        Text { text: "\u23f2"; font.pixelSize: 16; color: root.currentView === "history" ? "#ffffff" : Theme.textPrimary }
                        Text { text: "\u6700\u8fd1\u64ad\u653e"; font.family: "Microsoft YaHei UI"; font.pixelSize: 15; color: root.currentView === "history" ? "#ffffff" : Theme.textPrimary }
                    }
                    MouseArea {
                        id: historyBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.currentView = "history"
                            root.playHistoryClicked()
                        }
                    }
                }

                Rectangle {
                    visible: root.isUserLoggedIn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    height: 46
                    radius: 12
                    color: root.currentView === "favorite" ? Theme.accent : (favoriteBtn.containsMouse ? Theme.glassHover : "transparent")
                    border.width: root.currentView === "favorite" ? 0 : 1
                    border.color: Theme.glassBorder
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Row {
                        anchors.centerIn: parent
                        spacing: 10
                        Text { text: "\u2665"; font.pixelSize: 16; color: root.currentView === "favorite" ? "#ffffff" : Theme.textPrimary }
                        Text { text: "\u6211\u559c\u6b22\u7684\u97f3\u4e50"; font.family: "Microsoft YaHei UI"; font.pixelSize: 15; color: root.currentView === "favorite" ? "#ffffff" : Theme.textPrimary }
                    }
                    MouseArea {
                        id: favoriteBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.currentView = "favorite"
                            root.favoriteMusicClicked()
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: controlBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 96
        color: Theme.glassStrong
        border.width: 1
        border.color: Theme.glassBorder

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Theme.divider
        }
    }

    function setMaximizedState(maximized) {
        root.isMaximized = maximized
    }

    function setCurrentView(view) {
        root.currentView = view
    }
}
