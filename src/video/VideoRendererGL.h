#ifndef VIDEORENDERERGL_H
#define VIDEORENDERERGL_H

#include "VideoDecoder.h"

#include <QElapsedTimer>
#include <QMutex>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <QTimer>

class VideoBuffer;

class VideoRendererGL : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT

  public:
    enum QualityPreset { Standard1080P = 0, Enhanced2K = 1 };

    explicit VideoRendererGL(QWidget* parent = nullptr);
    ~VideoRendererGL() override;

    bool isPlaying() const {
        return m_playing;
    }
    QSize videoSize() const {
        return m_videoSize;
    }

  public slots:
    void start();
    void pause();
    void stop();
    void startBuffering();

    void setVideoBuffer(VideoBuffer* buffer);
    void setTargetFPS(int fps);
    void setPixelAspectRatio(int num, int den);
    void setDisplayMode(int mode);     // 0: Fit, 1: Fill
    void setQualityPreset(int preset); // 0: 标准(1080P), 1: 增强(2K)
    void setFullscreenTransitionActive(bool active);

    qint64 lastPTS() const {
        return m_lastPTS;
    }

  signals:
    void frameRendered(qint64 pts);
    void videoSizeChanged(QSize size);

  protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

  private slots:
    void renderNextFrame();

  private:
    bool initShaders();
    bool initPresentShaders();
    void ensureVertexBufferUpdated();
    void updateTextures(VideoFrame* frame);
    void updateVertexBuffer(int windowWidth, int windowHeight);
    void ensureRenderTarget(int width, int height);
    QSize calculateInternalRenderSize(int screenWidth, int screenHeight) const;
    void cleanupGL();

  private:
    bool m_playing;
    bool m_buffering;
    int m_bufferingThreshold;
    int m_bufferingTimeoutMs;
    bool m_bufferingTimeoutLogged;
    QElapsedTimer m_bufferingTimer;
    qint64 m_earlyWaitPts;
    bool m_earlyWaitLogged;
    QElapsedTimer m_earlyWaitTimer;
    QSize m_videoSize;
    float m_pixelAspectRatio;
    int m_displayMode;
    qint64 m_lastPTS;

    QOpenGLShaderProgram* m_shaderProgram;
    QOpenGLShaderProgram* m_presentShaderProgram;
    GLuint m_textureY;
    GLuint m_textureU;
    GLuint m_textureV;
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_fullscreenVao;
    GLuint m_fullscreenVbo;
    GLuint m_rgbFbo;
    GLuint m_rgbTexture;
    int m_rgbFboWidth;
    int m_rgbFboHeight;

    QMutex m_frameMutex;
    VideoFrame* m_currentFrame;
    bool m_frameUpdated;
    bool m_vertexBufferDirty;

    QTimer* m_renderTimer;
    int m_targetFPS;
    int m_frameInterval;

    VideoBuffer* m_buffer;
    bool m_glInitialized;

    QualityPreset m_qualityPreset;
    int m_renderMaxWidth;
    int m_renderMaxHeight;
    float m_sharpenStrength;
    bool m_enableHighQualityScaling;
    bool m_fullscreenTransitionActive;
    QSize m_pendingRenderTargetSize;
};

#endif // VIDEORENDERERGL_H
