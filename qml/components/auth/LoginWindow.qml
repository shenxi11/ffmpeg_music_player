import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Window 2.14
import QtGraphicalEffects 1.14

Window {
    id: loginWindow

    width: 420
    height: 560
    visible: false
    flags: Qt.FramelessWindowHint | Qt.Window
    color: "transparent"

    signal loginRequested(string account, string password)
    signal quickLoginRequested(string account, string password)
    signal registerRequested(string account, string password, string username)
    signal resetPasswordRequested(string account, string newPassword)
    signal loginSuccess(string username)

    property bool isLoginMode: true
    property string statusMessage: ""
    property bool statusSuccess: false
    property string savedAccount: ""
    property string savedPassword: ""
    property string savedUsername: ""

    Rectangle {
        id: mainContainer
        anchors.fill: parent
        anchors.margins: 8
        radius: 8
        color: "#ffffff"
        border.width: 1
        border.color: "#e0e0e0"

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

                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: closeButton.width + 15
                    onPressed: loginWindow.startSystemMove()
                }

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 15

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
                            color: closeButton.pressed ? "#e0e0e0" : closeButton.hovered ? "#f5f5f5" : "transparent"
                            radius: 4
                        }

                        contentItem: Text {
                            text: closeButton.text
                            color: "#666666"
                            font.pixelSize: 20
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: loginWindow.visible = false
                    }
                }
            }

            Item {
                width: parent.width
                height: parent.height - titleBar.height

                Column {
                    anchors.fill: parent
                    anchors.margins: 35
                    anchors.topMargin: 30
                    anchors.bottomMargin: 25
                    spacing: 16

                    Column {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 8

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "♪"
                            font.pixelSize: 40
                            color: "#409EFF"
                            font.bold: true
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: isLoginMode ? "欢迎回来" : "创建账号"
                            color: "#333333"
                            font.pixelSize: 20
                            font.bold: true
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: isLoginMode ? "登录以继续使用" : "填写信息完成注册"
                            color: "#999999"
                            font.pixelSize: 13
                        }
                    }

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
                            leftPadding: 15
                            rightPadding: 15
                            color: "#333333"

                            background: Rectangle {
                                color: "#f5f5f5"
                                border.width: accountInput.activeFocus ? 2 : 1
                                border.color: accountInput.activeFocus ? "#409EFF" : "#e0e0e0"
                                radius: 4
                            }
                        }
                    }

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
                            leftPadding: 15
                            rightPadding: 15
                            color: "#333333"

                            background: Rectangle {
                                color: "#f5f5f5"
                                border.width: passwordInput.activeFocus ? 2 : 1
                                border.color: passwordInput.activeFocus ? "#409EFF" : "#e0e0e0"
                                radius: 4
                            }

                            Keys.onReturnPressed: {
                                if (isLoginMode) {
                                    onLoginClicked()
                                }
                            }
                        }
                    }

                    Row {
                        width: parent.width
                        visible: isLoginMode
                        spacing: 0

                        Item { width: parent.width - forgotPasswordText.implicitWidth; height: 1 }

                        Text {
                            id: forgotPasswordText
                            text: "忘记密码？"
                            color: "#409EFF"
                            font.pixelSize: 13

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    resetAccountInput.text = accountInput.text.trim()
                                    resetPasswordInput.text = ""
                                    resetStatusText.text = ""
                                    resetPopup.open()
                                }
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 58
                        radius: 6
                        visible: isLoginMode && savedAccount.length > 0
                        color: "#f7fbff"
                        border.width: 1
                        border.color: "#dbeafe"

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 10

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: "上次账号"
                                color: "#409EFF"
                                font.pixelSize: 12
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: savedUsername.length > 0 ? (savedUsername + " (" + savedAccount + ")") : savedAccount
                                color: "#333333"
                                font.pixelSize: 13
                                elide: Text.ElideRight
                                width: 170
                            }

                            Item { width: 1; height: 1 }

                            Button {
                                width: 90
                                height: 32
                                text: "一键登录"
                                anchors.verticalCenter: parent.verticalCenter

                                background: Rectangle {
                                    color: parent.pressed ? "#3a8ee6" : parent.hovered ? "#66b1ff" : "#409EFF"
                                    radius: 4
                                }

                                contentItem: Text {
                                    text: parent.text
                                    color: "#ffffff"
                                    font.pixelSize: 13
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                onClicked: {
                                    if (savedAccount.length === 0 || savedPassword.length === 0) {
                                        statusSuccess = false
                                        statusMessage = "缓存账号信息不完整，请手动登录"
                                        return
                                    }
                                    statusMessage = ""
                                    loginWindow.quickLoginRequested(savedAccount, savedPassword)
                                }
                            }
                        }
                    }

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
                            leftPadding: 15
                            rightPadding: 15
                            color: "#333333"

                            background: Rectangle {
                                color: "#f5f5f5"
                                border.width: usernameInput.activeFocus ? 2 : 1
                                border.color: usernameInput.activeFocus ? "#409EFF" : "#e0e0e0"
                                radius: 4
                            }

                            Keys.onReturnPressed: {
                                if (!isLoginMode) {
                                    onLoginClicked()
                                }
                            }
                        }
                    }

                    Button {
                        id: loginButton
                        width: parent.width
                        height: 42
                        text: isLoginMode ? "登录" : "注册"

                        background: Rectangle {
                            color: loginButton.pressed ? "#3a8ee6" : loginButton.hovered ? "#66b1ff" : "#409EFF"
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

                        onClicked: onLoginClicked()
                    }

                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        visible: statusMessage.length > 0
                        text: statusMessage
                        color: statusSuccess ? "#67C23A" : "#E74C3C"
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                    }

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
                                onEntered: parent.font.underline = true
                                onExited: parent.font.underline = false
                                onClicked: {
                                    isLoginMode = !isLoginMode
                                    usernameInput.text = ""
                                    statusMessage = ""
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: resetPopup
        modal: true
        focus: true
        width: 320
        height: 230
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: "#ffffff"
            radius: 8
            border.width: 1
            border.color: "#e0e0e0"
        }

        Column {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            Text {
                text: "重置密码"
                color: "#333333"
                font.pixelSize: 16
                font.bold: true
            }

            TextField {
                id: resetAccountInput
                width: parent.width
                height: 38
                placeholderText: "请输入账号"
                leftPadding: 12
                rightPadding: 12
                background: Rectangle {
                    color: "#f5f5f5"
                    radius: 4
                    border.width: 1
                    border.color: "#e0e0e0"
                }
            }

            TextField {
                id: resetPasswordInput
                width: parent.width
                height: 38
                placeholderText: "请输入新密码"
                echoMode: TextInput.Password
                leftPadding: 12
                rightPadding: 12
                background: Rectangle {
                    color: "#f5f5f5"
                    radius: 4
                    border.width: 1
                    border.color: "#e0e0e0"
                }
            }

            Text {
                id: resetStatusText
                width: parent.width
                visible: text.length > 0
                color: "#E74C3C"
                font.pixelSize: 12
                wrapMode: Text.Wrap
            }

            Row {
                spacing: 10
                anchors.horizontalCenter: parent.horizontalCenter

                Button {
                    text: "取消"
                    width: 120
                    onClicked: resetPopup.close()
                }

                Button {
                    text: "确认重置"
                    width: 120
                    onClicked: {
                        var account = resetAccountInput.text.trim()
                        var newPwd = resetPasswordInput.text.trim()
                        if (account === "" || newPwd === "") {
                            resetStatusText.text = "账号和新密码不能为空"
                            return
                        }
                        resetStatusText.text = ""
                        loginWindow.resetPasswordRequested(account, newPwd)
                    }
                }
            }
        }
    }

    function onLoginClicked() {
        var account = accountInput.text.trim()
        var password = passwordInput.text.trim()

        if (account === "" || password === "") {
            statusSuccess = false
            statusMessage = "账号或密码不能为空"
            return
        }

        if (isLoginMode) {
            statusMessage = ""
            loginWindow.loginRequested(account, password)
        } else {
            var username = usernameInput.text.trim()
            if (username === "") {
                statusSuccess = false
                statusMessage = "用户名不能为空"
                return
            }
            statusMessage = ""
            loginWindow.registerRequested(account, password, username)
        }
    }

    function clearInputs() {
        accountInput.text = ""
        passwordInput.text = ""
        usernameInput.text = ""
    }

    function setSavedAccount(account, password, username) {
        savedAccount = account ? account : ""
        savedPassword = password ? password : ""
        savedUsername = username ? username : ""
        if (savedAccount.length > 0) {
            accountInput.text = savedAccount
            passwordInput.text = savedPassword
        }
    }

    function onLoginFailed(message) {
        statusSuccess = false
        statusMessage = (message && message.length > 0) ? message : "登录失败"
    }

    function switchToLoginMode() {
        isLoginMode = true
        usernameInput.text = ""
    }

    function onResetPasswordResult(success, message) {
        if (success) {
            resetPopup.close()
            statusSuccess = true
            statusMessage = (message && message.length > 0) ? message : "密码重置成功，请登录"
        } else {
            resetStatusText.text = (message && message.length > 0) ? message : "密码重置失败"
        }
    }

    function onRegisterResult(success, message) {
        statusSuccess = success
        if (success) {
            statusMessage = (message && message.length > 0) ? message : "注册成功，请登录"
        } else {
            statusMessage = (message && message.length > 0) ? message : "注册失败"
        }
    }
}
