import QtQuick 2.15
import "../../theme/Theme.js" as Theme

Item {
    id: root
    width: coverSize
    height: coverSize
    z: 2

    property string coverSource: ""
    property string fallbackSource: "qrc:/new/prefix1/icon/Music.png"
    property string playerIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/player/"
    property int coverSize: 44
    property int cornerRadius: 6
    property bool rowHovered: false
    property bool isCurrentTrack: false
    property bool isPlaying: false
    property bool interactionActive: coverHover.hovered || overlayArea.containsMouse

    readonly property string coverActionMode: {
        if (isCurrentTrack) {
            if (rowHovered) {
                return isPlaying ? "pause" : "play"
            }
            return "playing_indicator"
        }
        return rowHovered ? "play" : "none"
    }

    signal playRequested()
    signal pauseRequested()

    Rectangle {
        anchors.fill: parent
        radius: root.cornerRadius
        color: "#E9ECF5"
        border.width: 1
        border.color: "#D9DFEA"

        Image {
            id: coverImage
            anchors.fill: parent
            anchors.margins: 2
            source: root.coverSource && root.coverSource.length > 0 ? root.coverSource : root.fallbackSource
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: true
            sourceSize.width: root.coverSize
            sourceSize.height: root.coverSize

            onStatusChanged: {
                if (status === Image.Error && source !== root.fallbackSource) {
                    source = root.fallbackSource
                }
            }
        }
    }

    HoverHandler {
        id: coverHover
    }

    Rectangle {
        anchors.fill: parent
        radius: root.cornerRadius
        visible: root.coverActionMode === "play" || root.coverActionMode === "pause"
        color: "#66000000"

        Rectangle {
            width: 24
            height: 24
            radius: 12
            anchors.centerIn: parent
            color: "#FDFDFD"
            opacity: 0.96

            Image {
                anchors.centerIn: parent
                width: 16
                height: 16
                source: root.coverActionMode === "pause"
                        ? root.playerIconPrefix + "player_btn_pause_hover.svg"
                        : root.playerIconPrefix + "player_btn_play_hover.svg"
                fillMode: Image.PreserveAspectFit
            }
        }

        MouseArea {
            id: overlayArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (root.coverActionMode === "pause") {
                    root.pauseRequested()
                    return
                }
                root.playRequested()
            }
        }
    }

    Rectangle {
        width: 20
        height: 20
        radius: 10
        anchors.centerIn: parent
        visible: root.coverActionMode === "playing_indicator"
        color: "#66000000"

        Row {
            anchors.centerIn: parent
            spacing: 2

            Repeater {
                model: [
                    { "height": 7 },
                    { "height": 11 },
                    { "height": 8 }
                ]

                Rectangle {
                    width: 3
                    height: modelData.height
                    radius: 1.5
                    anchors.verticalCenter: parent.verticalCenter
                    color: Theme.accent
                }
            }
        }
    }
}
