import QtQuick 2.12

Item {
    id: root
    
    property string imageSource: "qrc:/new/prefix1/icon/maxresdefault.jpg"
    property real rotationAngle: 0
    property bool isRotating: false
    
    // 澶栧眰榛戣壊鍦嗙幆
    Rectangle {
        id: outerCircle
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height)
        height: width
        radius: width / 2
        color: "black"
        
        // 鍐呭眰鏃嬭浆鍥剧墖
        Item {
            id: innerCircle
            anchors.centerIn: parent
            width: parent.width * 0.7
            height: width
            rotation: root.rotationAngle
            clip: true
            
            // 鍦嗗舰閬僵
            Rectangle {
                id: mask
                anchors.fill: parent
                radius: width / 2
                visible: false
            }
            
            // 涓撹緫灏侀潰鍥剧墖
            Image {
                id: albumImage
                anchors.fill: parent
                source: root.imageSource
                fillMode: Image.PreserveAspectCrop
                smooth: true
                antialiasing: true
                asynchronous: true
                cache: false
                
                // 鍦嗗舰瑁佸壀
                layer.enabled: true
                layer.effect: ShaderEffect {
                    property variant source: albumImage
                    
                    fragmentShader: "
                        uniform lowp sampler2D source;
                        varying highp vec2 qt_TexCoord0;
                        void main() {
                            lowp vec2 center = vec2(0.5, 0.5);
                            lowp float dist = distance(qt_TexCoord0, center);
                            lowp float alpha = dist < 0.5 ? 1.0 : 0.0;
                            gl_FragColor = texture2D(source, qt_TexCoord0) * alpha;
                        }
                    "
                }
            }
        }
    }
    
    // 鏃嬭浆鍔ㄧ敾
    RotationAnimator {
        id: rotationAnimator
        target: innerCircle
        from: innerCircle.rotation
        to: innerCircle.rotation + 360
        duration: 12000  // 12绉掕浆涓€鍦?
        loops: Animation.Infinite
        running: root.isRotating
        
        onStopped: {
            // 淇濇寔褰撳墠瑙掑害
            root.rotationAngle = innerCircle.rotation % 360
        }
    }
    
    // 鍏叡鏂规硶
    function startRotation() {
        isRotating = true
    }
    
    function stopRotation() {
        isRotating = false
    }
    
    function setImage(imagePath) {
        // 澶勭悊璺緞鏍煎紡
        if (imagePath.startsWith("file:///")) {
            root.imageSource = imagePath
        } else if (imagePath.startsWith("qrc:/")) {
            root.imageSource = imagePath
        } else if (imagePath.startsWith(":/")) {
            root.imageSource = "qrc" + imagePath
        } else {
            root.imageSource = "file:///" + imagePath
        }
        console.log("RotatingCircle: Image set to:", root.imageSource)
    }
}

