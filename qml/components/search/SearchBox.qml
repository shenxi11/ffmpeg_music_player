import QtQuick 2.14
import QtQuick.Controls 2.14
import "../../theme/Theme.js" as Theme

Rectangle {
    id: root
    implicitWidth: 280
    implicitHeight: 56
    color: "transparent"

    property string placeholderText: "搜索歌曲 / 歌单 / 歌手..."
    property alias text: searchInput.text
    property string baseIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/base/"
    property bool suppressTextEdited: false

    signal search(string text)
    signal searchAll()
    signal inputActivated()
    signal textEdited(string text)

    Rectangle {
        anchors.fill: parent
        anchors.margins: 3
        color: searchInput.activeFocus ? "#FFFFFF" : "#FCFDFE"
        radius: height / 2
        border.width: 1
        border.color: searchInput.activeFocus ? Theme.accent : "#E5E8EF"

        Row {
            anchors.fill: parent
            anchors.leftMargin: 18
            anchors.rightMargin: 12
            spacing: 10

            TextInput {
                id: searchInput
                width: parent.width - searchButton.width - parent.spacing - 30
                height: parent.height
                verticalAlignment: TextInput.AlignVCenter
                font.family: "Microsoft YaHei UI"
                font.pixelSize: 15
                color: Theme.textPrimary
                selectByMouse: true
                selectionColor: Theme.accentSoft
                clip: true

                Text {
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    text: root.placeholderText
                    color: "#9CA3AF"
                    font.family: "Microsoft YaHei UI"
                    font.pixelSize: 14
                    visible: !searchInput.text && !searchInput.activeFocus
                }

                onActiveFocusChanged: {
                    if (activeFocus) {
                        root.inputActivated()
                    }
                }

                onTextChanged: {
                    if (!root.suppressTextEdited) {
                        root.textEdited(searchInput.text)
                    }
                }

                Keys.onReturnPressed: { onSearchTriggered() }
                Keys.onEnterPressed: { onSearchTriggered() }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    onTapped: root.inputActivated()
                }
            }

            Rectangle {
                id: searchButton
                width: 34
                height: 34
                anchors.verticalCenter: parent.verticalCenter
                color: searchButtonArea.pressed
                       ? "#F6D8D8"
                       : (searchButtonArea.containsMouse ? Theme.accentSoft : "transparent")
                radius: 17

                Image {
                    anchors.centerIn: parent
                    width: 18
                    height: 18
                    source: searchButtonArea.pressed
                            ? root.baseIconPrefix + "base_icon_search_pressed.svg"
                            : (searchButtonArea.containsMouse
                               ? root.baseIconPrefix + "base_icon_search_hover.svg"
                               : root.baseIconPrefix + "base_icon_search_default.svg")
                    fillMode: Image.PreserveAspectFit
                }

                MouseArea {
                    id: searchButtonArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        searchInput.forceActiveFocus()
                        onSearchTriggered()
                    }
                }
            }
        }
    }

    function onSearchTriggered() {
        var text = searchInput.text.trim()
        // 无论是否为空都发送 search 信号，由 C++ 侧统一处理空值逻辑
        root.search(text)
    }

    function clear() {
        setTextFromHost("")
    }

    function setTextFromHost(value) {
        root.suppressTextEdited = true
        searchInput.text = value || ""
        root.suppressTextEdited = false
    }

    function focusInput() {
        searchInput.forceActiveFocus()
    }
}
