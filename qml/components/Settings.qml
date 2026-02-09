import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtQuick.Dialogs 1.3

Rectangle {
    id: root
    color: "#F5F5F5"
    
    signal chooseDownloadPath()
    signal settingsClosed()
    
    property string downloadPath: ""
    property bool downloadLyrics: true
    property bool downloadCover: false
    
    // 标题栏
    Rectangle {
        id: titleBar
        width: parent.width
        height: 60
        color: "#FFFFFF"
        
        Row {
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            
            Image {
                width: 32
                height: 32
                source: "qrc:/new/prefix1/icon/settings.png"
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: "设置"
                font.pixelSize: 20
                font.bold: true
                color: "#333333"
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        
        // 分割线
        Rectangle {
            width: parent.width
            height: 1
            color: "#E0E0E0"
            anchors.bottom: parent.bottom
        }
    }
    
    // 设置内容区域
    Flickable {
        id: contentFlickable
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: buttonBar.top
        anchors.margins: 0
        contentHeight: contentColumn.height
        clip: true
        
        Column {
            id: contentColumn
            width: parent.width
            spacing: 0
            
            // 下载设置分组
            Rectangle {
                width: parent.width
                height: 50
                color: "transparent"
                
                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 20
                    anchors.verticalCenter: parent.verticalCenter
                    text: "下载设置"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#2196F3"
                }
            }
            
            // 下载路径设置
            Rectangle {
                width: parent.width
                height: 80
                color: "#FFFFFF"
                
                Column {
                    anchors.fill: parent
                    anchors.leftMargin: 40
                    anchors.rightMargin: 40
                    anchors.topMargin: 15
                    anchors.bottomMargin: 15
                    spacing: 8
                    
                    Text {
                        text: "歌曲下载保存路径"
                        font.pixelSize: 14
                        color: "#333333"
                    }
                    
                    Row {
                        width: parent.width
                        spacing: 10
                        
                        Rectangle {
                            width: parent.width - chooseDirBtn.width - parent.spacing
                            height: 36
                            border.width: 1
                            border.color: "#CCCCCC"
                            radius: 4
                            color: "#F9F9F9"
                            
                            Text {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                verticalAlignment: Text.AlignVCenter
                                text: root.downloadPath || "D:/Music"
                                font.pixelSize: 13
                                color: "#666666"
                                elide: Text.ElideMiddle
                            }
                        }
                        
                        Button {
                            id: chooseDirBtn
                            width: 80
                            height: 36
                            text: "选择..."
                            
                            background: Rectangle {
                                color: parent.pressed ? "#1976D2" : (parent.hovered ? "#2196F3" : "#42A5F5")
                                radius: 4
                            }
                            
                            contentItem: Text {
                                text: parent.text
                                color: "#FFFFFF"
                                font.pixelSize: 13
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            onClicked: root.chooseDownloadPath()
                        }
                    }
                }
            }
            
            // 间隔
            Rectangle {
                width: parent.width
                height: 1
                color: "#E0E0E0"
            }
            
            // 下载选项
            Rectangle {
                width: parent.width
                height: 120
                color: "#FFFFFF"
                
                Column {
                    anchors.fill: parent
                    anchors.leftMargin: 40
                    anchors.rightMargin: 40
                    anchors.topMargin: 15
                    anchors.bottomMargin: 15
                    spacing: 15
                    
                    Text {
                        text: "下载选项"
                        font.pixelSize: 14
                        color: "#333333"
                    }
                    
                    // 下载歌词选项
                    Row {
                        spacing: 10
                        
                        CheckBox {
                            id: lyricsCheckBox
                            checked: root.downloadLyrics
                            
                            indicator: Rectangle {
                                implicitWidth: 20
                                implicitHeight: 20
                                radius: 3
                                border.width: 2
                                border.color: parent.checked ? "#2196F3" : "#CCCCCC"
                                color: parent.checked ? "#2196F3" : "#FFFFFF"
                                
                                Image {
                                    anchors.centerIn: parent
                                    width: 12
                                    height: 12
                                    source: "qrc:/new/prefix1/icon/check.png"
                                    visible: parent.parent.checked
                                }
                            }
                            
                            onCheckedChanged: {
                                root.downloadLyrics = checked
                            }
                        }
                        
                        Text {
                            text: "同时下载歌词文件"
                            font.pixelSize: 13
                            color: "#666666"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    
                    // 下载专辑图片选项
                    Row {
                        spacing: 10
                        
                        CheckBox {
                            id: coverCheckBox
                            checked: root.downloadCover
                            
                            indicator: Rectangle {
                                implicitWidth: 20
                                implicitHeight: 20
                                radius: 3
                                border.width: 2
                                border.color: parent.checked ? "#2196F3" : "#CCCCCC"
                                color: parent.checked ? "#2196F3" : "#FFFFFF"
                                
                                Image {
                                    anchors.centerIn: parent
                                    width: 12
                                    height: 12
                                    source: "qrc:/new/prefix1/icon/check.png"
                                    visible: parent.parent.checked
                                }
                            }
                            
                            onCheckedChanged: {
                                root.downloadCover = checked
                            }
                        }
                        
                        Text {
                            text: "同时下载专辑封面图片"
                            font.pixelSize: 13
                            color: "#666666"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }
    }
    
    // 底部按钮栏
    Rectangle {
        id: buttonBar
        width: parent.width
        height: 70
        color: "#FFFFFF"
        anchors.bottom: parent.bottom
        
        // 顶部分割线
        Rectangle {
            width: parent.width
            height: 1
            color: "#E0E0E0"
            anchors.top: parent.top
        }
        
        Row {
            anchors.centerIn: parent
            spacing: 20
            
            Button {
                width: 100
                height: 36
                text: "关闭"
                
                background: Rectangle {
                    color: parent.pressed ? "#E0E0E0" : (parent.hovered ? "#F0F0F0" : "#FFFFFF")
                    border.width: 1
                    border.color: "#CCCCCC"
                    radius: 4
                }
                
                contentItem: Text {
                    text: parent.text
                    color: "#666666"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: root.settingsClosed()
            }
        }
    }
    
    // 公共函数
    function setDownloadPath(path) {
        root.downloadPath = path
    }
    
    function setDownloadLyrics(enable) {
        root.downloadLyrics = enable
    }
    
    function setDownloadCover(enable) {
        root.downloadCover = enable
    }
}
