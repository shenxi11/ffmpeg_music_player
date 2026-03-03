import QtQuick 2.14
import QtQuick.Window 2.14
import QtQuick.Controls 2.14
import QtGraphicalEffects 1.14

Window {
    id: volumeWindow
    width: 60
    height: 200
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"
    
    property int volumeValue: 50
    property bool suppressInitialSignal: true
    property bool ignoreNextFocusLoss: false  // 鏍囧織锛氬拷鐣ヤ笅涓€娆＄劍鐐逛涪澶?
    
    signal volumeChanged(int value)

    Component.onCompleted: {
        Qt.callLater(function() {
            volumeWindow.suppressInitialSignal = false
        })
    }
    
    // 鐩戝惉搴旂敤绋嬪簭閫€鍑轰俊鍙?
    Connections {
        target: Qt.application
        function onAboutToQuit() {
            volumeWindow.close()
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
            if (!volumeWindow.active) {
                volumeWindow.close()
            }
        }
    }
    
    // 鑳屾櫙瀹瑰櫒
    Rectangle {
        anchors.fill: parent
        radius: 30
        color: "#CC2C2C2C"  // 鍗婇€忔槑娣辫壊鑳屾櫙
        
        // 闃村奖鏁堟灉
        layer.enabled: true
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: 4
            radius: 12
            samples: 25
            color: "#80000000"
        }
        
        Column {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 8
            
            // 闊抽噺鐧惧垎姣旀樉绀?
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: Math.round(volumeSlider.value) + "%"
                font.pixelSize: 14
                font.bold: true
                color: "#FFFFFF"
            }
            
            // 鍨傜洿闊抽噺婊戝潡
            Slider {
                id: volumeSlider
                width: parent.width
                height: parent.height - 30
                from: 0
                to: 100
                value: volumeWindow.volumeValue
                orientation: Qt.Vertical
                
                onValueChanged: {
                    volumeWindow.volumeValue = Math.round(value)
                    if (!volumeWindow.suppressInitialSignal) {
                        volumeWindow.volumeChanged(Math.round(value))
                    }
                }
                
                background: Rectangle {
                    x: volumeSlider.leftPadding + volumeSlider.availableWidth / 2 - width / 2
                    y: volumeSlider.topPadding
                    width: 6
                    height: volumeSlider.availableHeight
                    radius: 3
                    color: "#40FFFFFF"  // 鍗婇€忔槑鐧借壊
                    
                    Rectangle {
                        y: volumeSlider.visualPosition * parent.height
                        width: parent.width
                        height: parent.height - y
                        color: "#EC4141"  // 缁胯壊濉厖
                        radius: 3
                    }
                }
                
                handle: Rectangle {
                    x: volumeSlider.leftPadding + volumeSlider.availableWidth / 2 - width / 2
                    y: volumeSlider.topPadding + volumeSlider.visualPosition * (volumeSlider.availableHeight - height)
                    width: 18
                    height: 18
                    radius: 9
                    color: volumeSlider.pressed ? "#EC4141" : "#FFFFFF"
                    border.color: "#EC4141"
                    border.width: 2
                    
                    // 鎮仠鍜屾寜涓嬫晥鏋?
                    scale: volumeSlider.pressed ? 1.3 : (volumeSlider.hovered ? 1.2 : 1.0)
                    
                    Behavior on scale {
                        NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
                    }
                    
                    // 闃村奖
                    layer.enabled: true
                    layer.effect: DropShadow {
                        horizontalOffset: 0
                        verticalOffset: 2
                        radius: 4
                        samples: 9
                        color: "#60000000"
                    }
                }
            }
        }
    }
}

