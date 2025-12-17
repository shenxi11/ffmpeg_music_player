import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.14

Item {
    id: root
    width: 820
    height: 780
    
    // 窗口拖动相关
    property point clickPos: Qt.point(0, 0)
    
    // 主容器 - 带圆角和阴影
    Rectangle {
        id: mainContainer
        anchors.fill: parent
        anchors.margins: 10
        radius: 15
        color: "transparent"
        
        // 阴影效果
        layer.enabled: true
        layer.effect: DropShadow {
            transparentBorder: true
            horizontalOffset: 0
            verticalOffset: 5
            radius: 15.0
            samples: 31
            color: "#40000000"
        }
        
        // 背景渐变
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#ffecd2" }
                GradientStop { position: 1.0; color: "#fcb69f" }
            }
            
            // 自定义标题栏
            Rectangle {
                id: titleBar
                width: parent.width
                height: 45
                radius: parent.radius
                color: "#ff6b6b"
                
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: parent.radius
                    color: parent.color
                }
                
                MouseArea {
                    anchors.fill: parent
                    
                    onPressed: {
                        windowHelper.startDrag()
                    }
                    onPositionChanged: {
                        if (pressed) {
                            windowHelper.drag()
                        }
                    }
                    onReleased: {
                        windowHelper.endDrag()
                    }
                }
                
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 10
                    spacing: 10
                    
                    Label {
                        text: "🎤"
                        font.pixelSize: 20
                        color: "white"
                    }
                    
                    Label {
                        text: "Whisper 语音转文字"
                        font.pixelSize: 16
                        font.bold: true
                        color: "white"
                        Layout.fillWidth: true
                    }
                    
                    // 最小化按钮
                    Button {
                        text: "−"
                        font.pixelSize: 18
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 30
                        onClicked: windowHelper.minimizeWindow()
                        
                        background: Rectangle {
                            radius: 5
                            color: parent.hovered ? "#ffffff30" : "transparent"
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    
                    // 关闭按钮
                    Button {
                        text: "×"
                        font.pixelSize: 20
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 30
                        onClicked: windowHelper.closeWindow()
                        
                        background: Rectangle {
                            radius: 5
                            color: parent.hovered ? "#cc0000" : "transparent"
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
            
            // 内容区域
            ScrollView {
                id: scrollView
                anchors.fill: parent
                anchors.topMargin: titleBar.height + 15
                anchors.leftMargin: 25
                anchors.rightMargin: 25
                anchors.bottomMargin: 25
                clip: true
        
        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 25
            
            // 音频文件卡片
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                radius: 12
                color: "white"
                
                layer.enabled: true
                layer.effect: DropShadow {
                    transparentBorder: true
                    horizontalOffset: 0
                    verticalOffset: 3
                    radius: 10.0
                    samples: 21
                    color: "#30000000"
                }
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 12
                    
                    Label {
                        text: "🎧 音频文件"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#2d3748"
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        
                        TextField {
                            id: audioFileField
                            text: backend.audioFile
                            placeholderText: "选择要转录的音频文件..."
                            readOnly: true
                            Layout.fillWidth: true
                            font.pixelSize: 14
                            
                            background: Rectangle {
                                radius: 8
                                color: "#fff5f5"
                                border.color: audioFileField.activeFocus ? "#ff6b6b" : "#fed7d7"
                                border.width: 2
                            }
                        }
                        
                        Button {
                            text: "选择"
                            font.pixelSize: 14
                            font.bold: true
                            onClicked: backend.selectAudioFile()
                            
                            background: Rectangle {
                                radius: 8
                                color: parent.hovered ? "#ee5a6f" : "#ff6b6b"
                                
                                Behavior on color {
                                    ColorAnimation { duration: 200 }
                                }
                            }
                            
                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: 40
                        }
                    }
                }
            }
            
            // 模型设置卡片
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 260
                radius: 12
                color: "white"
                
                layer.enabled: true
                layer.effect: DropShadow {
                    transparentBorder: true
                    horizontalOffset: 0
                    verticalOffset: 3
                    radius: 10.0
                    samples: 21
                    color: "#30000000"
                }
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 15
                    
                    Label {
                        text: "🔧 模型设置"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#2d3748"
                    }
                    
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        rowSpacing: 15
                        columnSpacing: 15
                        
                        Label {
                            text: "模型:"
                            font.pixelSize: 14
                            color: "#4a5568"
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            
                            ComboBox {
                                id: modelCombo
                                model: backend.availableModels.length > 0 ? backend.availableModels : ["请先扫描模型"]
                                currentIndex: backend.availableModels.indexOf(backend.modelPath)
                                Layout.fillWidth: true
                                font.pixelSize: 13
                                
                                delegate: ItemDelegate {
                                    width: modelCombo.width
                                    text: {
                                        if (modelData === "请先扫描模型") return modelData
                                        var path = modelData
                                        return path.substring(path.lastIndexOf('/') + 1)
                                    }
                                    font.pixelSize: 13
                                }
                                
                                displayText: {
                                    if (currentText === "请先扫描模型") return currentText
                                    if (currentText) {
                                        var path = currentText
                                        return path.substring(path.lastIndexOf('/') + 1)
                                    }
                                    return "选择模型..."
                                }
                                
                                onCurrentTextChanged: {
                                    if (currentText && currentText !== "请先扫描模型" && currentText !== backend.modelPath) {
                                        backend.modelPath = currentText
                                    }
                                }
                                
                                background: Rectangle {
                                    radius: 8
                                    color: modelCombo.pressed ? "#fff5f5" : "white"
                                    border.color: modelCombo.activeFocus ? "#ff6b6b" : "#fed7d7"
                                    border.width: 2
                                }
                            }
                            
                            Button {
                                text: "📁"
                                onClicked: backend.selectModelFile()
                                font.pixelSize: 16
                                Layout.preferredWidth: 45
                                Layout.preferredHeight: 40
                                
                                background: Rectangle {
                                    radius: 8
                                    color: parent.hovered ? "#fed7d7" : "#fff5f5"
                                    border.color: "#fecaca"
                                    border.width: 1
                                }
                            }
                            
                            Button {
                                text: "🔄"
                                onClicked: backend.scanForModels()
                                font.pixelSize: 16
                                Layout.preferredWidth: 45
                                Layout.preferredHeight: 40
                                
                                background: Rectangle {
                                    radius: 8
                                    color: parent.hovered ? "#fed7d7" : "#fff5f5"
                                    border.color: "#fecaca"
                                    border.width: 1
                                }
                            }
                        }
                        
                        Label {
                            text: "语言:"
                            font.pixelSize: 14
                            color: "#4a5568"
                        }
                        
                        ComboBox {
                            id: languageCombo
                            model: backend.supportedLanguages
                            currentIndex: 0
                            Layout.fillWidth: true
                            font.pixelSize: 14
                            
                            onCurrentTextChanged: {
                                backend.language = currentText
                            }
                            
                            background: Rectangle {
                                radius: 8
                                color: languageCombo.pressed ? "#fff5f5" : "white"
                                border.color: languageCombo.activeFocus ? "#ff6b6b" : "#fed7d7"
                                border.width: 2
                            }
                        }
                        
                        Label {
                            text: "输出格式:"
                            font.pixelSize: 14
                            color: "#4a5568"
                        }
                        
                        ComboBox {
                            id: formatCombo
                            model: ["lrc", "txt", "srt", "vtt", "json"]
                            currentIndex: 0
                            Layout.fillWidth: true
                            font.pixelSize: 14
                            
                            onCurrentTextChanged: {
                                backend.outputFormat = currentText
                            }
                            
                            background: Rectangle {
                                radius: 8
                                color: formatCombo.pressed ? "#fff5f5" : "white"
                                border.color: formatCombo.activeFocus ? "#ff6b6b" : "#fed7d7"
                                border.width: 2
                            }
                        }
                    }
                }
            }
            
            // 进度卡片
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                radius: 12
                color: "white"
                visible: backend.isProcessing
                
                layer.enabled: true
                layer.effect: DropShadow {
                    transparentBorder: true
                    horizontalOffset: 0
                    verticalOffset: 3
                    radius: 10.0
                    samples: 21
                    color: "#30000000"
                }
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 10
                    
                    Label {
                        text: "⏳ 识别进度"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#2d3748"
                    }
                    
                    ProgressBar {
                        id: progressBar
                        from: 0
                        to: 100
                        value: backend.progress
                        Layout.fillWidth: true
                        
                        background: Rectangle {
                            radius: 4
                            color: "#fed7d7"
                        }
                        
                        contentItem: Item {
                            Rectangle {
                                width: progressBar.visualPosition * parent.width
                                height: parent.height
                                radius: 4
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#ff6b6b" }
                                    GradientStop { position: 1.0; color: "#ee5a6f" }
                                }
                            }
                        }
                    }
                    
                    Label {
                        text: backend.progress + "%"
                        font.pixelSize: 13
                        color: "#718096"
                        Layout.alignment: Qt.AlignRight
                    }
                }
            }
            
            // 状态文本
            Label {
                text: backend.statusText
                font.pixelSize: 14
                font.bold: true
                color: backend.statusText.startsWith("错误") ? "#e53e3e" : 
                       backend.statusText.includes("完成") ? "#38a169" : "#ff6b6b"
                Layout.alignment: Qt.AlignHCenter
                
                Behavior on color {
                    ColorAnimation { duration: 300 }
                }
            }
            
            // 操作按钮
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 15
                
                Button {
                    id: startButton
                    text: backend.isProcessing ? "⏸️ 停止" : "▶️ 开始转录"
                    enabled: backend.audioFile !== "" && backend.modelPath !== ""
                    onClicked: {
                        if (backend.isProcessing) {
                            backend.stopTranscription()
                        } else {
                            backend.startTranscription()
                        }
                    }
                    font.pixelSize: 15
                    font.bold: true
                    
                    Layout.preferredWidth: 150
                    Layout.preferredHeight: 50
                    
                    background: Rectangle {
                        radius: 10
                        gradient: Gradient {
                            GradientStop { 
                                position: 0.0
                                color: startButton.enabled ? (startButton.hovered ? "#ee5a6f" : "#ff6b6b") : "#cbd5e0"
                            }
                            GradientStop { 
                                position: 1.0
                                color: startButton.enabled ? (startButton.hovered ? "#dc4a5c" : "#ee5a6f") : "#a0aec0"
                            }
                        }
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                
                Button {
                    text: "🗑️ 清空"
                    onClicked: backend.clearText()
                    enabled: !backend.isProcessing && backend.transcribedText !== ""
                    font.pixelSize: 15
                    
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 50
                    
                    background: Rectangle {
                        radius: 10
                        color: parent.enabled ? (parent.hovered ? "#fc8181" : "#f56565") : "#cbd5e0"
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                
                Button {
                    text: "💾 保存"
                    onClicked: backend.saveTranscription()
                    enabled: backend.transcribedText !== ""
                    font.pixelSize: 15
                    
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 50
                    
                    background: Rectangle {
                        radius: 10
                        color: parent.enabled ? (parent.hovered ? "#48bb78" : "#38a169") : "#cbd5e0"
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
            
            // 转录结果卡片
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 300
                Layout.minimumHeight: 200
                radius: 12
                color: "white"
                
                layer.enabled: true
                layer.effect: DropShadow {
                    transparentBorder: true
                    horizontalOffset: 0
                    verticalOffset: 3
                    radius: 10.0
                    samples: 21
                    color: "#30000000"
                }
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 12
                    
                    Label {
                        text: "📝 转录结果"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#2d3748"
                    }
                    
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        
                        TextArea {
                            id: resultText
                            text: backend.transcribedText
                            placeholderText: "转录结果将显示在这里...\n\n💡 提示: 请先选择音频文件和模型，然后点击'开始转录'"
                            wrapMode: TextArea.Wrap
                            selectByMouse: true
                            font.pixelSize: 14
                            
                            background: Rectangle {
                                radius: 8
                                color: "#fffaf0"
                                border.color: "#fed7d7"
                                border.width: 1
                            }
                        }
                    }
                }
            }
            
            Item {
                Layout.fillHeight: true
                Layout.minimumHeight: 20
            }
        }
            }
        }
    }
    
    Connections {
        target: backend
        function onTranscriptionFinished(success, message) {
            if (success) {
                console.log("✅ 转录成功:", message)
            } else {
                console.log("❌ 转录失败:", message)
            }
        }
    }
}
