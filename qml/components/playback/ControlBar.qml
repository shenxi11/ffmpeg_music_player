import QtQuick 2.14
import QtQuick.Controls 2.14
import "../../theme/Theme.js" as Theme

Item {
    id: root
    width: 320
    height: 74

    property int playState: 0    // 0: stop, 1: playing, 2: paused
    property bool loopState: false
    property bool isUp: false
    property int volumeValue: 50
    property bool volumeVisible: false
    property bool mlistChecked: false
    property bool deskChecked: false

    signal stop()
    signal nextSong()
    signal lastSong()
    signal volumeChanged(int value)
    signal mlistToggled(bool checked)
    signal playClicked()
    signal rePlay()
    signal deskToggled(bool checked)
    signal loopStateChanged(bool loop)

    Rectangle {
        anchors.fill: parent
        radius: 18
        color: Theme.glassLight
        border.width: 1
        border.color: Theme.glassBorder
    }

    Row {
        anchors.centerIn: parent
        spacing: 10

        Rectangle {
            width: 34
            height: 34
            radius: 17
            color: loopArea.containsMouse ? Theme.glassHover : "transparent"
            border.width: 1
            border.color: Theme.glassBorder
            Text {
                anchors.centerIn: parent
                text: root.loopState ? "\u27f3" : "\u2248"
                font.pixelSize: 16
                color: Theme.textPrimary
            }
            MouseArea {
                id: loopArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.loopState = !root.loopState
                    root.loopStateChanged(root.loopState)
                }
            }
        }

        Rectangle {
            width: 34
            height: 34
            radius: 17
            color: lastArea.containsMouse ? Theme.glassHover : "transparent"
            border.width: 1
            border.color: Theme.glassBorder
            Text {
                anchors.centerIn: parent
                text: "\u23ee"
                font.pixelSize: 15
                color: Theme.textPrimary
            }
            MouseArea {
                id: lastArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.lastSong()
            }
        }

        Rectangle {
            width: 42
            height: 42
            radius: 21
            color: playArea.containsMouse ? "#C8342C" : Theme.accent
            Text {
                anchors.centerIn: parent
                text: root.playState === 1 ? "\u23f8" : "\u25b6"
                font.pixelSize: 18
                color: "#ffffff"
            }
            MouseArea {
                id: playArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.playClicked()
            }
        }

        Rectangle {
            width: 34
            height: 34
            radius: 17
            color: nextArea.containsMouse ? Theme.glassHover : "transparent"
            border.width: 1
            border.color: Theme.glassBorder
            Text {
                anchors.centerIn: parent
                text: "\u23ed"
                font.pixelSize: 15
                color: Theme.textPrimary
            }
            MouseArea {
                id: nextArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.nextSong()
            }
        }

        Rectangle {
            width: 34
            height: 34
            radius: 17
            color: volumeArea.containsMouse ? Theme.glassHover : "transparent"
            border.width: 1
            border.color: Theme.glassBorder
            Text {
                anchors.centerIn: parent
                text: "\u266a"
                font.pixelSize: 15
                color: Theme.textPrimary
            }
            MouseArea {
                id: volumeArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.volumeVisible = !root.volumeVisible
            }

            Rectangle {
                id: volumeSliderBg
                width: 128
                height: 36
                visible: root.volumeVisible
                anchors.bottom: parent.top
                anchors.bottomMargin: 8
                anchors.horizontalCenter: parent.horizontalCenter
                radius: 12
                color: Theme.glassStrong
                border.width: 1
                border.color: Theme.glassBorder

                Slider {
                    id: volumeSlider
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    from: 0
                    to: 100
                    value: root.volumeValue
                    onValueChanged: {
                        root.volumeValue = Math.round(value)
                        root.volumeChanged(root.volumeValue)
                    }
                }
            }
        }

        Rectangle {
            width: 34
            height: 34
            radius: 17
            color: deskArea.containsMouse ? Theme.glassHover : "transparent"
            border.width: 1
            border.color: Theme.glassBorder
            Text {
                anchors.centerIn: parent
                text: "\u8bcd"
                font.pixelSize: 13
                font.bold: true
                color: root.deskChecked ? Theme.accent : Theme.textPrimary
            }
            MouseArea {
                id: deskArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.deskChecked = !root.deskChecked
                    root.deskToggled(root.deskChecked)
                }
            }
        }

        Rectangle {
            width: 34
            height: 34
            radius: 17
            color: listArea.containsMouse ? Theme.glassHover : "transparent"
            border.width: 1
            border.color: Theme.glassBorder
            Text {
                anchors.centerIn: parent
                text: "\u2630"
                font.pixelSize: 14
                color: root.mlistChecked ? Theme.accent : Theme.textPrimary
            }
            MouseArea {
                id: listArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.mlistChecked = !root.mlistChecked
                    root.mlistToggled(root.mlistChecked)
                }
            }
        }
    }

    function setPlayState(state) {
        root.playState = state
    }

    function getPlayState() {
        return root.playState
    }

    function setLoopState(loop) {
        root.loopState = loop
    }

    function getLoopState() {
        return root.loopState
    }

    function setIsUp(up) {
        root.isUp = up
    }

    function playFinished() {
        root.rePlay()
    }
}
