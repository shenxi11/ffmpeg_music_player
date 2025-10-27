import QtQuick 2.14
import QtQuick.Controls 2.14
import QtGraphicalEffects 1.14
import QtQuick.Window 2.14

// 用户弹出菜单窗口（独立窗口，避免被 QQuickWidget 裁剪）
Window {
    id: popupWindow
    
    width: 180
    height: 140
    flags: Qt.Popup | Qt.FramelessWindowHint | Qt.NoDropShadowWindowHint
    color: "transparent"
    
    // 属性
    property bool isLoggedIn: false
    property string username: "未登录"
    property string avatarSource: "qrc:/new/prefix1/icon/denglu.png"
    
    // 信号
    signal loginRequested()
    signal logoutRequested()
    
    // 点击外部区域关闭
    onActiveFocusItemChanged: {
        if (!activeFocusItem) {
            popupWindow.close()
        }
    }
    
    // 主容器
    Rectangle {
        anchors.fill: parent
        radius: 12
        color: "white"
        border.width: 1
        border.color: "#DCDCDC"
        
        // 阴影效果
        layer.enabled: true
        layer.effect: DropShadow {
            transparentBorder: true
            horizontalOffset: 0
            verticalOffset: 3
            radius: 12
            samples: 25
            color: Qt.rgba(0, 0, 0, 0.15)
        }
        
        // 菜单内容
        Column {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10
            
            // 用户信息区域
            Row {
                width: parent.width
                height: 60
                spacing: 10
                
                // 用户头像
                Rectangle {
                    width: 50
                    height: 50
                    radius: 25
                    color: "#F0F0F0"
                    
                    Image {
                        id: avatarImage
                        anchors.fill: parent
                        anchors.margins: 2
                        source: popupWindow.avatarSource
                        fillMode: Image.PreserveAspectCrop
                        
                        layer.enabled: true
                        layer.effect: OpacityMask {
                            maskSource: Rectangle {
                                width: avatarImage.width
                                height: avatarImage.height
                                radius: width / 2
                            }
                        }
                    }
                }
                
                // 用户名和状态
                Column {
                    width: parent.width - 60
                    height: parent.height
                    spacing: 5
                    
                    Text {
                        text: popupWindow.username
                        font.pixelSize: 16
                        font.bold: true
                        color: "#333333"
                        elide: Text.ElideRight
                        width: parent.width
                    }
                    
                    Text {
                        text: popupWindow.isLoggedIn ? "在线" : "未登录"
                        font.pixelSize: 12
                        color: popupWindow.isLoggedIn ? "#00CC66" : "#999999"
                    }
                }
            }
            
            // 分隔线
            Rectangle {
                width: parent.width
                height: 1
                color: "#EEEEEE"
            }
            
            // 登录/退出按钮
            Button {
                width: parent.width
                height: 36
                
                background: Rectangle {
                    color: parent.hovered ? Qt.rgba(0, 0.48, 0.8, 0.1) : "transparent"
                    radius: 8
                    border.width: 1
                    border.color: Qt.rgba(0, 0.48, 0.8, 0.3)
                }
                
                contentItem: Text {
                    text: popupWindow.isLoggedIn ? "退出登录" : "登录"
                    font.pixelSize: 14
                    color: Qt.rgba(0, 0.48, 0.8, 1.0)
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    if (popupWindow.isLoggedIn) {
                        popupWindow.logoutRequested()
                    } else {
                        popupWindow.loginRequested()
                    }
                    popupWindow.close()
                }
            }
        }
    }
}
