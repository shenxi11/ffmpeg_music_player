import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Window 2.14
import QtGraphicalEffects 1.14

Window {
    id: loginWindow

    width: 460
    height: 620
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

    readonly property color brandRed: "#EC4141"
    readonly property color brandRedDark: "#D83939"
    readonly property color textPrimary: "#1F2329"
    readonly property color textSecondary: "#6F7785"
    readonly property color borderColor: "#F1D8D8"
    readonly property color panelBg: "#FFF8F8"

    Rectangle {
        id: shell
        anchors.fill: parent
        anchors.margins: 10
        radius: 16
        color: "#FFFFFF"
        border.width: 1
        border.color: borderColor

        layer.enabled: true
        layer.effect: DropShadow {
            transparentBorder: true
            horizontalOffset: 0
            verticalOffset: 8
            radius: 20
            samples: 40
            color: Qt.rgba(0, 0, 0, 0.18)
        }

        Rectangle {
            id: headBg
            width: parent.width
            height: 178
            radius: shell.radius
            color: "transparent"

            gradient: Gradient {
                GradientStop { position: 0.0; color: "#FFF1F1" }
                GradientStop { position: 0.55; color: "#FFE9E9" }
                GradientStop { position: 1.0; color: "#FFF8F8" }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 30
                color: "#FFFFFF"
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: "#F3E3E3"
            }

            MouseArea {
                anchors.fill: parent
                anchors.rightMargin: 54
                onPressed: loginWindow.startSystemMove()
            }

            Rectangle {
                id: closeBtnBg
                width: 32
                height: 32
                radius: 16
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 16
                anchors.rightMargin: 16
                color: closeArea.pressed ? "#F7DADA" : (closeArea.containsMouse ? "#FCECEC" : "transparent")
                scale: closeArea.pressed ? 0.94 : (closeArea.containsMouse ? 1.04 : 1.0)

                Behavior on color {
                    ColorAnimation { duration: 120 }
                }
                Behavior on scale {
                    NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
                }

                Text {
                    anchors.centerIn: parent
                    text: "\u00D7"
                    color: "#7A818E"
                    font.pixelSize: 20
                }

                MouseArea {
                    id: closeArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: loginWindow.visible = false
                }
            }

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 24
                anchors.top: parent.top
                anchors.topMargin: 24
                spacing: 12

                Rectangle {
                    width: 50
                    height: 50
                    radius: 14
                    color: brandRed

                    Image {
                        anchors.centerIn: parent
                        width: 30
                        height: 30
                        source: "qrc:/design/design_exports/netease_ui_pack_20260309/icon/ui/brand/brand_logo_light@2x.png"
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4

                    Text {
                        text: "\u4e91\u97f3\u4e50"
                        color: textPrimary
                        font.pixelSize: 24
                        font.bold: true
                    }

                    Text {
                        text: isLoginMode
                              ? "\u767b\u5f55\u4f60\u7684\u79c1\u57df\u97f3\u4e50\u7a7a\u95f4"
                              : "\u521b\u5efa\u65b0\u7684\u79c1\u57df\u8d26\u53f7"
                        color: textSecondary
                        font.pixelSize: 13
                    }
                }
            }

            Rectangle {
                width: 88
                height: 24
                radius: 12
                anchors.left: parent.left
                anchors.leftMargin: 24
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 16
                color: brandRed

                Text {
                    anchors.centerIn: parent
                    text: "\u7f51\u6613\u4e91\u98ce\u683c"
                    color: "#FFFFFF"
                    font.pixelSize: 11
                    font.bold: true
                }
            }
        }

        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: headBg.bottom
            anchors.bottom: parent.bottom
            anchors.margins: 24
            anchors.topMargin: 18
            spacing: 12

            Text {
                text: isLoginMode ? "\u6b22\u8fce\u56de\u6765" : "\u521b\u5efa\u8d26\u53f7"
                color: textPrimary
                font.pixelSize: 24
                font.bold: true
            }

            Text {
                text: isLoginMode
                      ? "\u8f93\u5165\u8d26\u53f7\u5bc6\u7801\u540e\u5373\u53ef\u7ee7\u7eed"
                      : "\u586b\u5199\u8d26\u53f7\u4fe1\u606f\u5b8c\u6210\u6ce8\u518c"
                color: textSecondary
                font.pixelSize: 13
            }

            Column {
                width: parent.width
                spacing: 8

                Text {
                    text: "\u8d26\u53f7"
                    color: "#4F5663"
                    font.pixelSize: 13
                    font.bold: true
                }

                TextField {
                    id: accountInput
                    width: parent.width
                    height: 44
                    placeholderText: "\u8bf7\u8f93\u5165\u8d26\u53f7"
                    font.pixelSize: 14
                    selectByMouse: true
                    leftPadding: 14
                    rightPadding: 14
                    color: textPrimary

                    background: Rectangle {
                        radius: 12
                        color: panelBg
                        border.width: accountInput.activeFocus ? 2 : 1
                        border.color: accountInput.activeFocus ? brandRed : borderColor
                    }
                }
            }

            Column {
                width: parent.width
                spacing: 8

                Text {
                    text: "\u5bc6\u7801"
                    color: "#4F5663"
                    font.pixelSize: 13
                    font.bold: true
                }

                TextField {
                    id: passwordInput
                    width: parent.width
                    height: 44
                    placeholderText: "\u8bf7\u8f93\u5165\u5bc6\u7801"
                    echoMode: TextInput.Password
                    font.pixelSize: 14
                    selectByMouse: true
                    leftPadding: 14
                    rightPadding: 14
                    color: textPrimary

                    background: Rectangle {
                        radius: 12
                        color: panelBg
                        border.width: passwordInput.activeFocus ? 2 : 1
                        border.color: passwordInput.activeFocus ? brandRed : borderColor
                    }

                    Keys.onReturnPressed: {
                        if (isLoginMode) {
                            onLoginClicked()
                        }
                    }
                }
            }

            Item {
                id: loginModeArea
                width: parent.width
                clip: true
                property real factor: isLoginMode ? 1.0 : 0.0
                height: loginModeContent.implicitHeight * factor
                opacity: factor
                visible: height > 0.5

                Behavior on factor {
                    NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
                }
                Behavior on height {
                    NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
                }
                Behavior on opacity {
                    NumberAnimation { duration: 180 }
                }

                Column {
                    id: loginModeContent
                    width: parent.width
                    spacing: 10

                    Row {
                        width: parent.width

                        Item { width: parent.width - forgotPasswordText.implicitWidth; height: 1 }

                        Text {
                            id: forgotPasswordText
                            text: "\u5fd8\u8bb0\u5bc6\u7801\uff1f"
                            color: brandRed
                            font.pixelSize: 13
                            font.bold: true

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
                        height: 64
                        radius: 12
                        visible: savedAccount.length > 0
                        color: "#FFF4F4"
                        border.width: 1
                        border.color: "#F8CDCD"
                        opacity: savedAccount.length > 0 ? 1.0 : 0.0

                        Behavior on opacity {
                            NumberAnimation { duration: 150 }
                        }

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 12
                            spacing: 10

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 34
                                height: 34
                                radius: 17
                                color: brandRed

                                Text {
                                    anchors.centerIn: parent
                                    text: "\u266A"
                                    color: "#FFFFFF"
                                    font.pixelSize: 15
                                    font.bold: true
                                }
                            }

                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 2

                                Text {
                                    text: "\u6700\u8fd1\u4f7f\u7528"
                                    color: brandRed
                                    font.pixelSize: 11
                                    font.bold: true
                                }

                                Text {
                                    text: savedUsername.length > 0 ? (savedUsername + " (" + savedAccount + ")") : savedAccount
                                    color: textPrimary
                                    font.pixelSize: 13
                                    elide: Text.ElideRight
                                    width: 190
                                }
                            }

                            Item { width: 1; height: 1 }

                            Button {
                                id: quickLoginBtn
                                width: 94
                                height: 34
                                text: "\u4e00\u952e\u767b\u5f55"
                                anchors.verticalCenter: parent.verticalCenter
                                scale: quickLoginBtn.pressed ? 0.98 : (quickLoginBtn.hovered ? 1.02 : 1.0)

                                Behavior on scale {
                                    NumberAnimation { duration: 110; easing.type: Easing.OutCubic }
                                }

                                background: Rectangle {
                                    radius: 10
                                    color: quickLoginBtn.pressed ? brandRedDark : (quickLoginBtn.hovered ? "#F15A5A" : brandRed)
                                    Behavior on color {
                                        ColorAnimation { duration: 120 }
                                    }
                                }

                                contentItem: Text {
                                    text: quickLoginBtn.text
                                    color: "#ffffff"
                                    font.pixelSize: 13
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                onClicked: {
                                    if (savedAccount.length === 0 || savedPassword.length === 0) {
                                        statusSuccess = false
                                        statusMessage = "\u7f13\u5b58\u8d26\u53f7\u4fe1\u606f\u4e0d\u5b8c\u6574\uff0c\u8bf7\u624b\u52a8\u767b\u5f55"
                                        return
                                    }
                                    statusMessage = ""
                                    loginWindow.quickLoginRequested(savedAccount, savedPassword)
                                }
                            }
                        }
                    }
                }
            }

            Item {
                id: registerModeArea
                width: parent.width
                clip: true
                property real factor: isLoginMode ? 0.0 : 1.0
                height: registerModeContent.implicitHeight * factor
                opacity: factor
                visible: height > 0.5

                Behavior on factor {
                    NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
                }
                Behavior on height {
                    NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
                }
                Behavior on opacity {
                    NumberAnimation { duration: 180 }
                }

                Column {
                    id: registerModeContent
                    width: parent.width
                    spacing: 8

                    Text {
                        text: "\u7528\u6237\u540d"
                        color: "#4F5663"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    TextField {
                        id: usernameInput
                        width: parent.width
                        height: 44
                        placeholderText: "\u8bf7\u8f93\u5165\u7528\u6237\u540d"
                        font.pixelSize: 14
                        selectByMouse: true
                        leftPadding: 14
                        rightPadding: 14
                        color: textPrimary

                        background: Rectangle {
                            radius: 12
                            color: panelBg
                            border.width: usernameInput.activeFocus ? 2 : 1
                            border.color: usernameInput.activeFocus ? brandRed : borderColor
                        }

                        Keys.onReturnPressed: {
                            if (!isLoginMode) {
                                onLoginClicked()
                            }
                        }
                    }
                }
            }

            Button {
                id: loginButton
                width: parent.width
                height: 46
                text: isLoginMode ? "\u767b\u5f55" : "\u6ce8\u518c"
                scale: loginButton.pressed ? 0.985 : (loginButton.hovered ? 1.01 : 1.0)

                Behavior on scale {
                    NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
                }

                background: Rectangle {
                    radius: 12
                    color: loginButton.pressed ? brandRedDark : (loginButton.hovered ? "#F15A5A" : brandRed)
                    Behavior on color {
                        ColorAnimation { duration: 120 }
                    }
                }

                contentItem: Text {
                    text: loginButton.text
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: onLoginClicked()
            }

            Text {
                id: statusText
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: statusMessage
                color: statusSuccess ? "#1F8E49" : "#D64848"
                font.pixelSize: 12
                wrapMode: Text.Wrap
                height: statusMessage.length > 0 ? implicitHeight : 0
                opacity: statusMessage.length > 0 ? 1.0 : 0.0
                visible: height > 0

                Behavior on height {
                    NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
                }
                Behavior on opacity {
                    NumberAnimation { duration: 150 }
                }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 6

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: isLoginMode ? "\u8fd8\u6ca1\u6709\u8d26\u53f7\uff1f" : "\u5df2\u6709\u8d26\u53f7\uff1f"
                    color: textSecondary
                    font.pixelSize: 13
                }

                Text {
                    id: switchModeText
                    anchors.verticalCenter: parent.verticalCenter
                    text: isLoginMode ? "\u7acb\u5373\u6ce8\u518c" : "\u7acb\u5373\u767b\u5f55"
                    color: brandRed
                    font.pixelSize: 13
                    font.bold: true
                    scale: switchModeArea.pressed ? 0.96 : (switchModeArea.containsMouse ? 1.03 : 1.0)

                    Behavior on scale {
                        NumberAnimation { duration: 110; easing.type: Easing.OutCubic }
                    }

                    MouseArea {
                        id: switchModeArea
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

    Popup {
        id: resetPopup
        modal: true
        focus: true
        width: 340
        height: 258
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        opacity: 1.0

        enter: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 160 }
                NumberAnimation { property: "scale"; from: 0.95; to: 1.0; duration: 180; easing.type: Easing.OutCubic }
            }
        }
        exit: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 130 }
                NumberAnimation { property: "scale"; from: 1.0; to: 0.96; duration: 130; easing.type: Easing.InCubic }
            }
        }

        background: Rectangle {
            radius: 14
            color: "#FFFFFF"
            border.width: 1
            border.color: borderColor

            layer.enabled: true
            layer.effect: DropShadow {
                transparentBorder: true
                horizontalOffset: 0
                verticalOffset: 6
                radius: 14
                samples: 28
                color: Qt.rgba(0, 0, 0, 0.22)
            }
        }

        Column {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 10

            Text {
                text: "\u91cd\u7f6e\u5bc6\u7801"
                color: textPrimary
                font.pixelSize: 18
                font.bold: true
            }

            Text {
                text: "\u5f53\u524d\u7248\u672c\u76f4\u63a5\u91cd\u7f6e\uff0c\u4e0d\u505a\u90ae\u7bb1\u6821\u9a8c"
                color: textSecondary
                font.pixelSize: 12
            }

            TextField {
                id: resetAccountInput
                width: parent.width
                height: 40
                placeholderText: "\u8bf7\u8f93\u5165\u8d26\u53f7"
                leftPadding: 12
                rightPadding: 12
                color: textPrimary
                background: Rectangle {
                    color: panelBg
                    radius: 10
                    border.width: 1
                    border.color: borderColor
                }
            }

            TextField {
                id: resetPasswordInput
                width: parent.width
                height: 40
                placeholderText: "\u8bf7\u8f93\u5165\u65b0\u5bc6\u7801"
                echoMode: TextInput.Password
                leftPadding: 12
                rightPadding: 12
                color: textPrimary
                background: Rectangle {
                    color: panelBg
                    radius: 10
                    border.width: 1
                    border.color: borderColor
                }
            }

            Text {
                id: resetStatusText
                width: parent.width
                visible: text.length > 0
                color: "#D64848"
                font.pixelSize: 12
                wrapMode: Text.Wrap
            }

            Row {
                spacing: 10
                anchors.horizontalCenter: parent.horizontalCenter

                Button {
                    id: resetCancelBtn
                    text: "\u53d6\u6d88"
                    width: 130
                    height: 38
                    scale: resetCancelBtn.pressed ? 0.98 : (resetCancelBtn.hovered ? 1.01 : 1.0)

                    Behavior on scale {
                        NumberAnimation { duration: 110; easing.type: Easing.OutCubic }
                    }

                    background: Rectangle {
                        radius: 10
                        color: resetCancelBtn.pressed ? "#F3F3F3" : (resetCancelBtn.hovered ? "#F8F8F8" : "#FFFFFF")
                        border.width: 1
                        border.color: "#E1E1E1"
                        Behavior on color {
                            ColorAnimation { duration: 120 }
                        }
                    }

                    contentItem: Text {
                        text: resetCancelBtn.text
                        color: "#4A515C"
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: resetPopup.close()
                }

                Button {
                    id: resetConfirmBtn
                    text: "\u786e\u8ba4\u91cd\u7f6e"
                    width: 130
                    height: 38
                    scale: resetConfirmBtn.pressed ? 0.98 : (resetConfirmBtn.hovered ? 1.01 : 1.0)

                    Behavior on scale {
                        NumberAnimation { duration: 110; easing.type: Easing.OutCubic }
                    }

                    background: Rectangle {
                        radius: 10
                        color: resetConfirmBtn.pressed ? brandRedDark : (resetConfirmBtn.hovered ? "#F15A5A" : brandRed)
                        Behavior on color {
                            ColorAnimation { duration: 120 }
                        }
                    }

                    contentItem: Text {
                        text: resetConfirmBtn.text
                        color: "#FFFFFF"
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        var account = resetAccountInput.text.trim()
                        var newPwd = resetPasswordInput.text.trim()
                        if (account === "" || newPwd === "") {
                            resetStatusText.text = "\u8d26\u53f7\u548c\u65b0\u5bc6\u7801\u4e0d\u80fd\u4e3a\u7a7a"
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
            statusMessage = "\u8d26\u53f7\u6216\u5bc6\u7801\u4e0d\u80fd\u4e3a\u7a7a"
            return
        }

        if (isLoginMode) {
            statusMessage = ""
            loginWindow.loginRequested(account, password)
        } else {
            var username = usernameInput.text.trim()
            if (username === "") {
                statusSuccess = false
                statusMessage = "\u7528\u6237\u540d\u4e0d\u80fd\u4e3a\u7a7a"
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
        statusMessage = (message && message.length > 0) ? message : "\u767b\u5f55\u5931\u8d25"
    }

    function switchToLoginMode() {
        isLoginMode = true
        usernameInput.text = ""
    }

    function onResetPasswordResult(success, message) {
        if (success) {
            resetPopup.close()
            statusSuccess = true
            statusMessage = (message && message.length > 0) ? message : "\u5bc6\u7801\u91cd\u7f6e\u6210\u529f\uff0c\u8bf7\u767b\u5f55"
        } else {
            resetStatusText.text = (message && message.length > 0) ? message : "\u5bc6\u7801\u91cd\u7f6e\u5931\u8d25"
        }
    }

    function onRegisterResult(success, message) {
        statusSuccess = success
        if (success) {
            statusMessage = (message && message.length > 0) ? message : "\u6ce8\u518c\u6210\u529f\uff0c\u8bf7\u767b\u5f55"
        } else {
            statusMessage = (message && message.length > 0) ? message : "\u6ce8\u518c\u5931\u8d25"
        }
    }
}
