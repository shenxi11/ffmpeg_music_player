#ifndef VIDEORENDERERGL_H
#define VIDEORENDERERGL_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QTimer>
#include <QMutex>
#include "VideoDecoder.h"

// 前向声明
class VideoBuffer;

/**
 * @brief OpenGL硬件加速视频渲染器
 * 
 * 特性：
 * - GPU YUV420P → RGB转换（shader）
 * - 硬件加速缩放和混合
 * - 支持高分辨率高帧率（4K@60fps）
 * - 零拷贝纹理上传
 */
class VideoRendererGL : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
    
public:
    explicit VideoRendererGL(QWidget* parent = nullptr);
    ~VideoRendererGL() override;
    
    // ===== 状态查询 =====
    bool isPlaying() const { return m_playing; }
    QSize videoSize() const { return m_videoSize; }
    
public slots:
    // ===== 渲染控制 =====
    void start();
    void pause();
    void stop();
    void startBuffering();  // 开始缓冲（seek后调用）
    
    // ===== 设置 =====
    void setVideoBuffer(VideoBuffer* buffer);
    void setTargetFPS(int fps);
    
    // ===== 状态查询（返回值） =====
    qint64 lastPTS() const { return m_lastPTS; }
    
signals:
    void frameRendered(qint64 pts);
    void videoSizeChanged(QSize size);
    
protected:
    // OpenGL 回调
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    
private slots:
    void renderNextFrame();
    
private:
    bool initShaders();
    void updateTextures(VideoFrame* frame);
    void updateVertexBuffer(int windowWidth, int windowHeight);
    void cleanupGL();
    
private:
    // 渲染状态
    bool m_playing;
    bool m_buffering;
    int m_bufferingThreshold;
    QSize m_videoSize;
    qint64 m_lastPTS;
    
    // OpenGL资源
    QOpenGLShaderProgram* m_shaderProgram;
    GLuint m_textureY;  // Y平面纹理
    GLuint m_textureU;  // U平面纹理
    GLuint m_textureV;  // V平面纹理
    GLuint m_vao;       // 顶点数组对象
    GLuint m_vbo;       // 顶点缓冲对象
    
    // 当前帧数据（保护）
    QMutex m_frameMutex;
    VideoFrame* m_currentFrame;
    bool m_frameUpdated;
    
    // 帧率控制
    QTimer* m_renderTimer;
    int m_targetFPS;
    int m_frameInterval;
    
    // 缓冲区
    VideoBuffer* m_buffer;
    
    // OpenGL初始化标志
    bool m_glInitialized;
};

#endif // VIDEORENDERERGL_H
