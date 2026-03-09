import QtQuick 2.14
import QtQuick.Window 2.14
import QtQuick.Controls 2.14
import QtGraphicalEffects 1.14
import "../../theme/Theme.js" as Theme

Window {
    id: volumeWindow
    width: 74
    height: 212
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"

    property int volumeValue: 50
    property bool suppressInitialSignal: true
    property bool ignoreNextFocusLoss: false

    signal volumeChanged(int value)

    Component.onCompleted: {
        Qt.callLater(function() {
            volumeWindow.suppressInitialSignal = false
        })
    }

    Connections {
        target: Qt.application
        function onAboutToQuit() {
            volumeWindow.close()
        }
    }

    onActiveChanged: {
        if (!active && !ignoreNextFocusLoss) {
            closeTimer.restart()
        }
        ignoreNextFocusLoss = false
    }

    Timer {
        id: closeTimer
        interval: 150
        onTriggered: {
            if (!volumeWindow.active) {
                volumeWindow.close()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 14
        color: "#DE10141C"
        border.width: 1
        border.color: "#44FFFFFF"

        layer.enabled: true
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: 4
            radius: 14
            samples: 21
            color: "#70000000"
        }

        Column {
            anchors.fill: parent
            anchors.topMargin: 10
            anchors.bottomMargin: 12
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: 8

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: Math.round(volumeSlider.value) + "%"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                color: "#FFFFFF"
            }

            Slider {
                id: volumeSlider
                width: parent.width
                height: parent.height - 28
                from: 0
                to: 100
                value: volumeWindow.volumeValue
                orientation: Qt.Vertical

                onValueChanged: {
                    volumeWindow.volumeValue = Math.round(value)
                    if (!volumeWindow.suppressInitialSignal) {
                        volumeWindow.volumeChanged(Math.round(value))
                    }
                }

                background: Rectangle {
                    x: volumeSlider.leftPadding + volumeSlider.availableWidth / 2 - width / 2
                    y: volumeSlider.topPadding
                    width: 6
                    height: volumeSlider.availableHeight
                    radius: 3
                    color: "#33FFFFFF"

                    Rectangle {
                        y: volumeSlider.visualPosition * parent.height
                        width: parent.width
                        height: parent.height - y
                        color: Theme.accent
                        radius: 3
                    }
                }

                handle: Rectangle {
                    x: volumeSlider.leftPadding + volumeSlider.availableWidth / 2 - width / 2
                    y: volumeSlider.topPadding + volumeSlider.visualPosition * (volumeSlider.availableHeight - height)
                    width: 16
                    height: 16
                    radius: 8
                    color: volumeSlider.pressed ? Theme.accent : "#FFFFFF"
                    border.color: Theme.accent
                    border.width: 2

                    scale: volumeSlider.pressed ? 1.22 : (volumeSlider.hovered ? 1.12 : 1.0)
                    Behavior on scale {
                        NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
                    }

                    layer.enabled: true
                    layer.effect: DropShadow {
                        horizontalOffset: 0
                        verticalOffset: 2
                        radius: 4
                        samples: 9
                        color: "#50000000"
                    }
                }
            }
        }
    }
}