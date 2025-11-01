import QtQuick 2.14
import QtQuick.Controls 2.14

Item {
    id: root
    width: 150
    height: 80
    
    signal upClicked()
    
    property string songName: "暂无歌曲"
    property string picPath: "qrc:/new/prefix1/icon/pian.png"
    
    Rectangle {
        anchors.fill: parent
        color: "#FAFAFA"
        
        Row {
            anchors.fill: parent
            anchors.margins: 2
            spacing: 5
            
            // 封面图片
            Image {
                id: coverImage
                width: 76  // 80 - 4 (margins)
                height: 76
                source: root.picPath
                fillMode: Image.PreserveAspectFit
                smooth: true
                antialiasing: true
            }
            
            // 歌曲名称
            Text {
                id: nameText
                width: 65  // 150 - 80 - 5 = 65
                height: parent.height
                text: root.songName
                wrapMode: Text.WordWrap
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                font.pixelSize: 12
                color: "#333333"
                elide: Text.ElideRight
                maximumLineCount: 3
            }
        }
        
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                root.upClicked()
            }
        }
    }
    
    // 公开给 C++ 调用的函数
    function setName(name) {
        root.songName = name
    }
    
    function setPicPath(path) {
        root.picPath = path
    }
}
