import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Window 2.14
import QtGraphicalEffects 1.14

Window {
    id: loginWindow
    
    width: 420
    height: 540
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
        anchors.margins: 8
        radius: 8
        
        // 简洁的白色背景
        color: "#ffffff"
        
        border.width: 1
        border.color: "#e0e0e0"
        
        // 阴影效果
        layer.enabled: true
        layer.effect: DropShadow {
            transparentBorder: true
            horizontalOffset: 0
            verticalOffset: 4
            radius: 16
            samples: 33
            color: Qt.rgba(0, 0, 0, 0.15)
        }
        
        Column {
            anchors.fill: parent
            spacing: 0
            
            // 标题栏
            Rectangle {
                id: titleBar
                width: parent.width
                height: 50
                color: "#fafafa"
                radius: 8
                
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
                    color: "#e0e0e0"
                }
                
                // 拖动区域
                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: closeButton.width + 15
                    
                    onPressed: {
                        loginWindow.startSystemMove()
                    }
                }
                
                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 15
                    spacing: 0
                    
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: isLoginMode ? "登录" : "注册"
                        color: "#333333"
                        font.pixelSize: 16
                        font.bold: true
                    }
                    
                    Item { 
                        width: parent.width - 120
                        height: parent.height
                    }
                    
                    Button {
                        id: closeButton
                        anchors.verticalCenter: parent.verticalCenter
                        width: 32
                        height: 32
                        text: "×"
                        
                        background: Rectangle {
                            color: closeButton.pressed ? "#e0e0e0" : 
                                   closeButton.hovered ? "#f5f5f5" : 
                                   "transparent"
                            radius: 4
                        }
                        
                        contentItem: Text {
                            text: closeButton.text
                            color: "#666666"
                            font.pixelSize: 20
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
                    anchors.margins: 35
                    anchors.topMargin: 30
                    anchors.bottomMargin: 25
                    spacing: 18
                    
                    // Logo 和欢迎文字
                    Column {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 8
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "♪"
                            font.pixelSize: 42
                            color: "#409EFF"
                            font.bold: true
                        }
                        
                        Text {
                            id: welcomeText
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: isLoginMode ? "欢迎回来" : "创建账号"
                            color: "#333333"
                            font.pixelSize: 20
                            font.bold: true
                        }
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: isLoginMode ? "登录以使用完整功能" : "注册新用户账号"
                            color: "#999999"
                            font.pixelSize: 13
                        }
                    }
                    
                    Item { height: 5 }
                    
                    // 账号输入框
                    Column {
                        width: parent.width
                        spacing: 8
                        
                        Text {
                            text: "账号"
                            color: "#666666"
                            font.pixelSize: 13
                        }
                        
                        TextField {
                            id: accountInput
                            width: parent.width
                            height: 42
                            placeholderText: "请输入账号"
                            font.pixelSize: 14
                            selectByMouse: true
                            
                            background: Rectangle {
                                color: "#f5f5f5"
                                border.width: accountInput.activeFocus ? 2 : 1
                                border.color: accountInput.activeFocus ? "#409EFF" : "#e0e0e0"
                                radius: 4
                            }
                            
                            leftPadding: 15
                            rightPadding: 15
                            color: "#333333"
                        }
                    }
                    
                    // 密码输入框
                    Column {
                        width: parent.width
                        spacing: 8
                        
                        Text {
                            text: "密码"
                            color: "#666666"
                            font.pixelSize: 13
                        }
                        
                        TextField {
                            id: passwordInput
                            width: parent.width
                            height: 42
                            placeholderText: "请输入密码"
                            echoMode: TextInput.Password
                            font.pixelSize: 14
                            selectByMouse: true
                            
                            background: Rectangle {
                                color: "#f5f5f5"
                                border.width: passwordInput.activeFocus ? 2 : 1
                                border.color: passwordInput.activeFocus ? "#409EFF" : "#e0e0e0"
                                radius: 4
                            }
                            
                            leftPadding: 15
                            rightPadding: 15
                            color: "#333333"
                            
                            Keys.onReturnPressed: {
                                if (isLoginMode) {
                                    onLoginClicked()
                                }
                            }
                        }
                    }
                    
                    // 用户名输入框（注册时显示）
                    Column {
                        width: parent.width
                        spacing: 8
                        visible: !isLoginMode
                        
                        Text {
                            text: "用户名"
                            color: "#666666"
                            font.pixelSize: 13
                        }
                        
                        TextField {
                            id: usernameInput
                            width: parent.width
                            height: 42
                            placeholderText: "请输入用户名"
                            font.pixelSize: 14
                            selectByMouse: true
                            
                            background: Rectangle {
                                color: "#f5f5f5"
                                border.width: usernameInput.activeFocus ? 2 : 1
                                border.color: usernameInput.activeFocus ? "#409EFF" : "#e0e0e0"
                                radius: 4
                            }
                            
                            leftPadding: 15
                            rightPadding: 15
                            color: "#333333"
                            
                            Keys.onReturnPressed: {
                                if (!isLoginMode) {
                                    onLoginClicked()
                                }
                            }
                        }
                    }
                    
                    Item { height: 5 }
                    
                    // 登录/注册按钮
                    Button {
                        id: loginButton
                        width: parent.width
                        height: 42
                        text: isLoginMode ? "登录" : "注册"
                        
                        background: Rectangle {
                            color: loginButton.pressed ? "#3a8ee6" : 
                                   loginButton.hovered ? "#66b1ff" : "#409EFF"
                            radius: 4
                        }
                        
                        contentItem: Text {
                            text: loginButton.text
                            color: "white"
                            font.pixelSize: 15
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        onClicked: {
                            onLoginClicked()
                        }
                    }
                    
                    // 切换登录/注册
                    Row {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 5
                        
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: isLoginMode ? "还没有账号？" : "已有账号？"
                            color: "#999999"
                            font.pixelSize: 13
                        }
                        
                        Text {
                            id: switchModeText
                            anchors.verticalCenter: parent.verticalCenter
                            text: isLoginMode ? "立即注册" : "立即登录"
                            color: "#409EFF"
                            font.pixelSize: 13
                            
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                hoverEnabled: true
                                onEntered: {
                                    parent.font.underline = true
                                }
                                onExited: {
                                    parent.font.underline = false
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
