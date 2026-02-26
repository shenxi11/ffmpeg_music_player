import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.14

Item {
    id: root
    width: 820
    height: 680
    
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
                GradientStop { position: 0.0; color: "#f5f7fa" }
                GradientStop { position: 1.0; color: "#e8eef5" }
            }
            
            // 鑷畾涔夋爣棰樻爮
            Rectangle {
                id: titleBar
                width: parent.width
                height: 45
                radius: parent.radius
                color: "#EC4141"
                
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
                        text: "馃幍"
                        font.pixelSize: 20
                        color: "white"
                    }
                    
                    Label {
                        text: "闊抽鏍煎紡杞崲鍣?
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
            
            // 杈撳叆鏂囦欢鍗＄墖
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
                        text: "馃搨 杈撳叆鏂囦欢"
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
                            placeholderText: "鐐瑰嚮鍙充晶鎸夐挳閫夋嫨闊抽鏂囦欢..."
                            readOnly: true
                            Layout.fillWidth: true
                            font.pixelSize: 14
                            
                            background: Rectangle {
                                radius: 8
                                color: "#f7fafc"
                                border.color: inputFileField.activeFocus ? "#EC4141" : "#e2e8f0"
                                border.width: 2
                            }
                        }
                        
                        Button {
                            text: "娴忚"
                            font.pixelSize: 14
                            font.bold: true
                            onClicked: backend.selectInputFile()
                            
                            background: Rectangle {
                                radius: 8
                                color: parent.hovered ? "#5568d3" : "#EC4141"
                                
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
            
            // 杈撳嚭璁剧疆鍗＄墖
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
                        text: "鈿欙笍 杈撳嚭璁剧疆"
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
                            text: "鏍煎紡:"
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
                                border.color: formatCombo.activeFocus ? "#EC4141" : "#e2e8f0"
                                border.width: 2
                            }
                        }
                        
                        Label {
                            text: "姣旂壒鐜?"
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
                                    border.color: bitrateSpinBox.activeFocus ? "#EC4141" : "#e2e8f0"
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
                            text: "閲囨牱鐜?"
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
                                border.color: sampleRateCombo.activeFocus ? "#EC4141" : "#e2e8f0"
                                border.width: 2
                            }
                        }
                        
                        Label {
                            text: "杈撳嚭鏂囦欢:"
                            font.pixelSize: 14
                            color: "#4a5568"
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            
                            TextField {
                                id: outputFileField
                                text: backend.outputFile
                                placeholderText: "杈撳嚭鏂囦欢璺緞..."
                                readOnly: true
                                Layout.fillWidth: true
                                font.pixelSize: 14
                                
                                background: Rectangle {
                                    radius: 8
                                    color: "#f7fafc"
                                    border.color: outputFileField.activeFocus ? "#EC4141" : "#e2e8f0"
                                    border.width: 2
                                }
                            }
                            
                            Button {
                                text: "鍙﹀瓨涓?
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
            
            // 杩涘害鏉?
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
                        text: "鈴?杞崲杩涘害"
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
                                    GradientStop { position: 0.0; color: "#EC4141" }
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
            
            // 鐘舵€佹枃鏈?
            Label {
                text: backend.statusText
                font.pixelSize: 14
                font.bold: true
                color: backend.statusText.startsWith("閿欒") ? "#e53e3e" : 
                       backend.statusText.includes("瀹屾垚") ? "#EC4141" : "#EC4141"
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
                    text: backend.isConverting ? "鈴革笍 鍋滄杞崲" : "鈻讹笍 寮€濮嬭浆鎹?
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
                                color: startButton.enabled ? (startButton.hovered ? "#5568d3" : "#EC4141") : "#cbd5e0"
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
                    text: "馃棏锔?娓呯┖"
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
                console.log("鉁?杞崲鎴愬姛:", message)
            } else {
                console.log("鉂?杞崲澶辫触:", message)
            }
        }
    }
}

