import QtQuick 2.12

Rectangle {
    id: root
    
    property bool isUp: false
    property string backgroundImageSource: ""
    property string songName: ""
    
    // 鏍规嵁灞曞紑鐘舵€佽缃儗鏅壊
    color: isUp ? "transparent" : "#FAFAFA"
    
    // 鍐呭鍖哄煙
    Item {
        anchors.fill: parent
        
        // 姝岃瘝鏄剧ず鍖哄煙 - 鍙湪灞曞紑鏃舵樉绀?        Item {
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
            
            // 姝屾洸鍚嶆爣绛?            Text {
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
            
            // 姝岃瘝鏄剧ず缁勪欢
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
        
        // 鎺у埗鏍忓鍣?        Item {
            id: controlBarContainer
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 100
            
            // 鎺у埗鏍忚儗鏅?            Rectangle {
                anchors.fill: parent
                color: isUp ? "rgba(44, 62, 80, 0.9)" : "#FAFAFA"
                
                // 椤堕儴鍒嗗壊绾?                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 1
                    color: isUp ? "#555" : "#E0E0E0"
                }
            }
            
            // 鍔ㄦ€佸姞杞芥帶鍒舵爮缁勪欢
            Loader {
                id: controlBarLoader
                anchors.fill: parent
                source: "qrc:/qml/components/ProcessSlider.qml"
                
                onLoaded: {
                    if (item) {
                        // 璁剧疆鍒濆鐘舵€?                        item.isUp = root.isUp
                        
                        // 杩炴帴鎺у埗鏍忎俊鍙?                        item.seekTo.connect(root.onSeekTo)
                        item.upClicked.connect(root.onUpClicked)
                    }
                }
            }
        }
    }
    
    // 蹇呰鐨勪俊鍙?    signal upClicked()
    signal sliderMoved(int seconds)
    signal signal_Slider_Move(int seconds)
    signal signal_big_clicked(bool checked)
    
    // 蹇呰鐨勫嚱鏁?    function setIsUp(flag) {
        isUp = flag
        console.log("PlayWidget setIsUp called with:", flag)
        
        // 鍚屾鏇存柊鎺у埗鏍忕殑 isUp 鐘舵€?        if (controlBarLoader.item) {
            controlBarLoader.item.isUp = flag
        }
        
        // 鍚屾鏇存柊姝岃瘝鏄剧ず缁勪欢鐨勭姸鎬?        if (lyricDisplayLoader.item) {
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
    
    // 澶勭悊鎺у埗鏍忎俊鍙风殑鍑芥暟
    function onSeekTo(seconds) {
        console.log("Seek to:", seconds)
        signal_Slider_Move(seconds)
    }
    
    function onUpClicked() {
        console.log("Up button clicked, current isUp:", isUp)
        signal_big_clicked(!isUp)
    }
    
    // 娣诲姞缂哄け鐨勬柟娉曟潵閬垮厤璋冪敤閿欒
    function setPosition(position) {
        console.log("setPosition called with:", position)
        // 灏嗕綅缃俊鎭紶閫掔粰鎺у埗鏍?        if (controlBarLoader.item && controlBarLoader.item.setPosition) {
            controlBarLoader.item.setPosition(position)
        }
    }
    
    function setDuration(duration) {
        console.log("setDuration called with:", duration)
        // 灏嗘椂闀夸俊鎭紶閫掔粰鎺у埗鏍?        if (controlBarLoader.item && controlBarLoader.item.setMaxSeconds) {
            controlBarLoader.item.setMaxSeconds(duration)
        }
    }
    
    function setState(state) {
        console.log("setState called with:", state)
        // 灏嗘挱鏀剧姸鎬佷紶閫掔粰鎺у埗鏍?        if (controlBarLoader.item && controlBarLoader.item.setState) {
            controlBarLoader.item.setState(state)
        }
    }
    
    function setPicPath(imagePath) {
        console.log("setPicPath called with:", imagePath)
        // 灏嗕笓杈戝浘鐗囦紶閫掔粰鎺у埗鏍?        if (controlBarLoader.item && controlBarLoader.item.setPicPath) {
            controlBarLoader.item.setPicPath(imagePath)
        }
    }
    
    // 姝岃瘝鐩稿叧鍑芥暟
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
    
    // 娴嬭瘯榧犳爣鐐瑰嚮
    MouseArea {
        anchors.fill: parent
        onClicked: {
            console.log("PlayWidget clicked!")
            signal_big_clicked(!isUp)
        }
    }
}

