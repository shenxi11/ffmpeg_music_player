import QtQuick 2.14
import QtQuick.Controls 2.14

Rectangle {
    id: root
    width: 1200
    height: 700
    
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#667eea" }
        GradientStop { position: 0.5; color: "#764ba2" }
        GradientStop { position: 1.0; color: "#f093fb" }
    }
    
    signal minimizeClicked()
    signal maximizeClicked()
    signal closeClicked()
    signal menuClicked()
    signal localMusicClicked()
    signal onlineMusicClicked()
    
    property bool isMaximized: false
    property string currentView: "local"
    
    Rectangle {
        id: titleBar
        width: parent.width
        height: 60
        color: "transparent"
        
        Row {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 15
            spacing: 15
            
            Item {
                width: 180
                height: parent.height
                
                Row {
                    anchors.centerIn: parent
                    spacing: 10
                    
                    Image {
                        width: 36
                        height: 36
                        source: "qrc:/new/prefix1/icon/netease.ico"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    
                    Text {
                        text: "网易云音乐"
                        font.family: "Microsoft YaHei"
                        font.pixelSize: 18
                        font.bold: true
                        color: "white"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
            
            Item { width: 250; height: parent.height }
            Item { width: 200; height: parent.height }
            Item { width: 150; height: parent.height }
            
            Rectangle {
                width: 40
                height: 40
                radius: 20
                color: menuBtn.containsMouse ? "#4DFFFFFF" : "#26FFFFFF"
                anchors.verticalCenter: parent.verticalCenter
                
                Text {
                    text: "☰"
                    font.pixelSize: 20
                    color: "white"
                    anchors.centerIn: parent
                }
                
                MouseArea {
                    id: menuBtn
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.menuClicked()
                }
            }
            
            Row {
                spacing: 8
                anchors.verticalCenter: parent.verticalCenter
                
                Rectangle {
                    width: 36
                    height: 36
                    radius: 18
                    color: minBtn.containsMouse ? "#4DFFFFFF" : "transparent"
                    Text {
                        text: "─"
                        font.pixelSize: 16
                        color: "white"
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: -2
                    }
                    MouseArea {
                        id: minBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.minimizeClicked()
                    }
                }
                
                Rectangle {
                    width: 36
                    height: 36
                    radius: 18
                    color: maxBtn.containsMouse ? "#4DFFFFFF" : "transparent"
                    Text {
                        text: root.isMaximized ? "❐" : "□"
                        font.pixelSize: 14
                        color: "white"
                        anchors.centerIn: parent
                    }
                    MouseArea {
                        id: maxBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.maximizeClicked()
                    }
                }
                
                Rectangle {
                    width: 36
                    height: 36
                    radius: 18
                    color: closeBtn.containsMouse ? "#E74C3C" : "transparent"
                    Text {
                        text: "✕"
                        font.pixelSize: 16
                        color: "white"
                        anchors.centerIn: parent
                    }
                    MouseArea {
                        id: closeBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.closeClicked()
                    }
                }
            }
        }
    }
    
    Item {
        id: mainContent
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: controlBar.top
        
        Rectangle {
            id: leftPanel
            width: 220
            height: parent.height
            color: "#4D000000"
            
            Column {
                anchors.fill: parent
                anchors.topMargin: 30
                spacing: 8
                
                Rectangle {
                    width: parent.width
                    height: 55
                    color: root.currentView === "local" ? "#40FFFFFF" : (localMusicBtn.containsMouse ? "#1AFFFFFF" : "transparent")
                    
                    Rectangle {
                        visible: root.currentView === "local"
                        width: 4
                        height: parent.height * 0.6
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        color: "white"
                        radius: 2
                    }
                    
                    Row {
                        anchors.centerIn: parent
                        spacing: 15
                        Text {
                            text: "♪"
                            font.pixelSize: 24
                            color: "white"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: "本地音乐"
                            font.family: "Microsoft YaHei"
                            font.pixelSize: 16
                            color: "white"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    
                    MouseArea {
                        id: localMusicBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.currentView = "local"
                            root.localMusicClicked()
                        }
                    }
                }
                
                Rectangle {
                    width: parent.width
                    height: 55
                    color: root.currentView === "online" ? "#40FFFFFF" : (onlineMusicBtn.containsMouse ? "#1AFFFFFF" : "transparent")
                    
                    Rectangle {
                        visible: root.currentView === "online"
                        width: 4
                        height: parent.height * 0.6
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        color: "white"
                        radius: 2
                    }
                    
                    Row {
                        anchors.centerIn: parent
                        spacing: 15
                        Text {
                            text: "☁"
                            font.pixelSize: 24
                            color: "white"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: "在线音乐"
                            font.family: "Microsoft YaHei"
                            font.pixelSize: 16
                            color: "white"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    
                    MouseArea {
                        id: onlineMusicBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.currentView = "online"
                            root.onlineMusicClicked()
                        }
                    }
                }
            }
        }
    }
    
    Rectangle {
        id: controlBar
        width: parent.width
        height: 100
        anchors.bottom: parent.bottom
        color: "#66000000"
    }
    
    function setMaximizedState(maximized) {
        root.isMaximized = maximized
    }
    
    function setCurrentView(view) {
        root.currentView = view
    }
}
