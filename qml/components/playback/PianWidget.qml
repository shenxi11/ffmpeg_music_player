import QtQuick 2.14
import QtQuick.Controls 2.14
import "../../theme/Theme.js" as Theme

Item {
    id: root
    width: 170
    height: 72

    signal upClicked()

    property string songName: "\u6682\u65e0\u6b4c\u66f2"
    property string picPath: "qrc:/qml/assets/ai/icons/default-music-cover.svg"

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: Theme.glassLight
        border.width: 1
        border.color: Theme.glassBorder

        Row {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 10

            Rectangle {
                width: 58
                height: 58
                radius: 6
                clip: true
                border.width: 1
                border.color: "#D8DFEA"
                color: "#E9ECF5"

                Image {
                    anchors.fill: parent
                    source: root.picPath
                    fillMode: Image.PreserveAspectCrop
                    smooth: true
                    asynchronous: true
                    cache: true
                }
            }

            Text {
                width: parent.width - 68
                height: parent.height
                text: root.songName
                wrapMode: Text.WordWrap
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                font.pixelSize: 12
                color: Theme.textPrimary
                elide: Text.ElideRight
                maximumLineCount: 2
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.upClicked()
        }
    }

    function setName(name) {
        root.songName = name
    }

    function setPicPath(path) {
        root.picPath = path
    }
}
