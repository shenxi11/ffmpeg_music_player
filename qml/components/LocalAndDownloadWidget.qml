import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// 鏈湴鍜屼笅杞?- 澶歍ab鐣岄潰
Rectangle {
    id: root
    color: "#f5f5f5"

    signal playMusic(string filename)
    signal deleteMusic(string filename)
    signal addToFavorite(string path, string title, string artist, string duration)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Tab鏍?
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: "#ffffff"
            
            Row {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                spacing: 40

                Repeater {
                    model: ["\u672c\u5730\u97f3\u4e50", "\u4e0b\u8f7d\u97f3\u4e50", "\u6b63\u5728\u4e0b\u8f7d"]
                    
                    Rectangle {
                        width: 100
                        height: 40
                        color: tabBar.currentIndex === index ? "#409EFF" : "transparent"
                        radius: 4

                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            font.pixelSize: 16
                            font.bold: tabBar.currentIndex === index
                            color: tabBar.currentIndex === index ? "#ffffff" : "#333333"
                        }

                        MouseArea {
                            anchors.fill: parent
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

        // 鍒嗛殧绾?
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#e0e0e0"
        }

        // Tab鍐呭鍖?
        StackLayout {
            id: stackLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // Tab 1: 鏈湴闊充箰
            LocalMusicList {
                onPlayMusic: root.playMusic(filename)
                onDeleteMusic: root.deleteMusic(filename)
                onAddToFavorite: root.addToFavorite(path, title, artist, duration)
            }

            // Tab 2: 涓嬭浇闊充箰
            DownloadedMusicList {
                onPlayMusic: root.playMusic(filename)
                onDeleteMusic: root.deleteMusic(filename)
                onAddToFavorite: root.addToFavorite(path, title, artist, duration)
            }

            // Tab 3: 姝ｅ湪涓嬭浇
            DownloadingList {
            }
        }
    }

    // Tab鎺у埗鍣紙闅愯棌锛?
    QtObject {
        id: tabBar
        property int currentIndex: 0
    }
}

