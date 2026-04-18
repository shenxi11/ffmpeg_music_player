import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtQuick.Dialogs 1.3

Rectangle {
    id: root
    color: "#f5f5f5"

    signal chooseDownloadPath()
    signal chooseAudioCachePath()
    signal chooseLogPath()
    signal clearLocalCacheRequested()
    signal refreshPresenceRequested()
    signal settingsClosed()
    signal returnToWelcomeRequested()

    readonly property color accentColor: "#1ece9b"
    readonly property color panelColor: "#fafafa"
    readonly property color lineColor: "#efefef"
    readonly property color borderColor: "#e5e7eb"
    readonly property color textColor: "#303133"
    readonly property color hintColor: "#909399"
    readonly property color inputColor: "#f7f7f7"

    property string activeTab: "general"
    property var tabs: [
        { "id": "general", "label": "常规设置" },
        { "id": "download", "label": "下载与缓存" },
        { "id": "lyrics", "label": "桌面歌词" },
        { "id": "shortcuts", "label": "快捷键" },
        { "id": "plugins", "label": "音效插件" },
        { "id": "network", "label": "网络设置" },
        { "id": "audio", "label": "音频设备" }
    ]

    property var generalStartupOptions: [
        { "key": "launchAtStartup", "label": "开机自动启动QQ音乐" },
        { "key": "autoPlayOnStartup", "label": "自动播放歌曲" },
        { "key": "autoOpenLyrics", "label": "自动打开歌词" },
        { "key": "disableDpiScale", "label": "禁用屏幕DPI适配" },
        { "key": "disableHardwareAcceleration", "label": "禁用硬件加速" },
        { "key": "preventSystemSleep", "label": "禁用系统休眠和息屏" }
    ]
    property var generalPlaybackOptions: [
        { "key": "listAutoSwitch", "label": "列表间自动切换" },
        { "key": "remoteControlComputerPlayback", "label": "开启手机遥控电脑播歌" },
        { "key": "autoBestVolume", "label": "自动调节到最佳音量" },
        { "key": "enableFadeInOut", "label": "开启音乐渐进渐出" },
        { "key": "enableMultimediaKeys", "label": "响应多媒体键盘" },
        { "key": "autoPlayMvWhenAvailable", "label": "无版权歌曲有MV资源时自动播放MV" }
    ]
    property var preferredQualityOptions: [
        { "value": "standard", "label": "标准品质" },
        { "value": "hq", "label": "HQ高品质" },
        { "value": "sq", "label": "SQ无损品质" },
        { "value": "hires", "label": "Hi-Res无损品质" },
        { "value": "surround", "label": "5.1声道" },
        { "value": "lossless2", "label": "臻品音质2.0" },
        { "value": "immersive", "label": "臻品全景声" },
        { "value": "master", "label": "臻品母带" },
        { "value": "dolby", "label": "杜比全景声" }
    ]
    property var singleTrackQueueOptions: [
        { "value": "full_list", "label": "清空当前播放队列，添加该歌曲所在列表全部歌曲进播放队列" },
        { "value": "single_only", "label": "仅添加该歌曲进播放队列" }
    ]
    property var notificationOptions: [
        { "key": "showMessageCenterBadge", "label": "显示消息中心数字红点" },
        { "key": "showSmtc", "label": "显示系统媒体传输控件 (SMTC)" }
    ]
    property var downloadFolderOptions: [
        { "value": "flat", "label": "不分文件夹" },
        { "value": "artist", "label": "按歌手分文件夹" },
        { "value": "album", "label": "按专辑分文件夹" }
    ]
    property var songNamingOptions: [
        { "value": "title", "label": "歌曲名" },
        { "value": "artist_title", "label": "歌手-歌曲名" },
        { "value": "title_artist", "label": "歌曲名-歌手" }
    ]
    property var lyricLineOptions: [
        { "value": "double", "label": "双行显示" },
        { "value": "single", "label": "单行显示" }
    ]
    property var lyricAlignmentOptions: [
        { "value": "center", "label": "居中对齐" },
        { "value": "left", "label": "左对齐" },
        { "value": "right", "label": "右对齐" },
        { "value": "split", "label": "左右分离" }
    ]
    property var lyricFontOptions: [
        "微软雅黑",
        "Microsoft YaHei",
        "SimHei",
        "SimSun",
        "KaiTi",
        "Arial",
        "Times New Roman"
    ]
    property var lyricFontSizeOptions: [
        "36", "42", "48", "57", "64"
    ]
    property var shortcutPlaybackRows: [
        { "label": "播放/暂停", "key": "shortcutPlayPause", "status": "正常" },
        { "label": "上一首", "key": "shortcutPreviousTrack", "status": "正常" },
        { "label": "下一首", "key": "shortcutNextTrack", "status": "正常" },
        { "label": "增大音量", "key": "shortcutVolumeUp", "status": "正常" },
        { "label": "减少音量", "key": "shortcutVolumeDown", "status": "正常" }
    ]
    property var shortcutOtherRows: [
        { "label": "添加喜欢", "key": "shortcutLikeSong", "status": "正常" },
        { "label": "听歌识曲", "key": "shortcutMusicRecognition", "status": "正常" },
        { "label": "显示/隐藏 音乐界面", "key": "shortcutToggleMainWindow", "status": "正常" },
        { "label": "显示/隐藏 桌面歌词", "key": "shortcutToggleDesktopLyrics", "status": "正常" }
    ]
    property var shortcutFunctionRows: [
        { "label": "快退", "key": "shortcutRewind", "status": "仅在客户端内生效" },
        { "label": "快进", "key": "shortcutFastForward", "status": "仅在客户端内生效" }
    ]
    property var proxyTypeOptions: [
        { "value": "none", "label": "不使用代理" },
        { "value": "http", "label": "HTTP 代理" },
        { "value": "https", "label": "HTTPS 代理" },
        { "value": "socks5", "label": "SOCKS5 代理" }
    ]
    property var audioOutputDeviceOptions: [ "DS: 主声音驱动程序", "WASAPI: 默认设备", "ASIO: 主设备" ]
    property var audioOutputFormatOptions: [ "原始比特", "16-bit", "24-bit", "32-bit float" ]
    property var audioFrequencyOptions: [ "原始频率", "44.1kHz", "48kHz", "96kHz" ]
    property var audioChannelOptions: [ "原始声道数", "立体声", "5.1 声道", "7.1 声道" ]
    property var audioDsdModeOptions: [
        { "value": "pcm_only", "label": "仅使用PCM模式" },
        { "value": "auto", "label": "自动选择" },
        { "value": "native", "label": "优先原生DSD" }
    ]
    property string pendingColorKey: ""

    function normalizePath(path) {
        return (path || "").toString().split("\\").join("/")
    }

    function openLocalPath(path) {
        var normalized = normalizePath(path)
        if (!normalized.length)
            return
        if (normalized.indexOf("file:///") === 0) {
            Qt.openUrlExternally(normalized)
            return
        }
        if (normalized.length >= 2 && normalized.charAt(1) === ":") {
            Qt.openUrlExternally("file:///" + normalized)
            return
        }
        Qt.openUrlExternally("file:///" + normalized)
    }

    function comboIndex(options, value) {
        for (var i = 0; i < options.length; ++i) {
            if (options[i] === value)
                return i
            if (typeof options[i] === "object" && options[i].value === value)
                return i
        }
        return 0
    }

    function boolSetting(key) {
        switch (key) {
        case "launchAtStartup": return settingsViewModel.launchAtStartup
        case "autoPlayOnStartup": return settingsViewModel.autoPlayOnStartup
        case "autoOpenLyrics": return settingsViewModel.autoOpenLyrics
        case "disableDpiScale": return settingsViewModel.disableDpiScale
        case "disableHardwareAcceleration": return settingsViewModel.disableHardwareAcceleration
        case "preventSystemSleep": return settingsViewModel.preventSystemSleep
        case "listAutoSwitch": return settingsViewModel.listAutoSwitch
        case "remoteControlComputerPlayback": return settingsViewModel.remoteControlComputerPlayback
        case "autoBestVolume": return settingsViewModel.autoBestVolume
        case "enableFadeInOut": return settingsViewModel.enableFadeInOut
        case "enableMultimediaKeys": return settingsViewModel.enableMultimediaKeys
        case "autoPlayMvWhenAvailable": return settingsViewModel.autoPlayMvWhenAvailable
        case "skipTrialTracks": return settingsViewModel.skipTrialTracks
        case "syncRecentPlaylistToCloud": return settingsViewModel.syncRecentPlaylistToCloud
        case "playbackAccelerationService": return settingsViewModel.playbackAccelerationService
        case "preferLocalDownloadedQuality": return settingsViewModel.preferLocalDownloadedQuality
        case "showMessageCenterBadge": return settingsViewModel.showMessageCenterBadge
        case "showSmtc": return settingsViewModel.showSmtc
        case "downloadLyrics": return settingsViewModel.downloadLyrics
        case "downloadCover": return settingsViewModel.downloadCover
        case "preferMp3Download": return settingsViewModel.preferMp3Download
        case "mp3TagApeV2": return settingsViewModel.mp3TagApeV2
        case "mp3TagId3v1": return settingsViewModel.mp3TagId3v1
        case "mp3TagId3v2": return settingsViewModel.mp3TagId3v2
        case "desktopLyricsShowOnStartup": return settingsViewModel.desktopLyricsShowOnStartup
        case "desktopLyricsAlwaysOnTop": return settingsViewModel.desktopLyricsAlwaysOnTop
        case "desktopLyricsCoverTaskbar": return settingsViewModel.desktopLyricsCoverTaskbar
        case "desktopLyricsBold": return settingsViewModel.desktopLyricsBold
        case "globalShortcutsEnabled": return settingsViewModel.globalShortcutsEnabled
        case "proxyEnabled": return settingsViewModel.proxyEnabled
        case "followSystemAudioDevice": return settingsViewModel.followSystemAudioDevice
        case "exclusiveAudioDeviceMode": return settingsViewModel.exclusiveAudioDeviceMode
        case "lowLatencyAudioMode": return settingsViewModel.lowLatencyAudioMode
        case "audioLocalFileMemoryMode": return settingsViewModel.audioLocalFileMemoryMode
        case "audioGaplessPlayback": return settingsViewModel.audioGaplessPlayback
        default: return false
        }
    }

    function setBoolSetting(key, value) {
        switch (key) {
        case "launchAtStartup": settingsViewModel.setLaunchAtStartup(value); break
        case "autoPlayOnStartup": settingsViewModel.setAutoPlayOnStartup(value); break
        case "autoOpenLyrics": settingsViewModel.setAutoOpenLyrics(value); break
        case "disableDpiScale": settingsViewModel.setDisableDpiScale(value); break
        case "disableHardwareAcceleration": settingsViewModel.setDisableHardwareAcceleration(value); break
        case "preventSystemSleep": settingsViewModel.setPreventSystemSleep(value); break
        case "listAutoSwitch": settingsViewModel.setListAutoSwitch(value); break
        case "remoteControlComputerPlayback": settingsViewModel.setRemoteControlComputerPlayback(value); break
        case "autoBestVolume": settingsViewModel.setAutoBestVolume(value); break
        case "enableFadeInOut": settingsViewModel.setEnableFadeInOut(value); break
        case "enableMultimediaKeys": settingsViewModel.setEnableMultimediaKeys(value); break
        case "autoPlayMvWhenAvailable": settingsViewModel.setAutoPlayMvWhenAvailable(value); break
        case "skipTrialTracks": settingsViewModel.setSkipTrialTracks(value); break
        case "syncRecentPlaylistToCloud": settingsViewModel.setSyncRecentPlaylistToCloud(value); break
        case "playbackAccelerationService": settingsViewModel.setPlaybackAccelerationService(value); break
        case "preferLocalDownloadedQuality": settingsViewModel.setPreferLocalDownloadedQuality(value); break
        case "showMessageCenterBadge": settingsViewModel.setShowMessageCenterBadge(value); break
        case "showSmtc": settingsViewModel.setShowSmtc(value); break
        case "downloadLyrics": settingsViewModel.setDownloadLyrics(value); break
        case "downloadCover": settingsViewModel.setDownloadCover(value); break
        case "preferMp3Download": settingsViewModel.setPreferMp3Download(value); break
        case "mp3TagApeV2": settingsViewModel.setMp3TagApeV2(value); break
        case "mp3TagId3v1": settingsViewModel.setMp3TagId3v1(value); break
        case "mp3TagId3v2": settingsViewModel.setMp3TagId3v2(value); break
        case "desktopLyricsShowOnStartup": settingsViewModel.setDesktopLyricsShowOnStartup(value); break
        case "desktopLyricsAlwaysOnTop": settingsViewModel.setDesktopLyricsAlwaysOnTop(value); break
        case "desktopLyricsCoverTaskbar": settingsViewModel.setDesktopLyricsCoverTaskbar(value); break
        case "desktopLyricsBold": settingsViewModel.setDesktopLyricsBold(value); break
        case "globalShortcutsEnabled": settingsViewModel.setGlobalShortcutsEnabled(value); break
        case "proxyEnabled": settingsViewModel.setProxyEnabled(value); break
        case "followSystemAudioDevice": settingsViewModel.setFollowSystemAudioDevice(value); break
        case "exclusiveAudioDeviceMode": settingsViewModel.setExclusiveAudioDeviceMode(value); break
        case "lowLatencyAudioMode": settingsViewModel.setLowLatencyAudioMode(value); break
        case "audioLocalFileMemoryMode": settingsViewModel.setAudioLocalFileMemoryMode(value); break
        case "audioGaplessPlayback": settingsViewModel.setAudioGaplessPlayback(value); break
        }
    }

    function stringSetting(key) {
        switch (key) {
        case "preferredPlaybackQuality": return settingsViewModel.preferredPlaybackQuality
        case "singleTrackQueueMode": return settingsViewModel.singleTrackQueueMode
        case "downloadFolderClassification": return settingsViewModel.downloadFolderClassification
        case "downloadSongNamingFormat": return settingsViewModel.downloadSongNamingFormat
        case "cacheSpaceMode": return settingsViewModel.cacheSpaceMode
        case "desktopLyricsLineMode": return settingsViewModel.desktopLyricsLineMode
        case "desktopLyricsAlignment": return settingsViewModel.desktopLyricsAlignment
        case "desktopLyricsColorPreset": return settingsViewModel.desktopLyricsColorPreset
        case "desktopLyricFontFamily": return settingsViewModel.desktopLyricFontFamily
        case "shortcutPlayPause": return settingsViewModel.shortcutPlayPause
        case "shortcutPreviousTrack": return settingsViewModel.shortcutPreviousTrack
        case "shortcutNextTrack": return settingsViewModel.shortcutNextTrack
        case "shortcutVolumeUp": return settingsViewModel.shortcutVolumeUp
        case "shortcutVolumeDown": return settingsViewModel.shortcutVolumeDown
        case "shortcutLikeSong": return settingsViewModel.shortcutLikeSong
        case "shortcutMusicRecognition": return settingsViewModel.shortcutMusicRecognition
        case "shortcutToggleMainWindow": return settingsViewModel.shortcutToggleMainWindow
        case "shortcutToggleDesktopLyrics": return settingsViewModel.shortcutToggleDesktopLyrics
        case "shortcutRewind": return settingsViewModel.shortcutRewind
        case "shortcutFastForward": return settingsViewModel.shortcutFastForward
        case "effectPluginType": return settingsViewModel.effectPluginType
        case "pluginDirectory": return settingsViewModel.pluginDirectory
        case "serverHost": return settingsViewModel.serverHost
        case "proxyType": return settingsViewModel.proxyType
        case "proxyHost": return settingsViewModel.proxyHost
        case "proxyUsername": return settingsViewModel.proxyUsername
        case "proxyPassword": return settingsViewModel.proxyPassword
        case "audioOutputDevice": return settingsViewModel.audioOutputDevice
        case "audioOutputFormat": return settingsViewModel.audioOutputFormat
        case "audioFrequencyConversion": return settingsViewModel.audioFrequencyConversion
        case "audioOutputChannels": return settingsViewModel.audioOutputChannels
        case "audioDsdMode": return settingsViewModel.audioDsdMode
        default: return ""
        }
    }

    function setStringSetting(key, value) {
        switch (key) {
        case "preferredPlaybackQuality": settingsViewModel.setPreferredPlaybackQuality(value); break
        case "singleTrackQueueMode": settingsViewModel.setSingleTrackQueueMode(value); break
        case "downloadFolderClassification": settingsViewModel.setDownloadFolderClassification(value); break
        case "downloadSongNamingFormat": settingsViewModel.setDownloadSongNamingFormat(value); break
        case "cacheSpaceMode": settingsViewModel.setCacheSpaceMode(value); break
        case "desktopLyricsLineMode": settingsViewModel.setDesktopLyricsLineMode(value); break
        case "desktopLyricsAlignment": settingsViewModel.setDesktopLyricsAlignment(value); break
        case "desktopLyricsColorPreset": settingsViewModel.setDesktopLyricsColorPreset(value); break
        case "desktopLyricFontFamily": settingsViewModel.setDesktopLyricFontFamily(value); break
        case "shortcutPlayPause": settingsViewModel.setShortcutPlayPause(value); break
        case "shortcutPreviousTrack": settingsViewModel.setShortcutPreviousTrack(value); break
        case "shortcutNextTrack": settingsViewModel.setShortcutNextTrack(value); break
        case "shortcutVolumeUp": settingsViewModel.setShortcutVolumeUp(value); break
        case "shortcutVolumeDown": settingsViewModel.setShortcutVolumeDown(value); break
        case "shortcutLikeSong": settingsViewModel.setShortcutLikeSong(value); break
        case "shortcutMusicRecognition": settingsViewModel.setShortcutMusicRecognition(value); break
        case "shortcutToggleMainWindow": settingsViewModel.setShortcutToggleMainWindow(value); break
        case "shortcutToggleDesktopLyrics": settingsViewModel.setShortcutToggleDesktopLyrics(value); break
        case "shortcutRewind": settingsViewModel.setShortcutRewind(value); break
        case "shortcutFastForward": settingsViewModel.setShortcutFastForward(value); break
        case "effectPluginType": settingsViewModel.setEffectPluginType(value); break
        case "pluginDirectory": settingsViewModel.setPluginDirectory(value); break
        case "serverHost": settingsViewModel.setServerHost(value); break
        case "proxyType": settingsViewModel.setProxyType(value); break
        case "proxyHost": settingsViewModel.setProxyHost(value); break
        case "proxyUsername": settingsViewModel.setProxyUsername(value); break
        case "proxyPassword": settingsViewModel.setProxyPassword(value); break
        case "audioOutputDevice": settingsViewModel.setAudioOutputDevice(value); break
        case "audioOutputFormat": settingsViewModel.setAudioOutputFormat(value); break
        case "audioFrequencyConversion": settingsViewModel.setAudioFrequencyConversion(value); break
        case "audioOutputChannels": settingsViewModel.setAudioOutputChannels(value); break
        case "audioDsdMode": settingsViewModel.setAudioDsdMode(value); break
        }
    }

    function intSetting(key) {
        switch (key) {
        case "desktopLyricFontSize": return settingsViewModel.desktopLyricFontSize
        case "serverPort": return settingsViewModel.serverPort
        case "proxyPort": return settingsViewModel.proxyPort
        case "outputSampleRate": return settingsViewModel.outputSampleRate
        case "cacheSpaceLimitMb": return settingsViewModel.cacheSpaceLimitMb
        case "desktopLyricsOpacity": return settingsViewModel.desktopLyricsOpacity
        case "audioCacheSizeMs": return settingsViewModel.audioCacheSizeMs
        default: return 0
        }
    }

    function setIntSetting(key, value) {
        switch (key) {
        case "desktopLyricFontSize": settingsViewModel.setDesktopLyricFontSize(value); break
        case "serverPort": settingsViewModel.setServerPort(value); break
        case "proxyPort": settingsViewModel.setProxyPort(value); break
        case "outputSampleRate": settingsViewModel.setOutputSampleRate(value); break
        case "cacheSpaceLimitMb": settingsViewModel.setCacheSpaceLimitMb(value); break
        case "desktopLyricsOpacity": settingsViewModel.setDesktopLyricsOpacity(value); break
        case "audioCacheSizeMs": settingsViewModel.setAudioCacheSizeMs(value); break
        }
    }

    function colorSetting(key) {
        switch (key) {
        case "desktopLyricColor": return settingsViewModel.desktopLyricColor
        case "desktopLyricsPlayedColor": return settingsViewModel.desktopLyricsPlayedColor
        case "desktopLyricsUnplayedColor": return settingsViewModel.desktopLyricsUnplayedColor
        case "desktopLyricsStrokeColor": return settingsViewModel.desktopLyricsStrokeColor
        default: return "#ffffff"
        }
    }

    function setColorSetting(key, value) {
        switch (key) {
        case "desktopLyricColor": settingsViewModel.setDesktopLyricColor(value); break
        case "desktopLyricsPlayedColor": settingsViewModel.setDesktopLyricsPlayedColor(value); break
        case "desktopLyricsUnplayedColor": settingsViewModel.setDesktopLyricsUnplayedColor(value); break
        case "desktopLyricsStrokeColor": settingsViewModel.setDesktopLyricsStrokeColor(value); break
        }
    }

    function openColorPicker(key) {
        pendingColorKey = key
        lyricColorDialog.color = colorSetting(key)
        lyricColorDialog.open()
    }

    ColorDialog {
        id: lyricColorDialog
        title: "选择颜色"
        onAccepted: {
            if (root.pendingColorKey.length) {
                root.setColorSetting(root.pendingColorKey, color)
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: root.color
    }

    Rectangle {
        id: shell
        width: Math.min(root.width - 48, 960)
        height: root.height - 32
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 16
        color: root.panelColor
        border.color: "#f0f0f0"
        border.width: 1
        radius: 10

        Column {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                width: parent.width
                height: 122
                color: "#f5f5f5"
                radius: 10

                Column {
                    anchors.fill: parent
                    anchors.leftMargin: 32
                    anchors.rightMargin: 32
                    anchors.topMargin: 28
                    spacing: 24

                    Text {
                        text: "设置"
                        color: root.textColor
                        font.pixelSize: 28
                        font.weight: Font.Bold
                    }

                    Row {
                        spacing: 26

                        Repeater {
                            model: root.tabs

                            delegate: Item {
                                width: tabLabel.implicitWidth
                                height: 28

                                Text {
                                    id: tabLabel
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top
                                    text: modelData.label
                                    color: root.activeTab === modelData.id ? root.accentColor : "#4b5563"
                                    font.pixelSize: 14
                                }

                                Rectangle {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.bottom: parent.bottom
                                    width: 24
                                    height: 2
                                    radius: 1
                                    color: root.accentColor
                                    visible: root.activeTab === modelData.id
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.activeTab = modelData.id
                                }
                            }
                        }
                    }
                }
            }

            Flickable {
                id: contentFlickable
                width: parent.width
                height: parent.height - 122
                contentWidth: width
                contentHeight: contentColumn.implicitHeight + 24
                clip: true

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    width: 8
                }

                Column {
                    id: contentColumn
                    width: contentFlickable.width - 64
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 18
                    spacing: 0

                    Loader {
                        width: parent.width
                        sourceComponent: {
                            switch (root.activeTab) {
                            case "download": return downloadPage
                            case "lyrics": return lyricsPage
                            case "shortcuts": return shortcutsPage
                            case "plugins": return pluginsPage
                            case "network": return networkPage
                            case "audio": return audioPage
                            default: return generalPage
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: generalPage

        Column {
            width: contentColumn.width

            Rectangle {
                width: parent.width
                height: 1
                color: root.lineColor
            }

            Column {
                width: parent.width
                spacing: 0

                Item {
                    width: parent.width
                    height: generalStartupGrid.implicitHeight + 48

                    Text {
                        x: 0
                        y: 28
                        width: 110
                        text: "启动"
                        color: "#6b7280"
                        font.pixelSize: 14
                    }

                    Grid {
                        id: generalStartupGrid
                        x: 110
                        y: 24
                        width: parent.width - 110
                        columns: 3
                        rowSpacing: 18
                        columnSpacing: 24

                        Repeater {
                            model: root.generalStartupOptions

                            delegate: Item {
                                width: (generalStartupGrid.width - 48) / 3
                                height: 24

                                Row {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 8

                                    Rectangle {
                                        width: 18
                                        height: 18
                                        radius: 9
                                        border.width: 1
                                        border.color: root.boolSetting(modelData.key) ? root.accentColor : "#cfcfcf"
                                        color: root.boolSetting(modelData.key) ? root.accentColor : "#ffffff"

                                        Text {
                                            anchors.centerIn: parent
                                            text: root.boolSetting(modelData.key) ? "✓" : ""
                                            color: "#ffffff"
                                            font.pixelSize: 11
                                            font.weight: Font.Bold
                                        }
                                    }

                                    Text {
                                        text: modelData.label
                                        color: root.textColor
                                        font.pixelSize: 14
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.setBoolSetting(modelData.key, !root.boolSetting(modelData.key))
                                }
                            }
                        }
                    }
                }

                Rectangle { width: parent.width; height: 1; color: root.lineColor }

                Item {
                    width: parent.width
                    height: generalPlaybackBlock.implicitHeight + 58

                    Text {
                        x: 0
                        y: 28
                        width: 110
                        text: "播放"
                        color: "#6b7280"
                        font.pixelSize: 14
                    }

                    Column {
                        id: generalPlaybackBlock
                        x: 110
                        y: 24
                        width: parent.width - 110
                        spacing: 18

                        Grid {
                            width: parent.width
                            columns: 3
                            rowSpacing: 18
                            columnSpacing: 24

                            Repeater {
                                model: root.generalPlaybackOptions

                                delegate: Item {
                                    width: (parent.width - 48) / 3
                                    height: 24

                                    Row {
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: 8

                                        Rectangle {
                                            width: 18
                                            height: 18
                                            radius: 9
                                            border.width: 1
                                            border.color: root.boolSetting(modelData.key) ? root.accentColor : "#cfcfcf"
                                            color: root.boolSetting(modelData.key) ? root.accentColor : "#ffffff"

                                            Text {
                                                anchors.centerIn: parent
                                                text: root.boolSetting(modelData.key) ? "✓" : ""
                                                color: "#ffffff"
                                                font.pixelSize: 11
                                                font.weight: Font.Bold
                                            }
                                        }

                                        Text {
                                            text: modelData.label
                                            color: root.textColor
                                            font.pixelSize: 14
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.setBoolSetting(modelData.key, !root.boolSetting(modelData.key))
                                    }
                                }
                            }
                        }

                        Item {
                            width: parent.width
                            height: 24

                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 8

                                Rectangle {
                                    width: 18
                                    height: 18
                                    radius: 9
                                    border.width: 1
                                    border.color: root.boolSetting("skipTrialTracks") ? root.accentColor : "#cfcfcf"
                                    color: root.boolSetting("skipTrialTracks") ? root.accentColor : "#ffffff"

                                    Text {
                                        anchors.centerIn: parent
                                        text: root.boolSetting("skipTrialTracks") ? "✓" : ""
                                        color: "#ffffff"
                                        font.pixelSize: 11
                                        font.weight: Font.Bold
                                    }
                                }

                                Text {
                                    text: "播放时自动跳过试听歌曲"
                                    color: root.textColor
                                    font.pixelSize: 14
                                }

                                Text {
                                    text: "(该功能仅适用于「我喜欢」等部分场景)"
                                    color: root.hintColor
                                    font.pixelSize: 12
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.setBoolSetting("skipTrialTracks", !root.boolSetting("skipTrialTracks"))
                            }
                        }

                        Flow {
                            id: syncOptionsFlow
                            width: parent.width
                            spacing: 24

                            Repeater {
                                model: [
                                    { "key": "syncRecentPlaylistToCloud", "label": "最近播放列表同步云端" },
                                    { "key": "playbackAccelerationService", "label": "使用播放加速服务", "subLabel": "(重启后生效)" }
                                ]

                                delegate: Item {
                                    width: syncOptionsFlow.width >= 560
                                           ? (syncOptionsFlow.width - syncOptionsFlow.spacing) / 2
                                           : syncOptionsFlow.width
                                    height: syncOptionColumn.implicitHeight

                                    Column {
                                        id: syncOptionColumn
                                        width: parent.width
                                        spacing: modelData.subLabel ? 4 : 0

                                        Row {
                                            spacing: 8

                                            Rectangle {
                                                width: 18
                                                height: 18
                                                radius: 9
                                                border.width: 1
                                                border.color: root.boolSetting(modelData.key) ? root.accentColor : "#cfcfcf"
                                                color: root.boolSetting(modelData.key) ? root.accentColor : "#ffffff"

                                                Text {
                                                    anchors.centerIn: parent
                                                    text: root.boolSetting(modelData.key) ? "✓" : ""
                                                    color: "#ffffff"
                                                    font.pixelSize: 11
                                                    font.weight: Font.Bold
                                                }
                                            }

                                            Text {
                                                width: syncOptionColumn.width - 26
                                                text: modelData.label
                                                color: root.textColor
                                                font.pixelSize: 14
                                                wrapMode: Text.WordWrap
                                            }
                                        }

                                        Text {
                                            visible: !!modelData.subLabel
                                            text: modelData.subLabel || ""
                                            color: root.hintColor
                                            font.pixelSize: 12
                                            leftPadding: 26
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.setBoolSetting(modelData.key, !root.boolSetting(modelData.key))
                                    }
                                }
                            }
                        }

                        Column {
                            spacing: 12

                            Item {
                                width: parent.width
                                height: 24

                                Row {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 8

                                    Rectangle {
                                        width: 18
                                        height: 18
                                        radius: 9
                                        border.width: 1
                                        border.color: root.boolSetting("preferLocalDownloadedQuality") ? root.accentColor : "#cfcfcf"
                                        color: root.boolSetting("preferLocalDownloadedQuality") ? root.accentColor : "#ffffff"

                                        Text {
                                            anchors.centerIn: parent
                                            text: root.boolSetting("preferLocalDownloadedQuality") ? "✓" : ""
                                            color: "#ffffff"
                                            font.pixelSize: 11
                                            font.weight: Font.Bold
                                        }
                                    }

                                    Text {
                                        text: "优先播放本地下载品质"
                                        color: root.textColor
                                        font.pixelSize: 14
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.setBoolSetting("preferLocalDownloadedQuality", !root.boolSetting("preferLocalDownloadedQuality"))
                                }
                            }

                            Text {
                                text: "开启后，网络环境下播放已下载歌曲时，将优先选择本地品质播放"
                                color: "#9ca3af"
                                font.pixelSize: 12
                                leftPadding: 26
                            }
                        }

                        Column {
                            spacing: 12

                            Text {
                                text: "优先播放品质："
                                color: root.textColor
                                font.pixelSize: 14
                            }

                            Grid {
                                columns: 3
                                rowSpacing: 14
                                columnSpacing: 26

                                Repeater {
                                    model: root.preferredQualityOptions

                                    delegate: Item {
                                        width: 210
                                        height: 24

                                        Row {
                                            anchors.verticalCenter: parent.verticalCenter
                                            spacing: 8

                                            Rectangle {
                                                width: 18
                                                height: 18
                                                radius: 9
                                                border.width: 1
                                                border.color: root.stringSetting("preferredPlaybackQuality") === modelData.value ? root.accentColor : "#cfcfcf"
                                                color: "#ffffff"

                                                Rectangle {
                                                    anchors.centerIn: parent
                                                    width: 10
                                                    height: 10
                                                    radius: 5
                                                    color: root.accentColor
                                                    visible: root.stringSetting("preferredPlaybackQuality") === modelData.value
                                                }
                                            }

                                            Text {
                                                text: modelData.label
                                                color: root.textColor
                                                font.pixelSize: 14
                                            }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.setStringSetting("preferredPlaybackQuality", modelData.value)
                                        }
                                    }
                                }
                            }
                        }

                        Column {
                            spacing: 12

                            Text {
                                text: "播放单首歌曲（搜索结果列表除外）："
                                color: root.textColor
                                font.pixelSize: 14
                            }

                            Column {
                                spacing: 12

                                Repeater {
                                    model: root.singleTrackQueueOptions

                                    delegate: Item {
                                        width: parent.width
                                        implicitHeight: Math.max(18, queueModeLabel.implicitHeight)
                                        height: implicitHeight

                                        RowLayout {
                                            anchors.fill: parent
                                            spacing: 8

                                            Rectangle {
                                                Layout.alignment: Qt.AlignTop
                                                Layout.topMargin: 1
                                                Layout.preferredWidth: 18
                                                Layout.preferredHeight: 18
                                                width: 18
                                                height: 18
                                                radius: 9
                                                border.width: 1
                                                border.color: root.stringSetting("singleTrackQueueMode") === modelData.value ? root.accentColor : "#cfcfcf"
                                                color: "#ffffff"

                                                Rectangle {
                                                    anchors.centerIn: parent
                                                    width: 10
                                                    height: 10
                                                    radius: 5
                                                    color: root.accentColor
                                                    visible: root.stringSetting("singleTrackQueueMode") === modelData.value
                                                }
                                            }

                                            Text {
                                                id: queueModeLabel
                                                Layout.fillWidth: true
                                                text: modelData.label
                                                color: root.textColor
                                                font.pixelSize: 14
                                                wrapMode: Text.WordWrap
                                            }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.setStringSetting("singleTrackQueueMode", modelData.value)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle { width: parent.width; height: 1; color: root.lineColor }

                Item {
                    width: parent.width
                    height: 64

                    Text {
                        x: 0
                        y: 24
                        width: 110
                        text: "通知"
                        color: "#6b7280"
                        font.pixelSize: 14
                    }

                    Row {
                        x: 110
                        y: 20
                        spacing: 54

                        Repeater {
                            model: root.notificationOptions

                            delegate: Item {
                                width: 260
                                height: 24

                                Row {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 8

                                    Rectangle {
                                        width: 18
                                        height: 18
                                        radius: 9
                                        border.width: 1
                                        border.color: root.boolSetting(modelData.key) ? root.accentColor : "#cfcfcf"
                                        color: root.boolSetting(modelData.key) ? root.accentColor : "#ffffff"

                                        Text {
                                            anchors.centerIn: parent
                                            text: root.boolSetting(modelData.key) ? "✓" : ""
                                            color: "#ffffff"
                                            font.pixelSize: 11
                                            font.weight: Font.Bold
                                        }
                                    }

                                    Text {
                                        text: modelData.label
                                        color: root.textColor
                                        font.pixelSize: 14
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.setBoolSetting(modelData.key, !root.boolSetting(modelData.key))
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: downloadPage

        Column {
            width: contentColumn.width

            Item {
                width: parent.width
                height: 108

                Text {
                    x: 0
                    y: 28
                    width: 110
                    text: "下载目录"
                    color: "#6b7280"
                    font.pixelSize: 14
                }

                Column {
                    x: 110
                    y: 20
                    width: parent.width - 110
                    spacing: 10

                    Text {
                        text: "默认将下载的歌曲/视频保存在此文件夹中"
                        color: root.textColor
                        font.pixelSize: 14
                    }

                    Row {
                        spacing: 8

                        TextField {
                            width: 360
                            text: settingsViewModel.downloadPath
                            readOnly: true
                            font.pixelSize: 13
                            color: root.textColor
                            background: Rectangle {
                                color: "#ffffff"
                                border.color: root.borderColor
                                radius: 2
                            }
                        }

                        Button {
                            text: "更改目录"
                            onClicked: root.chooseDownloadPath()
                        }

                        Button {
                            text: "打开文件夹"
                            onClicked: root.openLocalPath(settingsViewModel.downloadPath)
                        }
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: 100

                Text {
                    x: 0
                    y: 28
                    width: 110
                    text: "下载歌曲"
                    color: "#6b7280"
                    font.pixelSize: 14
                }

                Column {
                    x: 110
                    y: 24
                    spacing: 14

                    Row {
                        spacing: 54

                        Repeater {
                            model: [
                                { "key": "downloadLyrics", "label": "同时下载歌词" },
                                { "key": "downloadCover", "label": "同时下载专辑图片" }
                            ]

                            delegate: Item {
                                width: 180
                                height: 24

                                Row {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 8
                                    Rectangle {
                                        width: 18; height: 18; radius: 9
                                        border.width: 1
                                        border.color: root.boolSetting(modelData.key) ? root.accentColor : "#cfcfcf"
                                        color: root.boolSetting(modelData.key) ? root.accentColor : "#ffffff"
                                        Text { anchors.centerIn: parent; text: root.boolSetting(modelData.key) ? "✓" : ""; color: "#ffffff"; font.pixelSize: 11; font.weight: Font.Bold }
                                    }
                                    Text { text: modelData.label; color: root.textColor; font.pixelSize: 14 }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.setBoolSetting(modelData.key, !root.boolSetting(modelData.key))
                                }
                            }
                        }
                    }

                    Column {
                        spacing: 6

                        Item {
                            width: 260
                            height: 24

                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 8
                                Rectangle {
                                    width: 18; height: 18; radius: 9
                                    border.width: 1
                                    border.color: root.boolSetting("preferMp3Download") ? root.accentColor : "#cfcfcf"
                                    color: root.boolSetting("preferMp3Download") ? root.accentColor : "#ffffff"
                                    Text { anchors.centerIn: parent; text: root.boolSetting("preferMp3Download") ? "✓" : ""; color: "#ffffff"; font.pixelSize: 11; font.weight: Font.Bold }
                                }
                                Text { text: "优先下载 mp3格式歌曲"; color: root.textColor; font.pixelSize: 14 }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.setBoolSetting("preferMp3Download", !root.boolSetting("preferMp3Download"))
                            }
                        }

                        Text {
                            text: "仅支持已单曲购买的歌曲以及非VIP歌曲"
                            color: "#9ca3af"
                            font.pixelSize: 12
                            leftPadding: 26
                        }
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: 74

                Text { x: 0; y: 28; width: 110; text: "文件智能分类"; color: "#6b7280"; font.pixelSize: 14 }

                Row {
                    x: 110
                    y: 24
                    spacing: 54

                    Repeater {
                        model: root.downloadFolderOptions
                        delegate: Item {
                            width: 160
                            height: 24
                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 8
                                Rectangle {
                                    width: 18; height: 18; radius: 9; border.width: 1
                                    border.color: root.stringSetting("downloadFolderClassification") === modelData.value ? root.accentColor : "#cfcfcf"
                                    color: "#ffffff"
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 10; height: 10; radius: 5
                                        color: root.accentColor
                                        visible: root.stringSetting("downloadFolderClassification") === modelData.value
                                    }
                                }
                                Text { text: modelData.label; color: root.textColor; font.pixelSize: 14 }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.setStringSetting("downloadFolderClassification", modelData.value)
                            }
                        }
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: 74

                Text { x: 0; y: 28; width: 110; text: "歌曲命名格式"; color: "#6b7280"; font.pixelSize: 14 }

                Row {
                    x: 110
                    y: 24
                    spacing: 54

                    Repeater {
                        model: root.songNamingOptions
                        delegate: Item {
                            width: 160
                            height: 24
                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 8
                                Rectangle {
                                    width: 18; height: 18; radius: 9; border.width: 1
                                    border.color: root.stringSetting("downloadSongNamingFormat") === modelData.value ? root.accentColor : "#cfcfcf"
                                    color: "#ffffff"
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 10; height: 10; radius: 5
                                        color: root.accentColor
                                        visible: root.stringSetting("downloadSongNamingFormat") === modelData.value
                                    }
                                }
                                Text { text: modelData.label; color: root.textColor; font.pixelSize: 14 }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.setStringSetting("downloadSongNamingFormat", modelData.value)
                            }
                        }
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: 148

                Text { x: 0; y: 28; width: 110; text: "歌曲缓存设置"; color: "#6b7280"; font.pixelSize: 14 }

                Column {
                    x: 110
                    y: 20
                    width: parent.width - 110
                    spacing: 12

                    Text { text: "默认将听过的歌曲缓存在此文件夹中"; color: root.textColor; font.pixelSize: 14 }

                    Row {
                        spacing: 8

                        TextField {
                            width: 360
                            text: settingsViewModel.audioCachePath
                            readOnly: true
                            font.pixelSize: 13
                            color: root.textColor
                            background: Rectangle { color: "#ffffff"; border.color: root.borderColor; radius: 2 }
                        }

                        Button { text: "更改目录"; onClicked: root.chooseAudioCachePath() }
                        Button { text: "打开文件夹"; onClicked: root.openLocalPath(settingsViewModel.audioCachePath) }
                        Button { text: "清理缓存"; onClicked: root.clearLocalCacheRequested() }
                    }

                    Row {
                        spacing: 14
                        Text { text: "缓存占用空间"; color: "#6b7280"; font.pixelSize: 14; width: 92 }

                        Repeater {
                            model: [
                                { "value": "auto", "label": "自动" },
                                { "value": "manual", "label": "手动" }
                            ]

                            delegate: Item {
                                width: modelData.value === "manual" ? 360 : 80
                                height: 24

                                Row {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 8
                                    Rectangle {
                                        width: 18; height: 18; radius: 9; border.width: 1
                                        border.color: root.stringSetting("cacheSpaceMode") === modelData.value ? root.accentColor : "#cfcfcf"
                                        color: "#ffffff"
                                        Rectangle {
                                            anchors.centerIn: parent
                                            width: 10; height: 10; radius: 5
                                            color: root.accentColor
                                            visible: root.stringSetting("cacheSpaceMode") === modelData.value
                                        }
                                    }
                                    Text { text: modelData.label; color: root.textColor; font.pixelSize: 14 }
                                    Text {
                                        visible: modelData.value === "manual"
                                        text: "设置缓存最大占用空间为(>2048)"
                                        color: root.hintColor
                                        font.pixelSize: 12
                                    }
                                    TextField {
                                        visible: modelData.value === "manual"
                                        width: 70
                                        text: String(settingsViewModel.cacheSpaceLimitMb)
                                        font.pixelSize: 13
                                        color: root.textColor
                                        horizontalAlignment: Text.AlignHCenter
                                        background: Rectangle { color: "#ffffff"; border.color: root.borderColor; radius: 2 }
                                        onEditingFinished: root.setIntSetting("cacheSpaceLimitMb", parseInt(text || "0"))
                                    }
                                    Text {
                                        visible: modelData.value === "manual"
                                        text: "MB (定期清理)"
                                        color: root.hintColor
                                        font.pixelSize: 12
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.setStringSetting("cacheSpaceMode", modelData.value)
                                }
                            }
                        }
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: 90

                Text { x: 0; y: 28; width: 110; text: "MP3标签设置"; color: "#6b7280"; font.pixelSize: 14 }

                Column {
                    x: 110
                    y: 20
                    spacing: 12

                    Text { text: "写入类型(当在车载或移动设备上无法显示信息或图片时请尝试优先单独勾选ID3v2)"; color: root.hintColor; font.pixelSize: 12 }

                    Row {
                        spacing: 54

                        Repeater {
                            model: [
                                { "key": "mp3TagApeV2", "label": "APEv2" },
                                { "key": "mp3TagId3v1", "label": "ID3v1" },
                                { "key": "mp3TagId3v2", "label": "ID3v2" }
                            ]

                            delegate: Item {
                                width: 110
                                height: 24

                                Row {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 8
                                    Rectangle {
                                        width: 18; height: 18; radius: 9
                                        border.width: 1
                                        border.color: root.boolSetting(modelData.key) ? root.accentColor : "#cfcfcf"
                                        color: root.boolSetting(modelData.key) ? root.accentColor : "#ffffff"
                                        Text { anchors.centerIn: parent; text: root.boolSetting(modelData.key) ? "✓" : ""; color: "#ffffff"; font.pixelSize: 11; font.weight: Font.Bold }
                                    }
                                    Text { text: modelData.label; color: root.textColor; font.pixelSize: 14 }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.setBoolSetting(modelData.key, !root.boolSetting(modelData.key))
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: lyricsPage

        Column {
            width: contentColumn.width

            Item {
                width: parent.width
                height: 72

                Text { x: 0; y: 24; width: 110; text: "桌面歌词"; color: "#6b7280"; font.pixelSize: 14 }

                Button {
                    x: 110
                    y: 18
                    text: "恢复默认"
                    onClicked: settingsViewModel.resetDesktopLyricsSettings()
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: 74
                Text { x: 0; y: 28; width: 110; text: "行数"; color: "#6b7280"; font.pixelSize: 14 }

                Row {
                    x: 110; y: 24; spacing: 54
                    Repeater {
                        model: root.lyricLineOptions
                        delegate: Item {
                            width: 130; height: 24
                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 8
                                Rectangle {
                                    width: 18; height: 18; radius: 9; border.width: 1
                                    border.color: root.stringSetting("desktopLyricsLineMode") === modelData.value ? root.accentColor : "#cfcfcf"
                                    color: "#ffffff"
                                    Rectangle { anchors.centerIn: parent; width: 10; height: 10; radius: 5; color: root.accentColor; visible: root.stringSetting("desktopLyricsLineMode") === modelData.value }
                                }
                                Text { text: modelData.label; color: root.textColor; font.pixelSize: 14 }
                            }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.setStringSetting("desktopLyricsLineMode", modelData.value) }
                        }
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: 74
                Text { x: 0; y: 28; width: 110; text: "对齐"; color: "#6b7280"; font.pixelSize: 14 }

                Row {
                    x: 110; y: 24; spacing: 42
                    Repeater {
                        model: root.lyricAlignmentOptions
                        delegate: Item {
                            width: 100; height: 24
                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 8
                                Rectangle {
                                    width: 18; height: 18; radius: 9; border.width: 1
                                    border.color: root.stringSetting("desktopLyricsAlignment") === modelData.value ? root.accentColor : "#cfcfcf"
                                    color: "#ffffff"
                                    Rectangle { anchors.centerIn: parent; width: 10; height: 10; radius: 5; color: root.accentColor; visible: root.stringSetting("desktopLyricsAlignment") === modelData.value }
                                }
                                Text { text: modelData.label; color: root.textColor; font.pixelSize: 14 }
                            }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.setStringSetting("desktopLyricsAlignment", modelData.value) }
                        }
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: 78
                Text { x: 0; y: 30; width: 110; text: "字体字号"; color: "#6b7280"; font.pixelSize: 14 }

                Row {
                    x: 110; y: 20; spacing: 12

                    ComboBox {
                        width: 180
                        model: root.lyricFontOptions
                        currentIndex: root.comboIndex(root.lyricFontOptions, root.stringSetting("desktopLyricFontFamily"))
                        font.pixelSize: 13
                        onActivated: root.setStringSetting("desktopLyricFontFamily", currentText)
                    }

                    ComboBox {
                        width: 180
                        model: root.lyricFontSizeOptions
                        currentIndex: root.comboIndex(root.lyricFontSizeOptions, String(root.intSetting("desktopLyricFontSize")))
                        font.pixelSize: 13
                        onActivated: root.setIntSetting("desktopLyricFontSize", parseInt(currentText))
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: 110
                Text { x: 0; y: 28; width: 110; text: "颜色(非气泡)"; color: "#6b7280"; font.pixelSize: 14 }

                Column {
                    x: 110; y: 18; width: parent.width - 110; spacing: 14

                    Row {
                        spacing: 16

                        ComboBox {
                            width: 180
                            model: [ "自定义" ]
                            currentIndex: 0
                        }

                        Row {
                            spacing: 18

                            Repeater {
                                model: [
                                    { "key": "desktopLyricsPlayedColor", "label": "已播放字色" },
                                    { "key": "desktopLyricsUnplayedColor", "label": "未播放字色" },
                                    { "key": "desktopLyricsStrokeColor", "label": "边框" }
                                ]

                                delegate: Row {
                                    spacing: 6
                                    Text { text: modelData.label; color: "#4b5563"; font.pixelSize: 12 }
                                    Rectangle {
                                        width: 14; height: 14; border.color: "#cfcfcf"
                                        color: root.colorSetting(modelData.key)
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.openColorPicker(modelData.key)
                                    }
                                }
                            }
                        }
                    }

                    Row {
                        spacing: 54
                        Repeater {
                            model: [
                                { "key": "desktopLyricsCoverTaskbar", "label": "覆盖任务栏" },
                                { "key": "desktopLyricsBold", "label": "粗体" }
                            ]
                            delegate: Item {
                                width: 120; height: 24
                                Row {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 8
                                    Rectangle {
                                        width: 18; height: 18; radius: 9
                                        border.width: 1
                                        border.color: root.boolSetting(modelData.key) ? root.accentColor : "#cfcfcf"
                                        color: root.boolSetting(modelData.key) ? root.accentColor : "#ffffff"
                                        Text { anchors.centerIn: parent; text: root.boolSetting(modelData.key) ? "✓" : ""; color: "#ffffff"; font.pixelSize: 11; font.weight: Font.Bold }
                                    }
                                    Text { text: modelData.label; color: root.textColor; font.pixelSize: 14 }
                                }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.setBoolSetting(modelData.key, !root.boolSetting(modelData.key)) }
                            }
                        }
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: 74
                Text { x: 0; y: 28; width: 110; text: "文字透明度"; color: "#6b7280"; font.pixelSize: 14 }

                Slider {
                    x: 110
                    y: 24
                    width: 260
                    from: 0
                    to: 100
                    value: root.intSetting("desktopLyricsOpacity")
                    onMoved: root.setIntSetting("desktopLyricsOpacity", Math.round(value))
                }

                Text {
                    x: 388
                    y: 28
                    text: root.intSetting("desktopLyricsOpacity") + "%"
                    color: root.hintColor
                    font.pixelSize: 12
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: 210
                Text { x: 0; y: 28; width: 110; text: "预览"; color: "#6b7280"; font.pixelSize: 14 }

                Rectangle {
                    x: 110
                    y: 20
                    width: Math.min(parent.width - 110, 620)
                    height: 150
                    color: "#f0f0f0"
                    radius: 4

                    Column {
                        anchors.centerIn: parent
                        spacing: 6

                        Text {
                            text: "QQ音乐 听我想听"
                            color: root.colorSetting("desktopLyricsPlayedColor")
                            font.pixelSize: root.intSetting("desktopLyricFontSize") * 0.6
                            font.family: root.stringSetting("desktopLyricFontFamily")
                            font.bold: root.boolSetting("desktopLyricsBold")
                            style: Text.Outline
                            styleColor: root.colorSetting("desktopLyricsStrokeColor")
                            opacity: root.intSetting("desktopLyricsOpacity") / 100.0
                        }

                        Text {
                            text: "QQ音乐 听我想听"
                            color: root.colorSetting("desktopLyricsUnplayedColor")
                            font.pixelSize: root.intSetting("desktopLyricFontSize") * 0.6
                            font.family: root.stringSetting("desktopLyricFontFamily")
                            font.bold: root.boolSetting("desktopLyricsBold")
                            style: Text.Outline
                            styleColor: root.colorSetting("desktopLyricsStrokeColor")
                            opacity: root.intSetting("desktopLyricsOpacity") / 100.0
                            x: 60
                        }
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: 68
                Text { x: 0; y: 24; width: 110; text: "快捷键"; color: "#6b7280"; font.pixelSize: 14 }
                Button {
                    x: 110
                    y: 18
                    text: "恢复默认"
                    onClicked: settingsViewModel.resetShortcutSettings()
                }
            }
        }
    }

    Component {
        id: shortcutsPage

        Column {
            width: contentColumn.width

            Item {
                width: parent.width
                height: 70
                Text { x: 0; y: 24; width: 110; text: "快捷键"; color: "#6b7280"; font.pixelSize: 14 }
                Button { x: 110; y: 18; text: "恢复默认"; onClicked: settingsViewModel.resetShortcutSettings() }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: 68
                Text { x: 0; y: 24; width: 110; text: "全局设置"; color: "#6b7280"; font.pixelSize: 14 }
                Item {
                    x: 110; y: 20; width: 200; height: 24
                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8
                        Rectangle {
                            width: 18; height: 18; radius: 9; border.width: 1
                            border.color: root.boolSetting("globalShortcutsEnabled") ? root.accentColor : "#cfcfcf"
                            color: root.boolSetting("globalShortcutsEnabled") ? root.accentColor : "#ffffff"
                            Text { anchors.centerIn: parent; text: root.boolSetting("globalShortcutsEnabled") ? "✓" : ""; color: "#ffffff"; font.pixelSize: 11; font.weight: Font.Bold }
                        }
                        Text { text: "启用全局快捷键"; color: root.textColor; font.pixelSize: 14 }
                    }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.setBoolSetting("globalShortcutsEnabled", !root.boolSetting("globalShortcutsEnabled")) }
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Repeater {
                model: [
                    { "title": "播放控制", "rows": root.shortcutPlaybackRows },
                    { "title": "其他", "rows": root.shortcutOtherRows },
                    { "title": "功能键", "rows": root.shortcutFunctionRows }
                ]

                delegate: Column {
                    width: parent.width

                    Item {
                        width: parent.width
                        height: modelData.rows.length * 38 + 32

                        Text {
                            x: 0
                            y: 24
                            width: 110
                            text: modelData.title
                            color: "#6b7280"
                            font.pixelSize: 14
                        }

                        Column {
                            x: 110
                            y: 18
                            width: parent.width - 110
                            spacing: 10

                            Repeater {
                                model: modelData.rows

                                delegate: Row {
                                    spacing: 16

                                    Text {
                                        width: 170
                                        text: modelData.label
                                        color: root.textColor
                                        font.pixelSize: 14
                                    }

                                    TextField {
                                        width: 180
                                        text: root.stringSetting(modelData.key)
                                        font.pixelSize: 13
                                        color: root.textColor
                                        background: Rectangle { color: "#ffffff"; border.color: root.borderColor; radius: 2 }
                                        onEditingFinished: root.setStringSetting(modelData.key, text)
                                    }

                                    Text {
                                        text: modelData.status
                                        color: root.hintColor
                                        font.pixelSize: 12
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }
                        }
                    }

                    Rectangle { width: parent.width; height: 1; color: root.lineColor; visible: index !== 2 }
                }
            }
        }
    }

    Component {
        id: pluginsPage

        Column {
            width: contentColumn.width

            Item {
                width: parent.width
                height: 72
                Text { x: 0; y: 24; width: 110; text: "音效插件"; color: "#6b7280"; font.pixelSize: 14 }
                Button { x: 110; y: 18; text: "VST3插件" }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: 396
                Text { x: 0; y: 24; width: 110; text: "音效插件"; color: "#6b7280"; font.pixelSize: 14 }

                Rectangle {
                    x: 110
                    y: 18
                    width: parent.width - 110
                    height: 338
                    color: "#f9f9f9"
                    border.color: root.borderColor

                    Column {
                        anchors.fill: parent

                        Rectangle {
                            width: parent.width
                            height: 40
                            color: "#ffffff"
                            border.color: root.borderColor
                            border.width: 0

                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: 16
                                anchors.rightMargin: 16
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 0

                                Text { width: parent.width * 0.5; text: "描述"; color: root.textColor; font.pixelSize: 13 }
                                Text { width: parent.width * 0.5; text: "文件名"; color: root.textColor; font.pixelSize: 13 }
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: 228
                            color: "#fafafa"

                            Text {
                                anchors.centerIn: parent
                                text: "添加本地插件, 让音乐更美妙"
                                color: root.accentColor
                                font.pixelSize: 15
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: 70
                            color: "#ffffff"
                            border.color: root.borderColor
                            border.width: 0

                            Row {
                                anchors.fill: parent
                                anchors.margins: 16
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 8

                                Button { text: "配置插件" }
                                Button { text: "删除插件" }
                                Item { width: 24; height: 1 }
                                Text { text: "优先级顺序"; color: root.textColor; font.pixelSize: 13; verticalAlignment: Text.AlignVCenter }
                                Button { text: "向上" }
                                Button { text: "向下" }
                            }
                        }
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: 78
                Text { x: 0; y: 28; width: 110; text: "插件存放目录"; color: "#6b7280"; font.pixelSize: 14 }

                Row {
                    x: 110
                    y: 20
                    spacing: 8

                    TextField {
                        width: 420
                        text: root.stringSetting("pluginDirectory")
                        font.pixelSize: 13
                        color: root.textColor
                        background: Rectangle { color: "#ffffff"; border.color: root.borderColor; radius: 2 }
                        onEditingFinished: root.setStringSetting("pluginDirectory", text)
                    }

                    Button { text: "添加插件" }
                }
            }
        }
    }

    Component {
        id: networkPage

        Column {
            width: contentColumn.width

            Item {
                width: parent.width
                height: 56
                Text {
                    x: 0
                    y: 20
                    text: "网络设置"
                    color: root.textColor
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: 270

                Text { x: 0; y: 28; width: 110; text: "代理设置"; color: "#6b7280"; font.pixelSize: 14 }

                Column {
                    x: 110
                    y: 20
                    width: parent.width - 110
                    spacing: 16

                    Row {
                        spacing: 16
                        Text { width: 48; text: "类型"; color: "#6b7280"; font.pixelSize: 14 }
                        ComboBox {
                            width: 200
                            model: root.proxyTypeOptions
                            textRole: "label"
                            currentIndex: root.comboIndex(root.proxyTypeOptions, root.stringSetting("proxyType"))
                            onActivated: root.setStringSetting("proxyType", root.proxyTypeOptions[index].value)
                        }
                    }

                    Row {
                        spacing: 36

                        Row {
                            spacing: 16
                            Text { width: 48; text: "地址"; color: "#6b7280"; font.pixelSize: 14 }
                            TextField {
                                width: 200
                                text: root.stringSetting("proxyHost")
                                font.pixelSize: 13
                                color: root.textColor
                                background: Rectangle { color: "#ffffff"; border.color: root.borderColor; radius: 2 }
                                onEditingFinished: root.setStringSetting("proxyHost", text)
                            }
                        }

                        Row {
                            spacing: 16
                            Text { width: 40; text: "端口"; color: "#6b7280"; font.pixelSize: 14 }
                            TextField {
                                width: 120
                                text: String(root.intSetting("proxyPort"))
                                font.pixelSize: 13
                                color: root.textColor
                                background: Rectangle { color: "#ffffff"; border.color: root.borderColor; radius: 2 }
                                onEditingFinished: root.setIntSetting("proxyPort", parseInt(text || "0"))
                            }
                        }
                    }

                    Row {
                        spacing: 36

                        Row {
                            spacing: 16
                            Text { width: 48; text: "用户名"; color: "#6b7280"; font.pixelSize: 14 }
                            TextField {
                                width: 200
                                text: root.stringSetting("proxyUsername")
                                font.pixelSize: 13
                                color: root.textColor
                                background: Rectangle { color: "#ffffff"; border.color: root.borderColor; radius: 2 }
                                onEditingFinished: root.setStringSetting("proxyUsername", text)
                            }
                        }

                        Row {
                            spacing: 16
                            Text { width: 40; text: "密码"; color: "#6b7280"; font.pixelSize: 14 }
                            TextField {
                                width: 200
                                echoMode: TextInput.Password
                                text: root.stringSetting("proxyPassword")
                                font.pixelSize: 13
                                color: root.textColor
                                background: Rectangle { color: "#ffffff"; border.color: root.borderColor; radius: 2 }
                                onEditingFinished: root.setStringSetting("proxyPassword", text)
                            }
                        }
                    }

                    Row {
                        spacing: 24

                        Button {
                            text: "测试"
                            enabled: false
                        }

                        Text {
                            text: "(页面需要重启QQ音乐客户端才会使用新的代理)"
                            color: root.accentColor
                            font.pixelSize: 13
                        }
                    }
                }
            }
        }
    }

    Component {
        id: audioPage

        Column {
            width: contentColumn.width

            Item {
                width: parent.width
                height: 56
                Text {
                    x: 0
                    y: 20
                    text: "音频设备"
                    color: root.textColor
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.lineColor }

            Item {
                width: parent.width
                height: audioDeviceBlock.implicitHeight + 60

                Text { x: 0; y: 28; width: 110; text: "设备设置"; color: "#6b7280"; font.pixelSize: 14 }

                Column {
                    id: audioDeviceBlock
                    x: 110
                    y: 24
                    width: Math.min(parent.width - 110, 640)
                    spacing: 16

                    Repeater {
                        model: [
                            { "label": "输出设备", "key": "audioOutputDevice", "options": root.audioOutputDeviceOptions },
                            { "label": "输出格式", "key": "audioOutputFormat", "options": root.audioOutputFormatOptions },
                            { "label": "频率转换", "key": "audioFrequencyConversion", "options": root.audioFrequencyOptions },
                            { "label": "输出声道", "key": "audioOutputChannels", "options": root.audioChannelOptions }
                        ]

                        delegate: Item {
                            width: parent.width
                            implicitHeight: Math.max(audioDeviceLabel.implicitHeight, audioDeviceCombo.implicitHeight)
                            height: implicitHeight

                            RowLayout {
                                anchors.fill: parent
                                spacing: 18

                                Text {
                                    id: audioDeviceLabel
                                    width: 76
                                    Layout.preferredWidth: 76
                                    Layout.alignment: Qt.AlignVCenter
                                    text: modelData.label
                                    color: root.textColor
                                    font.pixelSize: 14
                                    verticalAlignment: Text.AlignVCenter
                                }

                                ComboBox {
                                    id: audioDeviceCombo
                                    width: 320
                                    Layout.preferredWidth: 320
                                    Layout.alignment: Qt.AlignVCenter
                                    height: 32
                                    model: modelData.options
                                    currentIndex: root.comboIndex(modelData.options, root.stringSetting(modelData.key))
                                    font.pixelSize: 13
                                    onActivated: root.setStringSetting(modelData.key, currentText)

                                    contentItem: Text {
                                        leftPadding: 12
                                        rightPadding: 28
                                        text: audioDeviceCombo.currentText
                                        font: audioDeviceCombo.font
                                        color: root.textColor
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }

                                    indicator: Text {
                                        text: "▼"
                                        color: "#9ca3af"
                                        font.pixelSize: 10
                                        anchors.right: parent.right
                                        anchors.rightMargin: 10
                                        anchors.verticalCenter: parent.verticalCenter
                                    }

                                    background: Rectangle {
                                        radius: 2
                                        color: "#ffffff"
                                        border.color: root.borderColor
                                    }
                                }
                            }
                        }
                    }

                    Column {
                        width: parent.width
                        spacing: 12

                        Repeater {
                            model: [
                                { "key": "audioLocalFileMemoryMode", "label": "本地文件使用内存模式（占用较大内存，拥有较低延迟）" },
                                { "key": "audioGaplessPlayback", "label": "无缝播放" }
                            ]

                            delegate: Item {
                                width: parent.width
                                implicitHeight: Math.max(18, audioBoolLabel.implicitHeight)
                                height: implicitHeight

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 8
                                    Rectangle {
                                        Layout.alignment: Qt.AlignTop
                                        Layout.topMargin: 1
                                        Layout.preferredWidth: 18
                                        Layout.preferredHeight: 18
                                        width: 18; height: 18; radius: 9
                                        border.width: 1
                                        border.color: root.boolSetting(modelData.key) ? root.accentColor : "#cfcfcf"
                                        color: root.boolSetting(modelData.key) ? root.accentColor : "#ffffff"
                                        Text { anchors.centerIn: parent; text: root.boolSetting(modelData.key) ? "✓" : ""; color: "#ffffff"; font.pixelSize: 11; font.weight: Font.Bold }
                                    }
                                    Text {
                                        id: audioBoolLabel
                                        Layout.fillWidth: true
                                        text: modelData.label
                                        color: root.textColor
                                        font.pixelSize: 14
                                        wrapMode: Text.WordWrap
                                    }
                                }

                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.setBoolSetting(modelData.key, !root.boolSetting(modelData.key)) }
                            }
                        }
                    }

                    Item {
                        width: parent.width
                        implicitHeight: Math.max(audioDsdLabel.implicitHeight, audioDsdCombo.implicitHeight)
                        height: implicitHeight

                        RowLayout {
                            anchors.fill: parent
                            spacing: 16

                            Text {
                                id: audioDsdLabel
                                width: 76
                                Layout.preferredWidth: 76
                                Layout.alignment: Qt.AlignVCenter
                                text: "DSD优选模式"
                                color: root.textColor
                                font.pixelSize: 14
                            }

                            ComboBox {
                                id: audioDsdCombo
                                width: 180
                                Layout.preferredWidth: 180
                                Layout.alignment: Qt.AlignVCenter
                                model: root.audioDsdModeOptions
                                textRole: "label"
                                currentIndex: root.comboIndex(root.audioDsdModeOptions, root.stringSetting("audioDsdMode"))
                                onActivated: root.setStringSetting("audioDsdMode", root.audioDsdModeOptions[index].value)
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 190
                        color: "#fbfbfb"
                        border.color: root.borderColor

                        Column {
                            anchors.fill: parent
                            anchors.margins: 18
                            spacing: 12

                            Row {
                                spacing: 0
                                Text { width: 180; text: "属性"; color: root.textColor; font.pixelSize: 13; font.weight: Font.DemiBold }
                                Text { width: 320; text: "值"; color: root.textColor; font.pixelSize: 13; font.weight: Font.DemiBold }
                            }

                            Rectangle { width: parent.width; height: 1; color: root.lineColor }

                            Repeater {
                                model: [
                                    { "name": "设备类型", "value": "DirectSound" },
                                    { "name": "设备鉴定", "value": "不支持" },
                                    { "name": "驱动程序", "value": "Device" },
                                    { "name": "支持采样率", "value": "100 - 200000 Hz" },
                                    { "name": "硬件混音", "value": "Free 0 / Max 1" }
                                ]

                                delegate: Row {
                                    spacing: 0
                                    Text { width: 180; text: modelData.name; color: root.hintColor; font.pixelSize: 13 }
                                    Text { width: 320; text: modelData.value; color: root.textColor; font.pixelSize: 13 }
                                }
                            }
                        }
                    }

                    Row {
                        spacing: 18
                        Text { width: 76; text: "缓存大小"; color: root.textColor; font.pixelSize: 14 }
                        Slider {
                            width: 220
                            from: 200
                            to: 5000
                            stepSize: 100
                            value: root.intSetting("audioCacheSizeMs")
                            onMoved: root.setIntSetting("audioCacheSizeMs", Math.round(value))
                        }
                        Text { text: root.intSetting("audioCacheSizeMs") + "ms"; color: root.hintColor; font.pixelSize: 12 }
                    }

                    Row {
                        spacing: 16
                        Text { text: "上述参数会在重新播放的时候生效"; color: root.hintColor; font.pixelSize: 12 }
                        Button { text: "恢复默认"; onClicked: settingsViewModel.resetAudioDeviceSettings() }
                    }
                }
            }
        }
    }

    ButtonGroup { id: invisibleGroup }
}
