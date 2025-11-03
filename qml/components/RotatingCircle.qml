import QtQuick 2.12

Item {
    id: root
    
    property string imageSource: "qrc:/new/prefix1/icon/maxresdefault.jpg"
    property real rotationAngle: 0
    property bool isRotating: false
    
    // 外层黑色圆环
    Rectangle {
        id: outerCircle
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height)
        height: width
        radius: width / 2
        color: "black"
        
        // 内层旋转图片
        Item {
            id: innerCircle
            anchors.centerIn: parent
            width: parent.width * 0.7
            height: width
            rotation: root.rotationAngle
            clip: true
            
            // 圆形遮罩
            Rectangle {
                id: mask
                anchors.fill: parent
                radius: width / 2
                visible: false
            }
            
            // 专辑封面图片
            Image {
                id: albumImage
                anchors.fill: parent
                source: root.imageSource
                fillMode: Image.PreserveAspectCrop
                smooth: true
                antialiasing: true
                asynchronous: true
                cache: false
                
                // 圆形裁剪
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
    
    // 旋转动画
    RotationAnimator {
        id: rotationAnimator
        target: innerCircle
        from: innerCircle.rotation
        to: innerCircle.rotation + 360
        duration: 12000  // 12秒转一圈
        loops: Animation.Infinite
        running: root.isRotating
        
        onStopped: {
            // 保持当前角度
            root.rotationAngle = innerCircle.rotation % 360
        }
    }
    
    // 公共方法
    function startRotation() {
        isRotating = true
    }
    
    function stopRotation() {
        isRotating = false
    }
    
    function setImage(imagePath) {
        // 处理路径格式
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
