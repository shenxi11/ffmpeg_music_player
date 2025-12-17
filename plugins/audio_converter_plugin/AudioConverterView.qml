import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.14

Item {
    id: root
    width: 820
    height: 680
    
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
                GradientStop { position: 0.0; color: "#f5f7fa" }
                GradientStop { position: 1.0; color: "#e8eef5" }
            }
            
            // 自定义标题栏
            Rectangle {
                id: titleBar
                width: parent.width
                height: 45
                radius: parent.radius
                color: "#667eea"
                
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
                        text: "🎵"
                        font.pixelSize: 20
                        color: "white"
                    }
                    
                    Label {
                        text: "音频格式转换器"
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
                            color: parent.hovered ? "#ff4444" : "transparent"
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
            
            // 输入文件卡片
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                radius: 12
                color: "white"
                
                layer.enabled: true
                layer.effect: DropShadow {
                    transparentBorder: true
                    horizontalOffset: 0
                    verticalOffset: 2
                    radius: 8.0
                    samples: 17
                    color: "#20000000"
                }
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 12
                    
                    Label {
                        text: "📂 输入文件"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#2d3748"
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        
                        TextField {
                            id: inputFileField
                            text: backend.inputFile
                            placeholderText: "点击右侧按钮选择音频文件..."
                            readOnly: true
                            Layout.fillWidth: true
                            font.pixelSize: 14
                            
                            background: Rectangle {
                                radius: 8
                                color: "#f7fafc"
                                border.color: inputFileField.activeFocus ? "#667eea" : "#e2e8f0"
                                border.width: 2
                            }
                        }
                        
                        Button {
                            text: "浏览"
                            font.pixelSize: 14
                            font.bold: true
                            onClicked: backend.selectInputFile()
                            
                            background: Rectangle {
                                radius: 8
                                color: parent.hovered ? "#5568d3" : "#667eea"
                                
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
            
            // 输出设置卡片
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 280
                radius: 12
                color: "white"
                
                layer.enabled: true
                layer.effect: DropShadow {
                    transparentBorder: true
                    horizontalOffset: 0
                    verticalOffset: 2
                    radius: 8.0
                    samples: 17
                    color: "#20000000"
                }
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 15
                    
                    Label {
                        text: "⚙️ 输出设置"
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
                            text: "格式:"
                            font.pixelSize: 14
                            color: "#4a5568"
                        }
                        
                        ComboBox {
                            id: formatCombo
                            model: backend.supportedFormats
                            currentIndex: backend.supportedFormats.indexOf(backend.outputFormat)
                            onCurrentTextChanged: {
                                if (currentText !== backend.outputFormat) {
                                    backend.outputFormat = currentText
                                }
                            }
                            Layout.fillWidth: true
                            font.pixelSize: 14
                            
                            background: Rectangle {
                                radius: 8
                                color: formatCombo.pressed ? "#f7fafc" : "white"
                                border.color: formatCombo.activeFocus ? "#667eea" : "#e2e8f0"
                                border.width: 2
                            }
                        }
                        
                        Label {
                            text: "比特率:"
                            font.pixelSize: 14
                            color: "#4a5568"
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            
                            SpinBox {
                                id: bitrateSpinBox
                                from: 64
                                to: 320
                                stepSize: 32
                                value: backend.bitrate
                                onValueChanged: backend.bitrate = value
                                Layout.fillWidth: true
                                font.pixelSize: 14
                                
                                background: Rectangle {
                                    radius: 8
                                    color: "#f7fafc"
                                    border.color: bitrateSpinBox.activeFocus ? "#667eea" : "#e2e8f0"
                                    border.width: 2
                                }
                            }
                            
                            Label {
                                text: "kbps"
                                font.pixelSize: 13
                                color: "#718096"
                            }
                        }
                        
                        Label {
                            text: "采样率:"
                            font.pixelSize: 14
                            color: "#4a5568"
                        }
                        
                        ComboBox {
                            id: sampleRateCombo
                            model: ["22050 Hz", "44100 Hz", "48000 Hz", "96000 Hz"]
                            currentIndex: 1
                            onCurrentIndexChanged: {
                                var rates = [22050, 44100, 48000, 96000]
                                backend.sampleRate = rates[currentIndex]
                            }
                            Layout.fillWidth: true
                            font.pixelSize: 14
                            
                            background: Rectangle {
                                radius: 8
                                color: sampleRateCombo.pressed ? "#f7fafc" : "white"
                                border.color: sampleRateCombo.activeFocus ? "#667eea" : "#e2e8f0"
                                border.width: 2
                            }
                        }
                        
                        Label {
                            text: "输出文件:"
                            font.pixelSize: 14
                            color: "#4a5568"
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            
                            TextField {
                                id: outputFileField
                                text: backend.outputFile
                                placeholderText: "输出文件路径..."
                                readOnly: true
                                Layout.fillWidth: true
                                font.pixelSize: 14
                                
                                background: Rectangle {
                                    radius: 8
                                    color: "#f7fafc"
                                    border.color: outputFileField.activeFocus ? "#667eea" : "#e2e8f0"
                                    border.width: 2
                                }
                            }
                            
                            Button {
                                text: "另存为"
                                font.pixelSize: 13
                                onClicked: backend.selectOutputFile()
                                
                                background: Rectangle {
                                    radius: 8
                                    color: parent.hovered ? "#e2e8f0" : "#edf2f7"
                                    border.color: "#cbd5e0"
                                    border.width: 1
                                }
                                
                                contentItem: Text {
                                    text: parent.text
                                    font: parent.font
                                    color: "#4a5568"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                    }
                }
            }
            
            // 进度条
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                radius: 12
                color: "white"
                visible: backend.isConverting
                
                layer.enabled: true
                layer.effect: DropShadow {
                    transparentBorder: true
                    horizontalOffset: 0
                    verticalOffset: 2
                    radius: 8.0
                    samples: 17
                    color: "#20000000"
                }
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 10
                    
                    Label {
                        text: "⏳ 转换进度"
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
                            color: "#e2e8f0"
                        }
                        
                        contentItem: Item {
                            Rectangle {
                                width: progressBar.visualPosition * parent.width
                                height: parent.height
                                radius: 4
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#667eea" }
                                    GradientStop { position: 1.0; color: "#764ba2" }
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
                       backend.statusText.includes("完成") ? "#38a169" : "#667eea"
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
                    text: backend.isConverting ? "⏸️ 停止转换" : "▶️ 开始转换"
                    enabled: backend.inputFile !== "" && backend.outputFile !== ""
                    onClicked: {
                        if (backend.isConverting) {
                            backend.stopConversion()
                        } else {
                            backend.startConversion()
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
                                color: startButton.enabled ? (startButton.hovered ? "#5568d3" : "#667eea") : "#cbd5e0"
                            }
                            GradientStop { 
                                position: 1.0
                                color: startButton.enabled ? (startButton.hovered ? "#6553b5" : "#764ba2") : "#a0aec0"
                            }
                        }
                        
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
                }
                
                Button {
                    text: "🗑️ 清空"
                    onClicked: backend.clearAll()
                    enabled: !backend.isConverting
                    font.pixelSize: 15
                    
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 50
                    
                    background: Rectangle {
                        radius: 10
                        color: parent.enabled ? (parent.hovered ? "#fc8181" : "#f56565") : "#cbd5e0"
                        
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
        function onConversionFinished(success, message) {
            if (success) {
                console.log("✅ 转换成功:", message)
            } else {
                console.log("❌ 转换失败:", message)
            }
        }
    }
}
