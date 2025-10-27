import QtQuick 2.14
import QtQuick.Controls 2.14
import QtGraphicalEffects 1.14
import QtQuick.Window 2.14

// QML 版本的 UserWidget
Rectangle {
    id: root
    
    width: 150
    height: 40
    color: hoverArea.containsMouse ? Qt.rgba(0, 0.48, 0.8, 0.1) : "transparent"
    radius: hoverArea.containsMouse ? 20 : 0
    border.width: hoverArea.containsMouse ? 1 : 0
    border.color: Qt.rgba(0, 0.48, 0.8, 0.3)
    clip: false  // 允许弹出菜单溢出显示
    
    // 属性
    property bool isLoggedIn: false
    property string username: "未登录"
    property string avatarSource: "qrc:/new/prefix1/icon/denglu.png"
    
    // 信号
    signal loginRequested()
    signal logoutRequested()
    
    // 动画
    Behavior on color {
        ColorAnimation { duration: 200 }
    }
    
    Behavior on radius {
        NumberAnimation { duration: 200 }
    }
    
    // 主布局
    Row {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 10
        
        // 头像
        Image {
            id: avatarImage
            width: 32
            height: 32
            anchors.verticalCenter: parent.verticalCenter
            source: root.avatarSource
            fillMode: Image.PreserveAspectFit
            smooth: true
            
            // 圆形遮罩
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: Rectangle {
                    width: avatarImage.width
                    height: avatarImage.height
                    radius: width / 2
                }
            }
        }
        
        // 用户名
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.username
            color: "#333333"
            font.pixelSize: 14
            font.bold: true
        }
    }
    
    // 鼠标悬停区域
    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        
        onEntered: {
            showTimer.start()
        }
        
        onExited: {
            showTimer.stop()
            hideTimer.start()
        }
        
        onClicked: {
            console.log("UserWidget clicked! popupMenu.visible =", popupMenu.visible)
            if (popupMenu.visible) {
                hidePopup()
            } else {
                showPopup()
            }
        }
    }
    
    // 显示延迟定时器
    Timer {
        id: showTimer
        interval: 300
        onTriggered: {
            showPopup()
        }
    }
    
    // 隐藏延迟定时器
    Timer {
        id: hideTimer
        interval: 200
        onTriggered: {
            if (!popupMenu.hovered && !hoverArea.containsMouse) {
                hidePopup()
            }
        }
    }
    
    // 弹出菜单（使用 Rectangle 代替 Popup，避免 QQuickWidget 裁剪问题）
    Rectangle {
        id: popupMenu
        width: 180
        height: 140
        visible: false  // 默认隐藏
        z: 1000  // 确保在最上层
        
        // 定位在主控件下方
        y: root.height + 5
        x: root.width - width
        
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
        
        property bool hovered: popupMouseArea.containsMouse
        
        Column {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8
            
            // 头像
            Image {
                id: popupAvatar
                width: 50
                height: 50
                anchors.horizontalCenter: parent.horizontalCenter
                source: root.avatarSource
                fillMode: Image.PreserveAspectFit
                smooth: true
                
                // 圆形遮罩
                layer.enabled: true
                layer.effect: OpacityMask {
                    maskSource: Rectangle {
                        width: popupAvatar.width
                        height: popupAvatar.height
                        radius: width / 2
                    }
                }
            }
            
            // 用户名
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.username
                color: "#333333"
                font.pixelSize: 13
                font.bold: true
            }
            
            // 状态标签
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.isLoggedIn ? "已登录" : "点击下方按钮登录"
                color: "#666666"
                font.pixelSize: 10
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
            
            // 操作按钮
            Button {
                id: actionButton
                width: parent.width
                height: 28
                text: root.isLoggedIn ? "退出登录" : "我要登录/注册"
                
                background: Rectangle {
                    gradient: Gradient {
                        GradientStop { 
                            position: 0.0
                            color: root.isLoggedIn ? 
                                   (actionButton.pressed ? "#c82333" : actionButton.hovered ? "#e74c3c" : "#dc3545") :
                                   (actionButton.pressed ? "#005fa3" : actionButton.hovered ? "#0086e6" : "#007acc")
                        }
                        GradientStop { 
                            position: 1.0
                            color: root.isLoggedIn ?
                                   (actionButton.pressed ? "#a71e2a" : actionButton.hovered ? "#d62c1a" : "#c82333") :
                                   (actionButton.pressed ? "#004d82" : actionButton.hovered ? "#006bb3" : "#005fa3")
                        }
                    }
                    radius: 14
                }
                
                contentItem: Text {
                    text: actionButton.text
                    color: "white"
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    if (root.isLoggedIn) {
                        root.logoutRequested()
                    } else {
                        root.loginRequested()
                    }
                    hidePopup()
                }
            }
        }
        
        // 弹出窗口鼠标区域
        MouseArea {
            id: popupMouseArea
            anchors.fill: parent
            hoverEnabled: true
            
            onExited: {
                if (!hoverArea.containsMouse) {
                    hideTimer.start()
                }
            }
        }
    }
    
    // 显示弹出窗口
    function showPopup() {
        console.log("showPopup called")
        popupMenu.visible = true
        console.log("popupMenu visible =", popupMenu.visible)
    }
    
    // 隐藏弹出窗口
    function hidePopup() {
        console.log("hidePopup called")
        popupMenu.visible = false
    }
    
    // 设置用户信息
    function setUserInfo(username, avatarPath) {
        root.username = username
        if (avatarPath && avatarPath.length > 0) {
            root.avatarSource = avatarPath
        }
    }
    
    // 设置登录状态
    function setLoginState(loggedIn) {
        root.isLoggedIn = loggedIn
        if (!loggedIn) {
            root.username = "未登录"
            root.avatarSource = "qrc:/new/prefix1/icon/denglu.png"
        }
    }
}
