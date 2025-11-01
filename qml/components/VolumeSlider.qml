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
    
    property int volumeValue: 60
    property bool ignoreNextFocusLoss: false  // 标志：忽略下一次焦点丢失
    
    signal volumeChanged(int value)
    
    // 监听应用程序退出信号
    Connections {
        target: Qt.application
        function onAboutToQuit() {
            volumeWindow.close()
        }
    }
    
    // 失去焦点时关闭（点击窗口外部）
    onActiveChanged: {
        if (!active && !ignoreNextFocusLoss) {
            // 延迟检查，避免与按钮点击冲突
            closeTimer.restart()
        }
        ignoreNextFocusLoss = false
    }
    
    // 延迟关闭定时器
    Timer {
        id: closeTimer
        interval: 150
        onTriggered: {
            if (!volumeWindow.active) {
                volumeWindow.close()
            }
        }
    }
    
    // 背景容器
    Rectangle {
        anchors.fill: parent
        radius: 30
        color: "#CC2C2C2C"  // 半透明深色背景
        
        // 阴影效果
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
            
            // 音量百分比显示
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: Math.round(volumeSlider.value) + "%"
                font.pixelSize: 14
                font.bold: true
                color: "#FFFFFF"
            }
            
            // 垂直音量滑块
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
                    volumeWindow.volumeChanged(Math.round(value))
                }
                
                background: Rectangle {
                    x: volumeSlider.leftPadding + volumeSlider.availableWidth / 2 - width / 2
                    y: volumeSlider.topPadding
                    width: 6
                    height: volumeSlider.availableHeight
                    radius: 3
                    color: "#40FFFFFF"  // 半透明白色
                    
                    Rectangle {
                        y: volumeSlider.visualPosition * parent.height
                        width: parent.width
                        height: parent.height - y
                        color: "#1DB954"  // 绿色填充
                        radius: 3
                    }
                }
                
                handle: Rectangle {
                    x: volumeSlider.leftPadding + volumeSlider.availableWidth / 2 - width / 2
                    y: volumeSlider.topPadding + volumeSlider.visualPosition * (volumeSlider.availableHeight - height)
                    width: 18
                    height: 18
                    radius: 9
                    color: volumeSlider.pressed ? "#1DB954" : "#FFFFFF"
                    border.color: "#1DB954"
                    border.width: 2
                    
                    // 悬停和按下效果
                    scale: volumeSlider.pressed ? 1.3 : (volumeSlider.hovered ? 1.2 : 1.0)
                    
                    Behavior on scale {
                        NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
                    }
                    
                    // 阴影
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
