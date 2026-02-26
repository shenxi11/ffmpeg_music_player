import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtQuick.Dialogs 1.3

ApplicationWindow {
    id: settingsWindow
    title: "妗岄潰姝岃瘝璁剧疆"
    width: 400
    height: 350
    minimumWidth: 350
    minimumHeight: 300
    
    modality: Qt.ApplicationModal
    flags: Qt.Dialog
    
    // 灞炴€?    property color currentColor: "#ffffff"
    property int currentFontSize: 18
    property string currentFontFamily: "Microsoft YaHei"
    
    // 淇″彿
    signal settingsChanged(color color, int fontSize, string fontFamily)
    signal dialogClosed()
    
    // 绐楀彛鍏抽棴鏃跺彂鍑轰俊鍙?    onClosing: {
        dialogClosed()
    }
    
    // 鑳屾櫙
    Rectangle {
        anchors.fill: parent
        color: "#f0f0f0"
        
        ScrollView {
            anchors.fill: parent
            anchors.margins: 20
            
            ColumnLayout {
                width: parent.width
                spacing: 20
                
                // 鏍囬
                Text {
                    text: "妗岄潰姝岃瘝璁剧疆"
                    font.pixelSize: 20
                    font.bold: true
                    color: "#333333"
                    Layout.alignment: Qt.AlignHCenter
                }
                
                // 瀛椾綋璁剧疆缁?                GroupBox {
                    title: "瀛椾綋璁剧疆"
                    Layout.fillWidth: true
                    
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 15
                        
                        // 瀛椾綋鏃忚缃?                        RowLayout {
                            Layout.fillWidth: true
                            
                            Text {
                                text: "瀛椾綋锛?
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
                        
                        // 瀛椾綋澶у皬璁剧疆
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Text {
                                text: "澶у皬锛?
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
                
                // 棰滆壊璁剧疆缁?                GroupBox {
                    title: "棰滆壊璁剧疆"
                    Layout.fillWidth: true
                    
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 15
                        
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Text {
                                text: "棰滆壊锛?
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
                                text: "閫夋嫨棰滆壊"
                                onClicked: colorDialog.open()
                            }
                        }
                        
                        // 棰勮棰滆壊
                        Text {
                            text: "棰勮棰滆壊锛?
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
                
                // 棰勮缁?                GroupBox {
                    title: "棰勮"
                    Layout.fillWidth: true
                    
                    Rectangle {
                        anchors.fill: parent
                        color: "#2a2a2a"
                        radius: 8
                        implicitHeight: 80
                        
                        Text {
                            id: previewText
                            anchors.centerIn: parent
                            text: "杩欐槸妗岄潰姝岃瘝棰勮鏁堟灉"
                            color: settingsWindow.currentColor
                            font.family: settingsWindow.currentFontFamily
                            font.pixelSize: settingsWindow.currentFontSize
                            font.bold: true
                        }
                    }
                }
                
                // 鎸夐挳缁?                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 10
                    
                    Item {
                        Layout.fillWidth: true
                    }
                    
                    Button {
                        text: "閲嶇疆"
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
                        text: "鍙栨秷"
                        onClicked: settingsWindow.close()
                    }
                    
                    Button {
                        text: "纭畾"
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
    
    // 棰滆壊閫夋嫨瀵硅瘽妗?    ColorDialog {
        id: colorDialog
        title: "閫夋嫨瀛椾綋棰滆壊"
        color: settingsWindow.currentColor
        
        onAccepted: {
            settingsWindow.currentColor = color
            updatePreview()
        }
    }
    
    // 鍑芥暟
    function updatePreview() {
        // 鏇存柊棰勮鏂囨湰锛圦ML 浼氳嚜鍔ㄦ洿鏂扮粦瀹氱殑灞炴€э級
    }
    
    function setCurrentSettings(color, fontSize, fontFamily) {
        settingsWindow.currentColor = color
        settingsWindow.currentFontSize = fontSize
        settingsWindow.currentFontFamily = fontFamily
        
        // 鏇存柊鎺т欢鐘舵€?        var fontIndex = fontFamilyCombo.model.indexOf(fontFamily)
        if (fontIndex >= 0) {
            fontFamilyCombo.currentIndex = fontIndex
        }
        fontSizeSpinBox.value = fontSize
        
        updatePreview()
    }
}

