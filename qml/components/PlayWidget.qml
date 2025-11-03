import QtQuick 2.12

Rectangle {
    id: root
    
    property bool isUp: false
    property string backgroundImageSource: ""
    property string songName: ""
    
    // 根据展开状态设置背景色
    color: isUp ? "transparent" : "#FAFAFA"
    
    // 内容区域
    Item {
        anchors.fill: parent
        
        // 歌词显示区域 - 只在展开时显示
        Item {
            id: lyricContainer
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: controlBarContainer.top
            anchors.margins: 20
            
            visible: isUp
            opacity: isUp ? 1.0 : 0.0
            
            Behavior on opacity {
                NumberAnimation { duration: 300 }
            }
            
            // 歌曲名标签
            Text {
                id: songNameLabel
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 20
                
                text: root.songName
                color: "white"
                font.pixelSize: 24
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }
            
            // 歌词显示组件
            Loader {
                id: lyricDisplayLoader
                anchors.top: songNameLabel.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.topMargin: 30
                
                source: "qrc:/qml/components/LyricDisplay.qml"
                
                onLoaded: {
                    if (item) {
                        console.log("LyricDisplay loaded successfully")
                        item.isUp = root.isUp
                    }
                }
            }
        }
        
        // 控制栏容器
        Item {
            id: controlBarContainer
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 100
            
            // 控制栏背景
            Rectangle {
                anchors.fill: parent
                color: isUp ? "rgba(44, 62, 80, 0.9)" : "#FAFAFA"
                
                // 顶部分割线
                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 1
                    color: isUp ? "#555" : "#E0E0E0"
                }
            }
            
            // 动态加载控制栏组件
            Loader {
                id: controlBarLoader
                anchors.fill: parent
                source: "qrc:/qml/components/ProcessSlider.qml"
                
                onLoaded: {
                    if (item) {
                        // 设置初始状态
                        item.isUp = root.isUp
                        
                        // 连接控制栏信号
                        item.seekTo.connect(root.onSeekTo)
                        item.upClicked.connect(root.onUpClicked)
                    }
                }
            }
        }
    }
    
    // 必要的信号
    signal upClicked()
    signal sliderMoved(int seconds)
    signal signal_Slider_Move(int seconds)
    signal signal_big_clicked(bool checked)
    
    // 必要的函数
    function setIsUp(flag) {
        isUp = flag
        console.log("PlayWidget setIsUp called with:", flag)
        
        // 同步更新控制栏的 isUp 状态
        if (controlBarLoader.item) {
            controlBarLoader.item.isUp = flag
        }
        
        // 同步更新歌词显示组件的状态
        if (lyricDisplayLoader.item) {
            lyricDisplayLoader.item.isUp = flag
        }
    }
    
    function setBackgroundImage(imagePath) {
        backgroundImageSource = imagePath
        console.log("PlayWidget setBackgroundImage called with:", imagePath)
    }
    
    function setSongName(name) {
        songName = name
        console.log("PlayWidget setSongName called with:", name)
    }
    
    // 处理控制栏信号的函数
    function onSeekTo(seconds) {
        console.log("Seek to:", seconds)
        signal_Slider_Move(seconds)
    }
    
    function onUpClicked() {
        console.log("Up button clicked, current isUp:", isUp)
        signal_big_clicked(!isUp)
    }
    
    // 添加缺失的方法来避免调用错误
    function setPosition(position) {
        console.log("setPosition called with:", position)
        // 将位置信息传递给控制栏
        if (controlBarLoader.item && controlBarLoader.item.setPosition) {
            controlBarLoader.item.setPosition(position)
        }
    }
    
    function setDuration(duration) {
        console.log("setDuration called with:", duration)
        // 将时长信息传递给控制栏
        if (controlBarLoader.item && controlBarLoader.item.setMaxSeconds) {
            controlBarLoader.item.setMaxSeconds(duration)
        }
    }
    
    function setState(state) {
        console.log("setState called with:", state)
        // 将播放状态传递给控制栏
        if (controlBarLoader.item && controlBarLoader.item.setState) {
            controlBarLoader.item.setState(state)
        }
    }
    
    function setPicPath(imagePath) {
        console.log("setPicPath called with:", imagePath)
        // 将专辑图片传递给控制栏
        if (controlBarLoader.item && controlBarLoader.item.setPicPath) {
            controlBarLoader.item.setPicPath(imagePath)
        }
    }
    
    // 歌词相关函数
    function setLyrics(lyrics) {
        console.log("setLyrics called with:", lyrics)
        if (lyricDisplayLoader.item && lyricDisplayLoader.item.setLyrics) {
            lyricDisplayLoader.item.setLyrics(lyrics)
        }
    }
    
    function highlightLyricLine(lineNumber) {
        console.log("highlightLyricLine called with:", lineNumber)
        if (lyricDisplayLoader.item && lyricDisplayLoader.item.highlightLine) {
            lyricDisplayLoader.item.highlightLine(lineNumber)
        }
    }
    
    function clearLyrics() {
        console.log("clearLyrics called")
        if (lyricDisplayLoader.item && lyricDisplayLoader.item.clearLyrics) {
            lyricDisplayLoader.item.clearLyrics()
        }
    }
    
    // 测试鼠标点击
    MouseArea {
        anchors.fill: parent
        onClicked: {
            console.log("PlayWidget clicked!")
            signal_big_clicked(!isUp)
        }
    }
}