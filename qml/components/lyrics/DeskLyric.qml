import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.14

Item {
    id: root
    width: 500
    height: 120

    property string lyricText: "\u2764 \u6682\u65e0\u6b4c\u8bcd \u2764"
    property string songName: "\u6682\u65e0\u6b4c\u66f2"
    property color lyricColor: "#ffffff"
    property int lyricFontSize: 18
    property string lyricFontFamily: "Microsoft YaHei"
    property bool hovered: false
    property bool isResizing: false
    property bool isPlaying: false

    readonly property int minWidth: 250
    readonly property int minHeight: 100
    readonly property int maxWidth: 1600
    readonly property int maxHeight: 400
    readonly property int autoWidthPadding: 130
    readonly property int autoWidthThreshold: 20

    signal playClicked(int state)
    signal nextClicked()
    signal lastClicked()
    signal forwardClicked()
    signal backwardClicked()
    signal settingsClicked()
    signal closeClicked()
    signal windowMoved(var newPos)
    signal windowResized(var newSize)
    signal lyricStyleChanged(var color, int fontSize, string fontFamily)

    TextMetrics {
        id: lyricMetrics
        text: root.lyricText
        font.family: root.lyricFontFamily
        font.pixelSize: root.lyricFontSize
        font.bold: true
    }

    TextMetrics {
        id: songMetrics
        text: root.songName
        font.family: root.lyricFontFamily
        font.pixelSize: Math.max(root.lyricFontSize - 8, 12)
        font.bold: true
    }

    Timer {
        id: autoResizeTimer
        interval: 80
        repeat: false
        onTriggered: root.recalcPreferredWidth()
    }

    onLyricTextChanged: scheduleAutoResize()
    onSongNameChanged: scheduleAutoResize()
    onLyricFontSizeChanged: scheduleAutoResize()
    onLyricFontFamilyChanged: scheduleAutoResize()

    Rectangle {
        id: lyricBackground
        anchors.fill: parent
        anchors.margins: 10
        anchors.topMargin: hovered ? 40 : 10

        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(0.99, 0.35, 0.42, 0.86) }
            GradientStop { position: 0.52; color: Qt.rgba(0.99, 0.50, 0.33, 0.80) }
            GradientStop { position: 1.0; color: Qt.rgba(0.98, 0.73, 0.38, 0.78) }
        }

        radius: 20
        border.width: 2
        border.color: Qt.rgba(1, 1, 1, 0.45)

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 19
            color: Qt.rgba(1, 1, 1, 0.12)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.25)
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 25
            spacing: 10

            Text {
                id: songNameLabel
                Layout.fillWidth: true
                text: root.songName || "\u6b63\u5728\u64ad\u653e..."
                color: Qt.lighter(root.lyricColor, 1.2)
                font.family: root.lyricFontFamily
                font.pixelSize: Math.max(root.lyricFontSize - 8, 12)
                font.bold: true
                style: Text.Outline
                styleColor: Qt.rgba(0, 0, 0, 0.35)
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                opacity: 0.9
            }

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: parent.width * 0.8
                height: 1
                color: Qt.rgba(1, 1, 1, 0.3)
                visible: root.songName !== ""
            }

            Text {
                id: lyricLabel
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: root.lyricText
                color: root.lyricColor
                font.family: root.lyricFontFamily
                font.pixelSize: root.lyricFontSize
                font.bold: true
                style: Text.Outline
                styleColor: Qt.rgba(0, 0, 0, 0.4)
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
                textFormat: Text.PlainText
            }
        }
    }

    DropShadow {
        anchors.fill: lyricBackground
        source: lyricBackground
        radius: 16
        samples: 33
        color: Qt.rgba(0, 0, 0, 0.6)
        horizontalOffset: 0
        verticalOffset: 4
    }

    Rectangle {
        id: controlBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 35
        visible: root.hovered
        opacity: root.hovered ? 1.0 : 0.0

        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.30) }
            GradientStop { position: 1.0; color: Qt.rgba(1, 1, 1, 0.18) }
        }

        radius: 15
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.5)

        Behavior on opacity {
            NumberAnimation { duration: 180 }
        }

        Row {
            anchors.centerIn: parent
            spacing: 8

            function iconButton(symbol, callback) {
                return 0
            }

            Button {
                width: 24; height: 24
                text: "\u23EE"
                font.pixelSize: 12
                onClicked: root.lastClicked()
                background: Rectangle {
                    color: parent.pressed ? Qt.rgba(1,1,1,0.46)
                                          : parent.hovered ? Qt.rgba(1,1,1,0.33)
                                                           : "transparent"
                    radius: 4
                }
                contentItem: Text { text: parent.text; color: "#ffffff"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }

            Button {
                width: 24; height: 24
                text: "\u23EA"
                font.pixelSize: 12
                onClicked: root.backwardClicked()
                background: Rectangle {
                    color: parent.pressed ? Qt.rgba(1,1,1,0.46)
                                          : parent.hovered ? Qt.rgba(1,1,1,0.33)
                                                           : "transparent"
                    radius: 4
                }
                contentItem: Text { text: parent.text; color: "#ffffff"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }

            Button {
                id: playButton
                width: 28; height: 28
                text: root.isPlaying ? "\u23F8" : "\u25B6"
                font.pixelSize: 14
                onClicked: {
                    root.isPlaying = !root.isPlaying
                    root.playClicked(root.isPlaying ? 1 : 0)
                }
                background: Rectangle {
                    color: parent.pressed ? Qt.rgba(1,1,1,0.46)
                                          : parent.hovered ? Qt.rgba(1,1,1,0.33)
                                                           : "transparent"
                    radius: 4
                }
                contentItem: Text { text: parent.text; color: "#ffffff"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }

            Button {
                width: 24; height: 24
                text: "\u23E9"
                font.pixelSize: 12
                onClicked: root.forwardClicked()
                background: Rectangle {
                    color: parent.pressed ? Qt.rgba(1,1,1,0.46)
                                          : parent.hovered ? Qt.rgba(1,1,1,0.33)
                                                           : "transparent"
                    radius: 4
                }
                contentItem: Text { text: parent.text; color: "#ffffff"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }

            Button {
                width: 24; height: 24
                text: "\u23ED"
                font.pixelSize: 12
                onClicked: root.nextClicked()
                background: Rectangle {
                    color: parent.pressed ? Qt.rgba(1,1,1,0.46)
                                          : parent.hovered ? Qt.rgba(1,1,1,0.33)
                                                           : "transparent"
                    radius: 4
                }
                contentItem: Text { text: parent.text; color: "#ffffff"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }

            Button {
                width: 24; height: 24
                text: "\u2699"
                font.pixelSize: 12
                onClicked: root.settingsClicked()
                background: Rectangle {
                    color: parent.pressed ? Qt.rgba(1,1,1,0.46)
                                          : parent.hovered ? Qt.rgba(1,1,1,0.33)
                                                           : "transparent"
                    radius: 4
                }
                contentItem: Text { text: parent.text; color: "#ffffff"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }

            Button {
                width: 24; height: 24
                text: "\u2715"
                font.pixelSize: 12
                onClicked: root.closeClicked()
                background: Rectangle {
                    color: parent.pressed ? Qt.rgba(1,0,0,0.60)
                                          : parent.hovered ? Qt.rgba(1,0,0,0.40)
                                                           : "transparent"
                    radius: 4
                }
                contentItem: Text { text: parent.text; color: "#ffffff"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        anchors.rightMargin: root.hovered ? 10 : 0
        anchors.bottomMargin: root.hovered ? 10 : 0
        hoverEnabled: true
        acceptedButtons: Qt.NoButton

        onEntered: root.hovered = true
        onExited: {
            if (!resizeMouseArea.containsMouse) {
                root.hovered = false
            }
        }
    }

    Rectangle {
        id: resizeHandle
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 10
        height: 10
        visible: root.hovered
        color: Qt.rgba(0.4, 0.49, 0.92, 0.2)
        border.width: 2
        border.color: Qt.rgba(0.4, 0.49, 0.92, 0.59)
    }

    MouseArea {
        id: resizeMouseArea
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 15
        height: 15
        visible: root.hovered

        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeFDiagCursor

        property point startPos
        property size startSize

        onEntered: root.hovered = true

        onPressed: {
            if (mouse.button === Qt.LeftButton) {
                root.isResizing = true
                startPos = Qt.point(mouse.x, mouse.y)
                startSize = Qt.size(root.width, root.height)
            }
        }

        onPositionChanged: {
            if (!root.isResizing) {
                return
            }
            var deltaX = mouse.x - startPos.x
            var deltaY = mouse.y - startPos.y

            var newWidth = Math.max(root.minWidth, Math.min(root.maxWidth, startSize.width + deltaX))
            var newHeight = Math.max(root.minHeight, Math.min(root.maxHeight, startSize.height + deltaY))

            root.windowResized(Qt.size(newWidth, newHeight))
        }

        onReleased: root.isResizing = false
    }

    function setLyricText(text) {
        root.lyricText = text
        scheduleAutoResize()
    }

    function setSongName(name) {
        root.songName = name
        scheduleAutoResize()
    }

    function setPlayButtonState(playing) {
        root.isPlaying = playing
    }

    function setLyricStyle(color, fontSize, fontFamily) {
        var oldColor = ("" + root.lyricColor)
        var nextColor = ("" + color)
        if (oldColor === nextColor &&
                root.lyricFontSize === fontSize &&
                root.lyricFontFamily === fontFamily) {
            return
        }
        root.lyricColor = color
        root.lyricFontSize = fontSize
        root.lyricFontFamily = fontFamily
        root.lyricStyleChanged(color, fontSize, fontFamily)
    }

    function showSettingsDialog() {
        settingsLoader.active = true
    }

    function scheduleAutoResize() {
        if (!autoResizeTimer.running) {
            autoResizeTimer.start()
        } else {
            autoResizeTimer.restart()
        }
    }

    function recalcPreferredWidth() {
        var contentWidth = Math.max(lyricMetrics.width, songMetrics.width)
        var targetWidth = Math.ceil(contentWidth + root.autoWidthPadding)
        targetWidth = Math.max(root.minWidth, Math.min(root.maxWidth, targetWidth))
        if (Math.abs(targetWidth - root.width) >= root.autoWidthThreshold) {
            root.windowResized(Qt.size(targetWidth, root.height))
        }
    }

    Loader {
        id: settingsLoader
        active: false
        source: "qrc:/qml/components/lyrics/DeskLyricSettings.qml"

        onLoaded: {
            if (!item) {
                return
            }
            item.currentColor = root.lyricColor
            item.currentFontSize = root.lyricFontSize
            item.currentFontFamily = root.lyricFontFamily

            item.settingsChanged.connect(function(color, fontSize, fontFamily) {
                root.setLyricStyle(color, fontSize, fontFamily)
            })

            item.dialogClosed.connect(function() {
                settingsLoader.active = false
            })

            item.show()
        }
    }

    Component.onCompleted: scheduleAutoResize()
}

