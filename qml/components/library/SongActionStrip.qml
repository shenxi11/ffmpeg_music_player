import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

Item {
    id: root

    property var songData: ({})
    property var availablePlaylists: []
    property bool favoriteActive: false
    property bool showFavoriteButton: true
    property bool showDownloadButton: false
    property bool showAddButton: true
    property bool showMoreButton: true
    property bool showInlinePlayButton: false
    property bool showRemoveAction: false
    property bool showDownloadActionInMenu: showDownloadButton
    property string removeActionText: "删除"
    property string listIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/list/"
    property string playerIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/player/"
    property color borderColor: "#D6DCE8"
    property color hoverAccent: "#EC4141"
    property color hoverAccentSoft: "#FDECEC"
    property color buttonBaseColor: "#F7F9FC"
    property color menuShadowColor: "#220F172A"
    property bool interactionActive: hoverHandler.hovered || addPopup.visible || morePopup.visible

    signal actionRequested(string action, var payload)

    width: buttonRow.implicitWidth
    height: 32

    function mergedPayload(extra) {
        var payload = {}
        if (songData) {
            for (var key in songData) {
                payload[key] = songData[key]
            }
        }
        if (extra) {
            for (var extraKey in extra) {
                payload[extraKey] = extra[extraKey]
            }
        }
        return payload
    }

    function emitAction(action, extra) {
        root.actionRequested(action, mergedPayload(extra))
    }

    function closeMenus() {
        addPopup.close()
        morePopup.close()
    }

    function openAddMenu(anchorItem) {
        morePopup.close()
        addPopup.x = anchorItem.x - Math.max(0, addPopup.width - anchorItem.width)
        addPopup.y = anchorItem.y + anchorItem.height + 6
        addPopup.open()
    }

    function openMoreMenu(anchorItem) {
        addPopup.close()
        morePopup.x = anchorItem.x - Math.max(0, morePopup.width - anchorItem.width)
        morePopup.y = anchorItem.y + anchorItem.height + 6
        morePopup.open()
    }

    Component {
        id: popupActionDelegate

        Rectangle {
            id: menuRow
            property string text: ""
            property var triggerAction: null
            property bool closeMenusAfterClick: true
            width: parent ? parent.width : 180
            height: 36
            color: actionArea.containsMouse ? "#F4F7FB" : "transparent"
            radius: 10

            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.right: parent.right
                anchors.rightMargin: 14
                text: menuRow.text
                font.pixelSize: 12
                color: "#1F2937"
                elide: Text.ElideRight
            }

            MouseArea {
                id: actionArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (menuRow.triggerAction) {
                        menuRow.triggerAction()
                    }
                    if (menuRow.closeMenusAfterClick) {
                        root.closeMenus()
                    }
                }
            }
        }
    }

    Row {
        id: buttonRow
        spacing: 8

        Rectangle {
            visible: root.showFavoriteButton
            width: 32
            height: 32
            radius: 16
            color: root.favoriteActive
                   ? (favoriteArea.containsMouse ? "#FDE3E3" : "#FFF1F1")
                   : (favoriteArea.containsMouse ? "#FFF1F1" : root.buttonBaseColor)
            border.width: 1
            border.color: favoriteArea.containsMouse || root.favoriteActive ? "#F2BFBF" : "#E5EAF2"

            Image {
                anchors.centerIn: parent
                width: 18
                height: 18
                source: root.favoriteActive
                        ? (favoriteArea.containsMouse
                           ? "qrc:/qml/assets/ai/icons/song-action-favorite-hover.svg"
                           : "qrc:/qml/assets/ai/icons/song-action-favorite-active.svg")
                        : (favoriteArea.containsMouse
                           ? "qrc:/qml/assets/ai/icons/song-action-favorite-hover.svg"
                           : "qrc:/qml/assets/ai/icons/song-action-favorite-default.svg")
                fillMode: Image.PreserveAspectFit
            }

            MouseArea {
                id: favoriteArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.emitAction(root.favoriteActive ? "remove_favorite" : "add_favorite")
            }
        }

        Rectangle {
            visible: root.showDownloadButton
            width: 32
            height: 32
            radius: 16
            color: downloadArea.containsMouse ? root.hoverAccentSoft : root.buttonBaseColor
            border.width: 1
            border.color: downloadArea.containsMouse ? root.hoverAccent : "#E5EAF2"

            Image {
                anchors.centerIn: parent
                width: 18
                height: 18
                source: downloadArea.containsMouse
                        ? "qrc:/qml/assets/ai/icons/song-action-download-hover.svg"
                        : "qrc:/qml/assets/ai/icons/song-action-download-default.svg"
                fillMode: Image.PreserveAspectFit
            }

            MouseArea {
                id: downloadArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.emitAction("download")
            }
        }

        Rectangle {
            visible: root.showAddButton
            width: 32
            height: 32
            radius: 16
            color: addArea.containsMouse ? "#EEF5FF" : root.buttonBaseColor
            border.width: 1
            border.color: addArea.containsMouse ? "#79AFFF" : "#E5EAF2"

            Image {
                anchors.centerIn: parent
                width: 16
                height: 16
                source: "qrc:/qml/assets/ai/icons/song-action-add.svg"
                fillMode: Image.PreserveAspectFit
            }

            MouseArea {
                id: addArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.openAddMenu(parent)
            }
        }

        Rectangle {
            visible: root.showMoreButton
            width: 32
            height: 32
            radius: 16
            color: moreArea.containsMouse ? "#EEF2F7" : root.buttonBaseColor
            border.width: 1
            border.color: moreArea.containsMouse ? "#9AA4B2" : "#E5EAF2"

            Image {
                anchors.centerIn: parent
                width: 16
                height: 16
                source: "qrc:/qml/assets/ai/icons/song-action-more.svg"
                fillMode: Image.PreserveAspectFit
            }

            MouseArea {
                id: moreArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.openMoreMenu(parent)
            }
        }
    }

    HoverHandler {
        id: hoverHandler
    }

    Popup {
        id: addPopup
        parent: root
        width: 212
        padding: 8
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

        background: Item {
            implicitWidth: addPopup.width
            implicitHeight: addPopup.contentItem ? addPopup.contentItem.implicitHeight + addPopup.topPadding + addPopup.bottomPadding : 180

            DropShadow {
                anchors.fill: addPanel
                source: addPanel
                horizontalOffset: 0
                verticalOffset: 10
                radius: 22
                samples: 33
                color: root.menuShadowColor
            }

            Rectangle {
                id: addPanel
                anchors.fill: parent
                radius: 14
                color: "#FFFFFF"
                border.width: 1
                border.color: "#E9EEF5"
            }
        }

        Column {
            id: addMenuColumn
            width: parent.width
            spacing: 4

            Loader {
                width: parent.width
                sourceComponent: popupActionDelegate
                onLoaded: {
                    item.text = "播放队列"
                    item.triggerAction = function() { root.emitAction("queue_append") }
                }
            }

            Loader {
                width: parent.width
                sourceComponent: popupActionDelegate
                onLoaded: {
                    item.text = "下一首播放"
                    item.triggerAction = function() { root.emitAction("play_next") }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: "#F0F3F8"
            }

            Loader {
                width: parent.width
                sourceComponent: popupActionDelegate
                onLoaded: {
                    item.text = "添加到新歌单"
                    item.triggerAction = function() { root.emitAction("create_playlist_and_add") }
                }
            }

            Loader {
                visible: !root.availablePlaylists || root.availablePlaylists.length === 0
                width: parent.width
                sourceComponent: popupActionDelegate
                onLoaded: {
                    item.text = "暂无可用歌单"
                    item.triggerAction = null
                }
            }

            Repeater {
                model: root.availablePlaylists
                delegate: Loader {
                    property var playlistData: modelData
                    width: parent ? parent.width : addPopup.width
                    sourceComponent: popupActionDelegate
                    onLoaded: {
                        var playlist = playlistData || {}
                        item.text = playlist.name || "未命名歌单"
                        item.triggerAction = function() {
                            root.emitAction("add_to_playlist", {
                                playlistId: Number(playlist.id || 0),
                                playlistName: playlist.name || ""
                            })
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: morePopup
        parent: root
        width: 192
        padding: 8
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

        background: Item {
            implicitWidth: morePopup.width
            implicitHeight: morePopup.contentItem ? morePopup.contentItem.implicitHeight + morePopup.topPadding + morePopup.bottomPadding : 180

            DropShadow {
                anchors.fill: morePanel
                source: morePanel
                horizontalOffset: 0
                verticalOffset: 10
                radius: 22
                samples: 33
                color: root.menuShadowColor
            }

            Rectangle {
                id: morePanel
                anchors.fill: parent
                radius: 14
                color: "#FFFFFF"
                border.width: 1
                border.color: "#E9EEF5"
            }
        }

        Column {
            id: moreMenuColumn
            width: parent.width
            spacing: 4

            Loader {
                width: parent.width
                sourceComponent: popupActionDelegate
                onLoaded: {
                    item.text = "播放"
                    item.triggerAction = function() { root.emitAction("play") }
                }
            }

            Loader {
                width: parent.width
                sourceComponent: popupActionDelegate
                onLoaded: {
                    item.text = "下一首播放"
                    item.triggerAction = function() { root.emitAction("play_next") }
                }
            }

            Loader {
                width: parent.width
                sourceComponent: popupActionDelegate
                onLoaded: {
                    item.text = root.favoriteActive ? "取消喜欢" : "我喜欢"
                    item.triggerAction = function() {
                        root.emitAction(root.favoriteActive ? "remove_favorite" : "add_favorite")
                    }
                }
            }

            Loader {
                width: parent.width
                sourceComponent: popupActionDelegate
                onLoaded: {
                    item.text = "添加到"
                    item.closeMenusAfterClick = false
                    item.triggerAction = function() {
                        root.closeMenus()
                        root.openAddMenu(buttonRow)
                    }
                }
            }

            Loader {
                visible: root.showDownloadActionInMenu
                width: parent.width
                sourceComponent: popupActionDelegate
                onLoaded: {
                    item.text = "下载"
                    item.triggerAction = function() { root.emitAction("download") }
                }
            }

            Rectangle {
                visible: root.showRemoveAction
                width: parent.width
                height: visible ? 1 : 0
                color: "#F0F3F8"
            }

            Loader {
                visible: root.showRemoveAction
                width: parent.width
                sourceComponent: popupActionDelegate
                onLoaded: {
                    item.text = root.removeActionText
                    item.triggerAction = function() { root.emitAction("remove_or_delete") }
                }
            }
        }
    }
}
