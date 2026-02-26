import QtQuick 2.14
import QtQuick.Window 2.14
import QtQuick.Controls 2.14
import QtGraphicalEffects 1.14

// 寰幆妯″紡閫夋嫨寮圭獥
Window {
    id: root
    width: 180
    height: 200
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"
    
    property int currentMode: 2  // 榛樿鍒楄〃寰幆
    property bool ignoreNextFocusLoss: false  // 鏍囧織锛氬拷鐣ヤ笅涓€娆＄劍鐐逛涪澶?
    
    signal modeChanged(int mode)
    
    // 鐩戝惉搴旂敤绋嬪簭閫€鍑轰俊鍙?
    Connections {
        target: Qt.application
        function onAboutToQuit() {
            root.close()
        }
    }
    
    // 澶卞幓鐒︾偣鏃跺叧闂紙鐐瑰嚮绐楀彛澶栭儴锛?
    onActiveChanged: {
        if (!active && !ignoreNextFocusLoss) {
            // 寤惰繜妫€鏌ワ紝閬垮厤涓庢寜閽偣鍑诲啿绐?
            closeTimer.restart()
        }
        ignoreNextFocusLoss = false
    }
    
    // 寤惰繜鍏抽棴瀹氭椂鍣?
    Timer {
        id: closeTimer
        interval: 150
        onTriggered: {
            if (!root.active) {
                root.close()
            }
        }
    }
    
    // 鑳屾櫙瀹瑰櫒
    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"
        radius: 8
        border.color: "#E0E0E0"
        border.width: 1
        
        layer.enabled: true
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: 2
            radius: 8
            samples: 17
            color: "#40000000"
        }
    }
    
    Column {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 0
        
        // 椤哄簭鎾斁
        Rectangle {
            width: parent.width
            height: 45
            color: sequentialMouseArea.containsMouse ? "#F5F5F5" : "transparent"
            radius: 4
            
            Row {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 12
                
                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 20
                    height: 20
                    source: "qrc:/new/prefix1/icon/sequential.png"
                    fillMode: Image.PreserveAspectFit
                }
                
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "椤哄簭鎾斁"
                    font.pixelSize: 14
                    color: root.currentMode === 0 ? "#EC4141" : "#333333"
                }
                
                Item { width: parent.width - 32 - 12*2 - 80; height: 1 }
                
                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 16
                    height: 16
                    source: "qrc:/new/prefix1/icon/check.png"
                    visible: root.currentMode === 0
                    fillMode: Image.PreserveAspectFit
                }
            }
            
            MouseArea {
                id: sequentialMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.currentMode = 0
                    root.modeChanged(0)
                    root.close()
                }
            }
        }
        
        // 鍗曟洸寰幆
        Rectangle {
            width: parent.width
            height: 45
            color: repeatOneMouseArea.containsMouse ? "#F5F5F5" : "transparent"
            radius: 4
            
            Row {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 12
                
                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 20
                    height: 20
                    source: "qrc:/new/prefix1/icon/repeat_one.png"
                    fillMode: Image.PreserveAspectFit
                }
                
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "鍗曟洸寰幆"
                    font.pixelSize: 14
                    color: root.currentMode === 1 ? "#EC4141" : "#333333"
                }
                
                Item { width: 50; height: 1 }
                
                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 16
                    height: 16
                    source: "qrc:/new/prefix1/icon/check.png"
                    visible: root.currentMode === 1
                    fillMode: Image.PreserveAspectFit
                }
            }
            
            MouseArea {
                id: repeatOneMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.currentMode = 1
                    root.modeChanged(1)
                    root.close()
                }
            }
        }
        
        // 鍒楄〃寰幆
        Rectangle {
            width: parent.width
            height: 45
            color: repeatAllMouseArea.containsMouse ? "#F5F5F5" : "transparent"
            radius: 4
            
            Row {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 12
                
                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 20
                    height: 20
                    source: "qrc:/new/prefix1/icon/repeat_all.png"
                    fillMode: Image.PreserveAspectFit
                }
                
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "鍒楄〃寰幆"
                    font.pixelSize: 14
                    color: root.currentMode === 2 ? "#EC4141" : "#333333"
                }
                
                Item { width: 50; height: 1 }
                
                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 16
                    height: 16
                    source: "qrc:/new/prefix1/icon/check.png"
                    visible: root.currentMode === 2
                    fillMode: Image.PreserveAspectFit
                }
            }
            
            MouseArea {
                id: repeatAllMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.currentMode = 2
                    root.modeChanged(2)
                    root.close()
                }
            }
        }
        
        // 闅忔満鎾斁
        Rectangle {
            width: parent.width
            height: 45
            color: shuffleMouseArea.containsMouse ? "#F5F5F5" : "transparent"
            radius: 4
            
            Row {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 12
                
                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 20
                    height: 20
                    source: "qrc:/new/prefix1/icon/shuffle.png"
                    fillMode: Image.PreserveAspectFit
                }
                
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "闅忔満鎾斁"
                    font.pixelSize: 14
                    color: root.currentMode === 3 ? "#EC4141" : "#333333"
                }
                
                Item { width: 50; height: 1 }
                
                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 16
                    height: 16
                    source: "qrc:/new/prefix1/icon/check.png"
                    visible: root.currentMode === 3
                    fillMode: Image.PreserveAspectFit
                }
            }
            
            MouseArea {
                id: shuffleMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.currentMode = 3
                    root.modeChanged(3)
                    root.close()
                }
            }
        }
    }
    
    function setMode(mode) {
        root.currentMode = mode
    }
}

