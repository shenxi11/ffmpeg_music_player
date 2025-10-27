import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Window 2.14
import QtGraphicalEffects 1.14

Window {
    id: loginWindow
    
    width: 450
    height: 420
    visible: false
    flags: Qt.FramelessWindowHint | Qt.Window
    color: "transparent"
    
    // 信号定义
    signal loginRequested(string account, string password)
    signal registerRequested(string account, string password, string username)
    signal loginSuccess(string username)
    
    // 属性
    property bool isLoginMode: true
    
    // 主容器
    Rectangle {
        id: mainContainer
        anchors.fill: parent
        anchors.margins: 10
        radius: 15
        
        // 渐变背景
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#667eea" }
            GradientStop { position: 1.0; color: "#764ba2" }
        }
        
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.2)
        
        // 阴影效果
        layer.enabled: true
        layer.effect: DropShadow {
            transparentBorder: true
            horizontalOffset: 0
            verticalOffset: 5
            radius: 20
            samples: 41
            color: Qt.rgba(0, 0, 0, 0.3)
        }
        
        Column {
            anchors.fill: parent
            spacing: 0
            
            // 标题栏
            Rectangle {
                id: titleBar
                width: parent.width
                height: 50
                color: Qt.rgba(1, 1, 1, 0.1)
                radius: 15
                
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: parent.height / 2
                    color: parent.color
                }
                
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: Qt.rgba(1, 1, 1, 0.2)
                }
                
                // 拖动区域
                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: closeButton.width + 15
                    
                    onPressed: {
                        // 使用 Qt 的标准窗口移动方法
                        loginWindow.startSystemMove()
                    }
                }
                
                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    spacing: 0
                    
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "🎵 网易云音乐"
                        color: "white"
                        font.pixelSize: 18
                        font.bold: true
                    }
                    
                    Item { 
                        width: parent.width - 200
                        height: parent.height
                    }
                    
                    Button {
                        id: closeButton
                        anchors.verticalCenter: parent.verticalCenter
                        width: 35
                        height: 35
                        text: "✕"
                        
                        background: Rectangle {
                            color: closeButton.pressed ? Qt.rgba(1, 0.28, 0.4, 1) : 
                                   closeButton.hovered ? Qt.rgba(1, 0.28, 0.4, 0.8) : 
                                   Qt.rgba(1, 1, 1, 0.1)
                            radius: 17
                        }
                        
                        contentItem: Text {
                            text: closeButton.text
                            color: "white"
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        onClicked: {
                            loginWindow.visible = false
                        }
                    }
                }
            }
            
            // 内容区域
            Item {
                width: parent.width
                height: parent.height - titleBar.height
                
                Column {
                    anchors.fill: parent
                    anchors.margins: 30
                    anchors.topMargin: 10
                    anchors.bottomMargin: 20
                    spacing: 12
                    
                    // Logo
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "🎵"
                        font.pixelSize: 40
                        color: "white"
                    }
                    
                    // 欢迎文字
                    Text {
                        id: welcomeText
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: isLoginMode ? "欢迎登录" : "创建新账号"
                        color: "white"
                        font.pixelSize: 16
                        font.bold: true
                    }
                    
                    Item { height: 5 }
                    
                    // 账号输入框
                    TextField {
                        id: accountInput
                        width: parent.width
                        height: 45
                        placeholderText: "📧 请输入账号"
                        font.pixelSize: 14
                        selectByMouse: true
                        
                        background: Rectangle {
                            color: accountInput.activeFocus ? "white" : Qt.rgba(1, 1, 1, 0.9)
                            border.width: 2
                            border.color: accountInput.activeFocus ? Qt.rgba(1, 0.28, 0.4, 0.8) : Qt.rgba(1, 1, 1, 0.3)
                            radius: 8
                        }
                        
                        leftPadding: 15
                        rightPadding: 15
                    }
                    
                    // 密码输入框
                    TextField {
                        id: passwordInput
                        width: parent.width
                        height: 45
                        placeholderText: "🔒 请输入密码"
                        echoMode: TextInput.Password
                        font.pixelSize: 14
                        selectByMouse: true
                        
                        background: Rectangle {
                            color: passwordInput.activeFocus ? "white" : Qt.rgba(1, 1, 1, 0.9)
                            border.width: 2
                            border.color: passwordInput.activeFocus ? Qt.rgba(1, 0.28, 0.4, 0.8) : Qt.rgba(1, 1, 1, 0.3)
                            radius: 8
                        }
                        
                        leftPadding: 15
                        rightPadding: 15
                        
                        Keys.onReturnPressed: {
                            if (isLoginMode) {
                                onLoginClicked()
                            }
                        }
                    }
                    
                    // 用户名输入框（注册时显示）
                    TextField {
                        id: usernameInput
                        width: parent.width
                        height: 45
                        placeholderText: "👤 请输入用户名"
                        font.pixelSize: 14
                        selectByMouse: true
                        visible: !isLoginMode
                        
                        background: Rectangle {
                            color: usernameInput.activeFocus ? "white" : Qt.rgba(1, 1, 1, 0.9)
                            border.width: 2
                            border.color: usernameInput.activeFocus ? Qt.rgba(1, 0.28, 0.4, 0.8) : Qt.rgba(1, 1, 1, 0.3)
                            radius: 8
                        }
                        
                        leftPadding: 15
                        rightPadding: 15
                        
                        Keys.onReturnPressed: {
                            if (!isLoginMode) {
                                onLoginClicked()
                            }
                        }
                    }
                    
                    // 登录/注册按钮
                    Button {
                        id: loginButton
                        width: parent.width
                        height: 45
                        text: isLoginMode ? "登录" : "注册"
                        
                        background: Rectangle {
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: loginButton.pressed ? "#e53935" : loginButton.hovered ? "#ff5252" : "#ff6b6b" }
                                GradientStop { position: 1.0; color: loginButton.pressed ? "#d32f2f" : loginButton.hovered ? "#e53935" : "#ee5a52" }
                            }
                            radius: 8
                        }
                        
                        contentItem: Text {
                            text: loginButton.text
                            color: "white"
                            font.pixelSize: 16
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        onClicked: {
                            onLoginClicked()
                        }
                    }
                    
                    // 切换登录/注册
                    Text {
                        id: switchModeText
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: isLoginMode ? "还没有账号？<u>点击注册</u>" : "已有账号？<u>点击登录</u>"
                        color: Qt.rgba(1, 1, 1, 0.8)
                        font.pixelSize: 13
                        textFormat: Text.RichText
                        
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            hoverEnabled: true
                            onEntered: {
                                parent.color = "white"
                            }
                            onExited: {
                                parent.color = Qt.rgba(1, 1, 1, 0.8)
                            }
                            onClicked: {
                                isLoginMode = !isLoginMode
                                usernameInput.text = ""
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 登录/注册处理函数
    function onLoginClicked() {
        var account = accountInput.text.trim()
        var password = passwordInput.text.trim()
        
        if (account === "" || password === "") {
            console.log("账号或密码不能为空！")
            return
        }
        
        if (isLoginMode) {
            console.log("尝试登录:", account)
            loginWindow.loginRequested(account, password)
        } else {
            var username = usernameInput.text.trim()
            if (username === "") {
                console.log("用户名不能为空！")
                return
            }
            console.log("尝试注册:", account, username)
            loginWindow.registerRequested(account, password, username)
        }
    }
    
    // 清空输入框
    function clearInputs() {
        accountInput.text = ""
        passwordInput.text = ""
        usernameInput.text = ""
    }
    
    // 切换到登录模式
    function switchToLoginMode() {
        isLoginMode = true
        usernameInput.text = ""
    }
}
