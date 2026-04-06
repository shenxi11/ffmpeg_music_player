import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../theme/Theme.js" as Theme

Rectangle {
    id: root
    color: "transparent"
    property var availablePlaylists: []
    property var favoritePaths: []

    property int tabLeftMargin: 18
    property int tabSpacing: width >= 1280 ? 18 : 10
    property int tabButtonWidth: Math.max(120, Math.min(220, Math.floor((width - tabLeftMargin * 2 - tabSpacing * 2) / 3)))

    signal playMusic(string filename)
    signal deleteMusic(string filename)
    signal addToFavorite(string path, string title, string artist, string duration)
    signal songActionRequested(string action, var song)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            color: Theme.glassLight
            border.width: 1
            border.color: Theme.glassBorder

            Row {
                anchors.left: parent.left
                anchors.leftMargin: root.tabLeftMargin
                anchors.verticalCenter: parent.verticalCenter
                spacing: root.tabSpacing

                Repeater {
                    model: [
                        "\u672c\u5730\u97f3\u4e50",
                        "\u4e0b\u8f7d\u97f3\u4e50",
                        "\u6b63\u5728\u4e0b\u8f7d"
                    ]

                    Rectangle {
                        id: tabButton
                        width: root.tabButtonWidth
                        height: 40
                        radius: 12
                        color: tabBar.currentIndex === index
                               ? Theme.accent
                               : (tabArea.containsMouse ? Theme.glassHover : "transparent")
                        border.width: tabBar.currentIndex === index ? 0 : 1
                        border.color: Theme.glassBorder
                        Behavior on color { ColorAnimation { duration: 120 } }

                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            font.family: "Microsoft YaHei UI"
                            font.pixelSize: 14
                            font.bold: tabBar.currentIndex === index
                            color: tabBar.currentIndex === index ? "#ffffff" : Theme.textPrimary
                        }

                        MouseArea {
                            id: tabArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                tabBar.currentIndex = index
                                stackLayout.currentIndex = index
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.divider
        }

        StackLayout {
            id: stackLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            LocalMusicList {
                availablePlaylists: root.availablePlaylists
                favoritePaths: root.favoritePaths
                onPlayMusic: root.playMusic(filename)
                onDeleteMusic: root.deleteMusic(filename)
                onAddToFavorite: root.addToFavorite(path, title, artist, duration)
                onSongActionRequested: root.songActionRequested(action, song)
            }

            DownloadedMusicList {
                availablePlaylists: root.availablePlaylists
                favoritePaths: root.favoritePaths
                onPlayMusic: root.playMusic(filename)
                onDeleteMusic: root.deleteMusic(filename)
                onAddToFavorite: root.addToFavorite(path, title, artist, duration)
                onSongActionRequested: root.songActionRequested(action, song)
            }

            DownloadingList {
            }
        }
    }

    QtObject {
        id: tabBar
        property int currentIndex: 0
    }
}
