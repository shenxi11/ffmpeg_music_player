import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtQuick.Dialogs 1.3

ApplicationWindow {
    id: settingsWindow
    title: "\u684c\u9762\u6b4c\u8bcd\u8bbe\u7f6e"
    width: 400
    height: 350
    minimumWidth: 350
    minimumHeight: 300
    modality: Qt.ApplicationModal
    flags: Qt.Dialog

    property color currentColor: "#ffffff"
    property int currentFontSize: 18
    property string currentFontFamily: "Microsoft YaHei"

    signal settingsChanged(color color, int fontSize, string fontFamily)
    signal dialogClosed()

    onClosing: dialogClosed()

    Rectangle {
        anchors.fill: parent
        color: "#f0f0f0"

        ScrollView {
            anchors.fill: parent
            anchors.margins: 20

            ColumnLayout {
                width: parent.width
                spacing: 20

                Text {
                    text: "\u684c\u9762\u6b4c\u8bcd\u8bbe\u7f6e"
                    font.pixelSize: 20
                    font.bold: true
                    color: "#333333"
                    Layout.alignment: Qt.AlignHCenter
                }

                GroupBox {
                    title: "\u5b57\u4f53\u8bbe\u7f6e"
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 15

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: "\u5b57\u4f53\uff1a"
                                font.pixelSize: 14
                                color: "#333333"
                                Layout.minimumWidth: 80
                            }

                            ComboBox {
                                id: fontFamilyCombo
                                Layout.fillWidth: true
                                model: ["Microsoft YaHei", "SimSun", "KaiTi", "SimHei", "Arial", "Times New Roman"]
                                currentIndex: {
                                    var index = model.indexOf(settingsWindow.currentFontFamily)
                                    return index >= 0 ? index : 0
                                }

                                onCurrentTextChanged: {
                                    settingsWindow.currentFontFamily = currentText
                                    updatePreview()
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: "\u5927\u5c0f\uff1a"
                                font.pixelSize: 14
                                color: "#333333"
                                Layout.minimumWidth: 80
                            }

                            SpinBox {
                                id: fontSizeSpinBox
                                from: 12
                                to: 48
                                value: settingsWindow.currentFontSize

                                onValueChanged: {
                                    settingsWindow.currentFontSize = value
                                    updatePreview()
                                }
                            }

                            Text {
                                text: "px"
                                font.pixelSize: 14
                                color: "#666666"
                            }
                        }
                    }
                }

                GroupBox {
                    title: "\u989c\u8272\u8bbe\u7f6e"
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 15

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: "\u989c\u8272\uff1a"
                                font.pixelSize: 14
                                color: "#333333"
                                Layout.minimumWidth: 80
                            }

                            Rectangle {
                                id: colorPreview
                                width: 40
                                height: 30
                                color: settingsWindow.currentColor
                                border.width: 1
                                border.color: "#cccccc"
                                radius: 4

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: colorDialog.open()
                                    cursorShape: Qt.PointingHandCursor
                                }
                            }

                            Button {
                                text: "\u9009\u62e9\u989c\u8272"
                                onClicked: colorDialog.open()
                            }
                        }

                        Text {
                            text: "\u9884\u8bbe\u989c\u8272\uff1a"
                            font.pixelSize: 14
                            color: "#333333"
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 8

                            Repeater {
                                model: [
                                    "#ffffff", "#000000", "#ff0000", "#00ff00",
                                    "#0000ff", "#ffff00", "#ff00ff", "#00ffff",
                                    "#ffa500", "#800080", "#ffc0cb", "#a0522d"
                                ]

                                Rectangle {
                                    width: 30
                                    height: 30
                                    color: modelData
                                    border.width: settingsWindow.currentColor === modelData ? 3 : 1
                                    border.color: settingsWindow.currentColor === modelData ? "#333333" : "#cccccc"
                                    radius: 4

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            settingsWindow.currentColor = modelData
                                            updatePreview()
                                        }
                                        cursorShape: Qt.PointingHandCursor
                                    }
                                }
                            }
                        }
                    }
                }

                GroupBox {
                    title: "\u9884\u89c8"
                    Layout.fillWidth: true

                    Rectangle {
                        anchors.fill: parent
                        color: "#2a2a2a"
                        radius: 8
                        implicitHeight: 80

                        Text {
                            id: previewText
                            anchors.centerIn: parent
                            text: "\u8fd9\u662f\u684c\u9762\u6b4c\u8bcd\u9884\u89c8\u6548\u679c"
                            color: settingsWindow.currentColor
                            font.family: settingsWindow.currentFontFamily
                            font.pixelSize: settingsWindow.currentFontSize
                            font.bold: true
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 10

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "\u91cd\u7f6e"
                        onClicked: {
                            settingsWindow.currentColor = "#ffffff"
                            settingsWindow.currentFontSize = 18
                            settingsWindow.currentFontFamily = "Microsoft YaHei"
                            fontFamilyCombo.currentIndex = 0
                            fontSizeSpinBox.value = 18
                            updatePreview()
                        }
                    }

                    Button {
                        text: "\u53d6\u6d88"
                        onClicked: settingsWindow.close()
                    }

                    Button {
                        text: "\u786e\u5b9a"
                        highlighted: true
                        onClicked: {
                            settingsWindow.settingsChanged(
                                settingsWindow.currentColor,
                                settingsWindow.currentFontSize,
                                settingsWindow.currentFontFamily
                            )
                            settingsWindow.close()
                        }
                    }
                }
            }
        }
    }

    ColorDialog {
        id: colorDialog
        title: "\u9009\u62e9\u5b57\u4f53\u989c\u8272"
        color: settingsWindow.currentColor

        onAccepted: {
            settingsWindow.currentColor = color
            updatePreview()
        }
    }

    function updatePreview() {
        // 属性绑定会自动更新预览，这里保留接口供外部调用。
    }

    function setCurrentSettings(color, fontSize, fontFamily) {
        settingsWindow.currentColor = color
        settingsWindow.currentFontSize = fontSize
        settingsWindow.currentFontFamily = fontFamily

        var fontIndex = fontFamilyCombo.model.indexOf(fontFamily)
        if (fontIndex >= 0) {
            fontFamilyCombo.currentIndex = fontIndex
        }
        fontSizeSpinBox.value = fontSize

        updatePreview()
    }
}
