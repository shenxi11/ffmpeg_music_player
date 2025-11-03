import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtQuick.Dialogs 1.3

ApplicationWindow {
    id: settingsWindow
    title: "桌面歌词设置"
    width: 400
    height: 350
    minimumWidth: 350
    minimumHeight: 300
    
    modality: Qt.ApplicationModal
    flags: Qt.Dialog
    
    // 属性
    property color currentColor: "#ffffff"
    property int currentFontSize: 18
    property string currentFontFamily: "Microsoft YaHei"
    
    // 信号
    signal settingsChanged(color color, int fontSize, string fontFamily)
    signal dialogClosed()
    
    // 窗口关闭时发出信号
    onClosing: {
        dialogClosed()
    }
    
    // 背景
    Rectangle {
        anchors.fill: parent
        color: "#f0f0f0"
        
        ScrollView {
            anchors.fill: parent
            anchors.margins: 20
            
            ColumnLayout {
                width: parent.width
                spacing: 20
                
                // 标题
                Text {
                    text: "桌面歌词设置"
                    font.pixelSize: 20
                    font.bold: true
                    color: "#333333"
                    Layout.alignment: Qt.AlignHCenter
                }
                
                // 字体设置组
                GroupBox {
                    title: "字体设置"
                    Layout.fillWidth: true
                    
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 15
                        
                        // 字体族设置
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Text {
                                text: "字体："
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
                        
                        // 字体大小设置
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Text {
                                text: "大小："
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
                
                // 颜色设置组
                GroupBox {
                    title: "颜色设置"
                    Layout.fillWidth: true
                    
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 15
                        
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Text {
                                text: "颜色："
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
                                text: "选择颜色"
                                onClicked: colorDialog.open()
                            }
                        }
                        
                        // 预设颜色
                        Text {
                            text: "预设颜色："
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
                
                // 预览组
                GroupBox {
                    title: "预览"
                    Layout.fillWidth: true
                    
                    Rectangle {
                        anchors.fill: parent
                        color: "#2a2a2a"
                        radius: 8
                        implicitHeight: 80
                        
                        Text {
                            id: previewText
                            anchors.centerIn: parent
                            text: "这是桌面歌词预览效果"
                            color: settingsWindow.currentColor
                            font.family: settingsWindow.currentFontFamily
                            font.pixelSize: settingsWindow.currentFontSize
                            font.bold: true
                        }
                    }
                }
                
                // 按钮组
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 10
                    
                    Item {
                        Layout.fillWidth: true
                    }
                    
                    Button {
                        text: "重置"
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
                        text: "取消"
                        onClicked: settingsWindow.close()
                    }
                    
                    Button {
                        text: "确定"
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
    
    // 颜色选择对话框
    ColorDialog {
        id: colorDialog
        title: "选择字体颜色"
        color: settingsWindow.currentColor
        
        onAccepted: {
            settingsWindow.currentColor = color
            updatePreview()
        }
    }
    
    // 函数
    function updatePreview() {
        // 更新预览文本（QML 会自动更新绑定的属性）
    }
    
    function setCurrentSettings(color, fontSize, fontFamily) {
        settingsWindow.currentColor = color
        settingsWindow.currentFontSize = fontSize
        settingsWindow.currentFontFamily = fontFamily
        
        // 更新控件状态
        var fontIndex = fontFamilyCombo.model.indexOf(fontFamily)
        if (fontIndex >= 0) {
            fontFamilyCombo.currentIndex = fontIndex
        }
        fontSizeSpinBox.value = fontSize
        
        updatePreview()
    }
}