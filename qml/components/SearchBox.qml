import QtQuick 2.14
import QtQuick.Controls 2.14

Rectangle {
    id: root
    implicitWidth: 250
    implicitHeight: 60
    color: "transparent"
    
    property string placeholderText: " 搜索想听的歌曲吧..."
    property alias text: searchInput.text
    
    signal search(string text)
    signal searchAll()
    
    Rectangle {
        anchors.fill: parent
        anchors.margins: 5
        color: "#FFFFFF"
        radius: 8
        border.width: 1
        border.color: searchInput.activeFocus ? "#2196F3" : "#E0E0E0"
        
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
                
                Keys.onReturnPressed: { onSearchTriggered() }
                Keys.onEnterPressed: { onSearchTriggered() }
            }
            
            Rectangle {
                id: searchButton
                width: 32
                height: 32
                anchors.verticalCenter: parent.verticalCenter
                color: searchButtonArea.pressed ? Qt.rgba(0, 0.48, 0.8, 0.2) : 
                       searchButtonArea.containsMouse ? Qt.rgba(0, 0.48, 0.8, 0.1) : 
                       "transparent"
                radius: 16
                
                Image {
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    source: "qrc:/new/prefix1/icon/search.png"
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
        if (text === "") {
            root.searchAll()
        } else {
            root.search(text)
        }
    }
    
    function clear() {
        searchInput.text = ""
    }
}