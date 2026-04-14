import QtQuick 2.14
import QtQuick.Controls 2.14
import "../../theme/Theme.js" as Theme

Rectangle {
    id: root
    implicitWidth: 250
    implicitHeight: 60
    color: "transparent"

    property string placeholderText: " 搜索想听的歌曲吧..."
    property alias text: searchInput.text
    property string baseIconPrefix: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/base/"
    property bool suppressTextEdited: false

    signal search(string text)
    signal searchAll()
    signal inputActivated()
    signal textEdited(string text)

    Rectangle {
        anchors.fill: parent
        anchors.margins: 5
        color: "#FFFFFF"
        radius: 8
        border.width: 1
        border.color: searchInput.activeFocus ? Theme.accent : "#E0E0E0"

        Row {
            anchors.fill: parent
            anchors.leftMargin: 15
            anchors.rightMargin: 8
            spacing: 10

            TextInput {
                id: searchInput
                width: parent.width - searchButton.width - parent.spacing - 30
                height: parent.height
                verticalAlignment: TextInput.AlignVCenter
                font.pixelSize: 14
                color: "#333333"
                selectByMouse: true
                selectionColor: Qt.rgba(0, 0.48, 0.8, 0.3)
                clip: true

                Text {
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    text: root.placeholderText
                    color: "#999999"
                    font.italic: true
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
                width: 32
                height: 32
                anchors.verticalCenter: parent.verticalCenter
                color: searchButtonArea.pressed
                       ? Qt.rgba(0, 0.48, 0.8, 0.2)
                       : (searchButtonArea.containsMouse ? Theme.accentSoft : "transparent")
                radius: 16

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
                    onClicked: { onSearchTriggered() }
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
