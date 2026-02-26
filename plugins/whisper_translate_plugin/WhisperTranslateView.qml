import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.14

Item {
    id: root
    width: 820
    height: 780
    
    // 绐楀彛鎷栧姩鐩稿叧
    property point clickPos: Qt.point(0, 0)
    
    // 涓诲鍣?- 甯﹀渾瑙掑拰闃村奖
    Rectangle {
        id: mainContainer
        anchors.fill: parent
        anchors.margins: 10
        radius: 15
        color: "transparent"
        
        // 闃村奖鏁堟灉
        layer.enabled: true
        layer.effect: DropShadow {
            transparentBorder: true
            horizontalOffset: 0
            verticalOffset: 5
            radius: 15.0
            samples: 31
            color: "#40000000"
        }
        
        // 鑳屾櫙娓愬彉
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#ffecd2" }
                GradientStop { position: 1.0; color: "#fcb69f" }
            }
            
            // 鑷畾涔夋爣棰樻爮
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
                        text: "馃帳"
                        font.pixelSize: 20
                        color: "white"
                    }
                    
                    Label {
                        text: "Whisper 璇煶杞枃瀛?
                        font.pixelSize: 16
                        font.bold: true
                        color: "white"
                        Layout.fillWidth: true
                    }
                    
                    // 鏈€灏忓寲鎸夐挳
                    Button {
                        text: "鈭?
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
                    
                    // 鍏抽棴鎸夐挳
                    Button {
                        text: "脳"
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
            
            // 鍐呭鍖哄煙
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
            
            // 闊抽鏂囦欢鍗＄墖
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
                        text: "馃帶 闊抽鏂囦欢"
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
                            placeholderText: "閫夋嫨瑕佽浆褰曠殑闊抽鏂囦欢..."
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
                            text: "閫夋嫨"
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
            
            // 妯″瀷璁剧疆鍗＄墖
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
                        text: "馃敡 妯″瀷璁剧疆"
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
                            text: "妯″瀷:"
                            font.pixelSize: 14
                            color: "#4a5568"
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            
                            ComboBox {
                                id: modelCombo
                                model: backend.availableModels.length > 0 ? backend.availableModels : ["璇峰厛鎵弿妯″瀷"]
                                currentIndex: backend.availableModels.indexOf(backend.modelPath)
                                Layout.fillWidth: true
                                font.pixelSize: 13
                                
                                delegate: ItemDelegate {
                                    width: modelCombo.width
                                    text: {
                                        if (modelData === "璇峰厛鎵弿妯″瀷") return modelData
                                        var path = modelData
                                        return path.substring(path.lastIndexOf('/') + 1)
                                    }
                                    font.pixelSize: 13
                                }
                                
                                displayText: {
                                    if (currentText === "璇峰厛鎵弿妯″瀷") return currentText
                                    if (currentText) {
                                        var path = currentText
                                        return path.substring(path.lastIndexOf('/') + 1)
                                    }
                                    return "閫夋嫨妯″瀷..."
                                }
                                
                                onCurrentTextChanged: {
                                    if (currentText && currentText !== "璇峰厛鎵弿妯″瀷" && currentText !== backend.modelPath) {
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
                                text: "馃搧"
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
                                text: "馃攧"
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
                            text: "璇█:"
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
                            text: "杈撳嚭鏍煎紡:"
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
            
            // 杩涘害鍗＄墖
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
                        text: "鈴?璇嗗埆杩涘害"
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
            
            // 鐘舵€佹枃鏈?
            Label {
                text: backend.statusText
                font.pixelSize: 14
                font.bold: true
                color: backend.statusText.startsWith("閿欒") ? "#e53e3e" : 
                       backend.statusText.includes("瀹屾垚") ? "#EC4141" : "#ff6b6b"
                Layout.alignment: Qt.AlignHCenter
                
                Behavior on color {
                    ColorAnimation { duration: 300 }
                }
            }
            
            // 鎿嶄綔鎸夐挳
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 15
                
                Button {
                    id: startButton
                    text: backend.isProcessing ? "鈴革笍 鍋滄" : "鈻讹笍 寮€濮嬭浆褰?
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
                    text: "馃棏锔?娓呯┖"
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
                    text: "馃捑 淇濆瓨"
                    onClicked: backend.saveTranscription()
                    enabled: backend.transcribedText !== ""
                    font.pixelSize: 15
                    
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 50
                    
                    background: Rectangle {
                        radius: 10
                        color: parent.enabled ? (parent.hovered ? "#FF5757" : "#EC4141") : "#cbd5e0"
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
            
            // 杞綍缁撴灉鍗＄墖
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
                        text: "馃摑 杞綍缁撴灉"
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
                            placeholderText: "杞綍缁撴灉灏嗘樉绀哄湪杩欓噷...\n\n馃挕 鎻愮ず: 璇峰厛閫夋嫨闊抽鏂囦欢鍜屾ā鍨嬶紝鐒跺悗鐐瑰嚮'寮€濮嬭浆褰?"
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
                console.log("鉁?杞綍鎴愬姛:", message)
            } else {
                console.log("鉂?杞綍澶辫触:", message)
            }
        }
    }
}

