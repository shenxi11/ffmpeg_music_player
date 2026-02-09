import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// 本地和下载 - 多Tab界面
Rectangle {
    id: root
    color: "#f5f5f5"

    signal playMusic(string filename)
    signal deleteMusic(string filename)
    signal addToFavorite(string path, string title, string artist, string duration)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Tab栏
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
                    model: ["本地音乐", "下载音乐", "正在下载"]
                    
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

        // 分隔线
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#e0e0e0"
        }

        // Tab内容区
        StackLayout {
            id: stackLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // Tab 1: 本地音乐
            LocalMusicList {
                onPlayMusic: root.playMusic(filename)
                onDeleteMusic: root.deleteMusic(filename)
                onAddToFavorite: root.addToFavorite(path, title, artist, duration)
            }

            // Tab 2: 下载音乐
            DownloadedMusicList {
                onPlayMusic: root.playMusic(filename)
                onDeleteMusic: root.deleteMusic(filename)
                onAddToFavorite: root.addToFavorite(path, title, artist, duration)
            }

            // Tab 3: 正在下载
            DownloadingList {
            }
        }
    }

    // Tab控制器（隐藏）
    QtObject {
        id: tabBar
        property int currentIndex: 0
    }
}
