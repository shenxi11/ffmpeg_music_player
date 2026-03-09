import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import "../../theme/Theme.js" as Theme

Item {
    id: root
    width: 1000
    height: 72

    Component.onDestruction: {
        if (volumeWindowLoader.active) {
            volumeWindowLoader.active = false
        }
        if (playModePopupLoader.active) {
            playModePopupLoader.active = false
        }
    }

    // 进度与歌曲信息
    property int currentTime: 0
    property int maxTime: 0
    property bool sliderPressed: false
    property bool seekPending: false
    property int seekTargetTime: 0
    property double seekPendingSinceMs: 0
    property string songName: "\u6682\u65e0\u6b4c\u66f2"
    property string picPath: "qrc:/new/prefix1/icon/pian.png"

    // 播放控制属性
    property int playState: 0   // 0: Stop, 1: Play, 2: Pause
    property bool loopState: false
    property bool isUp: false
    property int volumeValue: 50
    property bool volumeVisible: false
    property bool mlistChecked: false
    property bool deskChecked: false
    property int playMode: 2    // 0: Sequential, 1: RepeatOne, 2: RepeatAll, 3: Shuffle
    property string playerIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/player/"

    // 样式与自适应
    property bool compact: width < 980
    property bool narrow: width < 860
    property int sidePadding: compact ? 10 : 14
    property int songBlockWidth: narrow ? 200 : (compact ? 250 : 290)
    property int controlGap: compact ? 12 : 16
    property int iconButtonSize: compact ? 32 : 34
    property int playButtonSize: compact ? 40 : 46

    property color barBackgroundColor: root.isUp ? "#CC151923" : Theme.bgBase
    property color barBorderColor: root.isUp ? "#33FFFFFF" : "#DDE3ED"
    property color secondaryTextColor: root.isUp ? "#D8DCE6" : Theme.textSecondary
    property color sliderTrackColor: root.isUp ? "#5E667A" : "#D8DEE8"

    property bool finishNotified: false

    signal seekTo(int seconds)
    signal sliderPressedSignal()
    signal sliderReleasedSignal()
    signal upClicked()
    signal playFinished()
    signal stop()
    signal nextSong()
    signal lastSong()
    signal volumeChanged(int value)
    signal mlistToggled(bool checked)
    signal playClicked()
    signal rePlay()
    signal deskToggled(bool checked)
    signal loopToggled(bool isLooping)

    function formatTime(seconds) {
        var safe = Math.max(0, Math.floor(seconds))
        var mins = Math.floor(safe / 60)
        var secs = safe % 60
        return mins + ":" + (secs < 10 ? "0" : "") + secs
    }

    function modeText() {
        if (root.playMode === 0) return "顺"
        if (root.playMode === 1) return "1"
        if (root.playMode === 2) return "环"
        return "随机"
    }

    function modeTip() {
        if (root.playMode === 0) return "顺序播放"
        if (root.playMode === 1) return "单曲循环"
        if (root.playMode === 2) return "列表循环"
        return "随机播放"
    }

    function modeIconSource(hovered) {
        if (root.playMode === 1) {
            return root.playerIconPrefix + (hovered ? "player_mode_single_hover.svg" : "player_mode_single_active.svg")
        }
        if (root.playMode === 3) {
            return root.playerIconPrefix + (hovered ? "player_mode_shuffle_hover.svg" : "player_mode_shuffle_active.svg")
        }
        if (root.playMode === 2) {
            return root.playerIconPrefix + "player_mode_repeat_active.svg"
        }
        return root.playerIconPrefix + ((hovered || root.isUp) ? "player_mode_repeat_hover.svg" : "player_mode_repeat_default.svg")
    }

    function setCurrentTime(seconds) {
        if (!sliderPressed) {
            if (seekPending) {
                var nowMs = Date.now()
                var drift = Math.abs(seconds - seekTargetTime)
                var reachedTarget = drift <= 1
                var backendTimelineDiff = drift >= 2
                var pendingTimeout = (nowMs - seekPendingSinceMs) >= 2500
                if (reachedTarget || backendTimelineDiff || pendingTimeout) {
                    seekPending = false
                } else {
                    root.currentTime = seekTargetTime
                    return
                }
            }

            if (seconds > root.maxTime) {
                var chunk = 30
                var target = Math.ceil(seconds / chunk) * chunk
                if (target <= root.maxTime) {
                    target = root.maxTime + chunk
                }
                root.maxTime = target
            }
            root.currentTime = seconds
        }
    }

    function setMaxTime(seconds) {
        root.maxTime = Math.max(0, Math.floor(seconds))
    }

    function setSeekPending(seconds) {
        root.seekTargetTime = Math.max(0, Math.floor(seconds))
        root.seekPendingSinceMs = Date.now()
        root.seekPending = true
        root.currentTime = root.seekTargetTime
        if (root.seekTargetTime > root.maxTime) {
            root.maxTime = root.seekTargetTime
        }
    }

    function clearSeekPending() {
        root.seekPending = false
    }

    function setSongName(name) {
        root.songName = name
    }

    function setPicPath(path) {
        root.picPath = path
    }

    function setVolumeValue(value) {
        root.volumeValue = Math.max(0, Math.min(100, Math.round(value)))
    }

    function setPlayState(state) {
        root.playState = state
        if (state === 0) {
            root.seekPending = false
        }
    }

    function getPlayState() {
        return root.playState
    }

    function setPlayMode(mode) {
        var normalized = Math.max(0, Math.min(3, Math.floor(mode)))
        if (root.playMode === normalized) {
            return
        }
        root.playMode = normalized
        var isLooping = (normalized === 1 || normalized === 2)
        root.loopState = isLooping
        root.loopToggled(isLooping)
    }

    function setLoopState(loop) {
        root.loopState = loop
        if (loop) {
            if (root.playMode === 0 || root.playMode === 3) {
                root.playMode = 2
            }
        } else if (root.playMode === 1 || root.playMode === 2) {
            root.playMode = 0
        }
    }

    function getLoopState() {
        return root.loopState
    }

    function setIsUp(up) {
        root.isUp = up
    }

    function handlePlayFinished() {
        if (root.loopState || root.playMode === 1) {
            root.rePlay()
        } else {
            root.currentTime = 0
            root.playState = 0
            root.stop()
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.isUp ? 12 : 0
        color: root.barBackgroundColor
        border.width: root.isUp ? 1 : 1
        border.color: root.barBorderColor

        Column {
            anchors.fill: parent
            anchors.leftMargin: root.sidePadding
            anchors.rightMargin: root.sidePadding
            spacing: 0

            Item {
                width: parent.width
                height: 22

                Row {
                    anchors.fill: parent
                    spacing: 8

                    Text {
                        width: 42
                        height: parent.height
                        text: root.formatTime(root.currentTime)
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignRight
                        font.pixelSize: 11
                        color: root.secondaryTextColor
                    }

                    Slider {
                        id: progressSlider
                        width: parent.width - 92
                        height: parent.height
                        from: 0
                        to: root.maxTime > 0 ? root.maxTime : 100
                        value: root.currentTime

                        onPressedChanged: {
                            root.sliderPressed = pressed
                            if (pressed) {
                                root.sliderPressedSignal()
                            } else {
                                var seekSeconds = Math.floor(value)
                                root.setSeekPending(seekSeconds)
                                root.seekTo(seekSeconds)
                                root.sliderReleasedSignal()
                            }
                        }

                        Connections {
                            target: root
                            function onCurrentTimeChanged() {
                                if (!progressSlider.pressed) {
                                    progressSlider.value = root.currentTime
                                }
                            }
                        }

                        background: Rectangle {
                            x: progressSlider.leftPadding
                            y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                            width: progressSlider.availableWidth
                            height: 4
                            radius: 2
                            color: root.sliderTrackColor
                            opacity: root.isUp ? 0.60 : 0.90

                            Rectangle {
                                width: progressSlider.visualPosition * parent.width
                                height: parent.height
                                radius: 2
                                color: Theme.accent
                            }
                        }

                        handle: Rectangle {
                            x: progressSlider.leftPadding + progressSlider.visualPosition * (progressSlider.availableWidth - width)
                            y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                            width: 12
                            height: 12
                            radius: 6
                            color: progressSlider.pressed ? Theme.accent : "#FFFFFF"
                            border.width: 2
                            border.color: Theme.accent
                        }
                    }

                    Text {
                        width: 42
                        height: parent.height
                        text: root.formatTime(root.maxTime)
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                        font.pixelSize: 11
                        color: root.secondaryTextColor
                    }
                }
            }

            Item {
                width: parent.width
                height: 50

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    Item {
                        Layout.preferredWidth: root.songBlockWidth
                        Layout.minimumWidth: root.songBlockWidth
                        Layout.fillHeight: true

                        Row {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 10

                            Rectangle {
                                width: 42
                                height: 42
                                radius: 6
                                border.width: 1
                                border.color: root.isUp ? "#44FFFFFF" : "#D7DCE8"
                                color: "transparent"
                                clip: true

                                Image {
                                    anchors.fill: parent
                                    source: root.picPath
                                    fillMode: Image.PreserveAspectCrop
                                    smooth: true
                                    asynchronous: true
                                    cache: true
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.upClicked()
                                }
                            }

                            Text {
                                width: root.songBlockWidth - 56
                                height: 42
                                text: root.songName
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: root.compact ? 13 : 14
                                font.weight: Font.Medium
                                color: root.isUp ? "#F8FAFF" : "#2F3440"
                                elide: Text.ElideRight
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Row {
                        spacing: root.controlGap
                        Layout.alignment: Qt.AlignVCenter

                        Rectangle {
                            id: modeButton
                            width: root.iconButtonSize
                            height: root.iconButtonSize
                            radius: width / 2
                            color: modeArea.containsMouse ? Theme.accentSoft : (root.isUp ? "#1FFFFFFF" : "transparent")
                            border.width: 1
                            border.color: (root.playMode === 1 || root.playMode === 2 || root.playMode === 3)
                                          ? Theme.accent
                                          : (root.isUp ? "#44FFFFFF" : "#D6DCE8")

                            Image {
                                anchors.centerIn: parent
                                width: 18
                                height: 18
                                source: root.modeIconSource(modeArea.containsMouse || root.isUp)
                                fillMode: Image.PreserveAspectFit
                            }

                            ToolTip.visible: modeArea.containsMouse
                            ToolTip.text: root.modeTip()

                            MouseArea {
                                id: modeArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (playModePopupLoader.active) {
                                        if (playModePopupLoader.item) {
                                            playModePopupLoader.item.ignoreNextFocusLoss = true
                                        }
                                        playModePopupLoader.active = false
                                    } else {
                                        playModePopupLoader.active = true
                                    }
                                }
                            }

                            Loader {
                                id: playModePopupLoader
                                active: false
                                sourceComponent: Component {
                                    PlayModePopup {
                                        currentMode: root.playMode
                                        Component.onCompleted: {
                                            var buttonPos = modeButton.mapToGlobal(0, 0)
                                            x = buttonPos.x - (width - modeButton.width) / 2
                                            y = buttonPos.y - height - 10
                                            show()
                                            requestActivate()
                                        }
                                        onModeChanged: {
                                            root.setPlayMode(mode)
                                        }
                                        onClosing: {
                                            playModePopupLoader.active = false
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            width: root.iconButtonSize
                            height: root.iconButtonSize
                            radius: width / 2
                            color: prevArea.containsMouse ? Theme.accentSoft : (root.isUp ? "#1FFFFFFF" : "transparent")
                            border.width: 1
                            border.color: root.isUp ? "#44FFFFFF" : "#D6DCE8"
                            Image {
                                anchors.centerIn: parent
                                width: 18
                                height: 18
                                source: (prevArea.containsMouse || root.isUp)
                                        ? root.playerIconPrefix + "player_btn_previous_hover.svg"
                                        : root.playerIconPrefix + "player_btn_previous_default.svg"
                                fillMode: Image.PreserveAspectFit
                            }
                            MouseArea {
                                id: prevArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.lastSong()
                            }
                        }

                        Rectangle {
                            width: root.playButtonSize
                            height: root.playButtonSize
                            radius: width / 2
                            color: playArea.containsMouse ? Theme.accentSoft : (root.isUp ? "#2AFFFFFF" : "transparent")
                            border.width: 1
                            border.color: Theme.accent

                            Image {
                                anchors.centerIn: parent
                                width: 20
                                height: 20
                                source: {
                                    if (root.playState === 1) {
                                        if (playArea.pressed) return root.playerIconPrefix + "player_btn_pause_pressed.svg"
                                        if (playArea.containsMouse || root.isUp) return root.playerIconPrefix + "player_btn_pause_hover.svg"
                                        return root.playerIconPrefix + "player_btn_pause_default.svg"
                                    }
                                    if (playArea.pressed) return root.playerIconPrefix + "player_btn_play_pressed.svg"
                                    if (playArea.containsMouse || root.isUp) return root.playerIconPrefix + "player_btn_play_hover.svg"
                                    return root.playerIconPrefix + "player_btn_play_default.svg"
                                }
                                fillMode: Image.PreserveAspectFit
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
                            width: root.iconButtonSize
                            height: root.iconButtonSize
                            radius: width / 2
                            color: nextArea.containsMouse ? Theme.accentSoft : (root.isUp ? "#1FFFFFFF" : "transparent")
                            border.width: 1
                            border.color: root.isUp ? "#44FFFFFF" : "#D6DCE8"
                            Image {
                                anchors.centerIn: parent
                                width: 18
                                height: 18
                                source: (nextArea.containsMouse || root.isUp)
                                        ? root.playerIconPrefix + "player_btn_next_hover.svg"
                                        : root.playerIconPrefix + "player_btn_next_default.svg"
                                fillMode: Image.PreserveAspectFit
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
                            id: volumeButton
                            width: root.iconButtonSize
                            height: root.iconButtonSize
                            radius: width / 2
                            color: volumeArea.containsMouse || volumeWindowLoader.active ? Theme.accentSoft : (root.isUp ? "#1FFFFFFF" : "transparent")
                            border.width: 1
                            border.color: root.isUp ? "#44FFFFFF" : "#D6DCE8"
                            Image {
                                anchors.centerIn: parent
                                width: 18
                                height: 18
                                source: {
                                    if (root.volumeValue <= 0) {
                                        return volumeArea.containsMouse || volumeWindowLoader.active || root.isUp
                                                ? root.playerIconPrefix + "player_btn_mute_active.svg"
                                                : root.playerIconPrefix + "player_btn_mute_default.svg"
                                    }
                                    if (volumeWindowLoader.active) {
                                        return root.playerIconPrefix + "player_btn_volume_active.svg"
                                    }
                                    if (volumeArea.containsMouse || root.isUp) {
                                        return root.playerIconPrefix + "player_btn_volume_hover.svg"
                                    }
                                    return root.playerIconPrefix + "player_btn_volume_default.svg"
                                }
                                fillMode: Image.PreserveAspectFit
                            }
                            MouseArea {
                                id: volumeArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (volumeWindowLoader.active) {
                                        if (volumeWindowLoader.item) {
                                            volumeWindowLoader.item.ignoreNextFocusLoss = true
                                        }
                                        volumeWindowLoader.active = false
                                        root.volumeVisible = false
                                    } else {
                                        volumeWindowLoader.active = true
                                        root.volumeVisible = true
                                    }
                                }
                            }

                            Loader {
                                id: volumeWindowLoader
                                active: false
                                sourceComponent: Component {
                                    VolumeSlider {
                                        volumeValue: root.volumeValue
                                        Component.onCompleted: {
                                            var buttonPos = volumeButton.mapToGlobal(0, 0)
                                            x = buttonPos.x - (width - volumeButton.width) / 2
                                            y = buttonPos.y - height - 10
                                            show()
                                            requestActivate()
                                        }
                                        onVolumeChanged: {
                                            root.volumeValue = value
                                            root.volumeChanged(value)
                                        }
                                        onClosing: {
                                            volumeWindowLoader.active = false
                                            root.volumeVisible = false
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Row {
                        spacing: root.compact ? 10 : 14
                        Layout.alignment: Qt.AlignVCenter

                        Rectangle {
                            width: root.iconButtonSize
                            height: root.iconButtonSize
                            radius: width / 2
                            color: deskArea.containsMouse ? Theme.accentSoft : (root.isUp ? "#1FFFFFFF" : "transparent")
                            border.width: 1
                            border.color: root.isUp ? "#44FFFFFF" : "#D6DCE8"
                            Image {
                                anchors.centerIn: parent
                                width: 18
                                height: 18
                                source: {
                                    if (root.deskChecked) {
                                        return root.playerIconPrefix + "player_btn_desktop_lyrics_active.svg"
                                    }
                                    if (deskArea.containsMouse || root.isUp) {
                                        return root.playerIconPrefix + "player_btn_desktop_lyrics_hover.svg"
                                    }
                                    return root.playerIconPrefix + "player_btn_desktop_lyrics_default.svg"
                                }
                                fillMode: Image.PreserveAspectFit
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
                            width: root.iconButtonSize
                            height: root.iconButtonSize
                            radius: width / 2
                            color: listArea.containsMouse ? Theme.accentSoft : (root.isUp ? "#1FFFFFFF" : "transparent")
                            border.width: 1
                            border.color: root.isUp ? "#44FFFFFF" : "#D6DCE8"
                            Image {
                                anchors.centerIn: parent
                                width: 18
                                height: 18
                                source: {
                                    if (root.mlistChecked) {
                                        return root.playerIconPrefix + "player_btn_playlist_active.svg"
                                    }
                                    if (listArea.containsMouse || root.isUp) {
                                        return root.playerIconPrefix + "player_btn_playlist_hover.svg"
                                    }
                                    return root.playerIconPrefix + "player_btn_playlist_default.svg"
                                }
                                fillMode: Image.PreserveAspectFit
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
                }
            }
        }
    }
}
