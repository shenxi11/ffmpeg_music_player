#include "VideoRendererGL.h"
#include "VideoBuffer.h"
#include "AudioPlayer.h"
#include <QDebug>

// YUV420P 到 RGB 的 Fragment Shader
static const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D texY;
uniform sampler2D texU;
uniform sampler2D texV;

void main()
{
    float y = texture(texY, TexCoord).r;
    float u = texture(texU, TexCoord).r - 0.5;
    float v = texture(texV, TexCoord).r - 0.5;
    
    // BT.709 YUV to RGB conversion
    float r = y + 1.5748 * v;
    float g = y - 0.1873 * u - 0.4681 * v;
    float b = y + 1.8556 * u;
    
    FragColor = vec4(r, g, b, 1.0);
}
)";

// Vertex Shader
static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

VideoRendererGL::VideoRendererGL(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_playing(false)
    , m_buffering(false)
    , m_bufferingThreshold(15)
    , m_lastPTS(0)
    , m_shaderProgram(nullptr)
    , m_textureY(0)
    , m_textureU(0)
    , m_textureV(0)
    , m_vao(0)
    , m_vbo(0)
    , m_currentFrame(nullptr)
    , m_frameUpdated(false)
    , m_renderTimer(new QTimer(this))
    , m_targetFPS(30)
    , m_frameInterval(33)
    , m_buffer(nullptr)
    , m_glInitialized(false)
{
    qDebug() << "[VideoRendererGL] Created";
    
    // 设置OpenGL格式
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);
    
    // 设置渲染定时器
    connect(m_renderTimer, &QTimer::timeout, this, &VideoRendererGL::renderNextFrame);
}

VideoRendererGL::~VideoRendererGL()
{
    makeCurrent();
    cleanupGL();
    doneCurrent();
    
    if (m_currentFrame) {
        delete m_currentFrame;
        m_currentFrame = nullptr;
    }
    
    qDebug() << "[VideoRendererGL] Destroyed";
}

void VideoRendererGL::initializeGL()
{
    initializeOpenGLFunctions();
    
    qDebug() << "[VideoRendererGL] Initializing OpenGL";
    qDebug() << "  OpenGL Version:" << (const char*)glGetString(GL_VERSION);
    qDebug() << "  GLSL Version:" << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    qDebug() << "  Renderer:" << (const char*)glGetString(GL_RENDERER);
    
    // 设置清除颜色（黑色）
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    // 初始化shader
    if (!initShaders()) {
        qWarning() << "[VideoRendererGL] Failed to initialize shaders";
        return;
    }
    
    // 创建顶点数据（全屏四边形）
    float vertices[] = {
        // 位置          // 纹理坐标
        -1.0f,  1.0f, 0.0f,  0.0f, 0.0f,  // 左上
        -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,  // 左下
         1.0f, -1.0f, 0.0f,  1.0f, 1.0f,  // 右下
         
        -1.0f,  1.0f, 0.0f,  0.0f, 0.0f,  // 左上
         1.0f, -1.0f, 0.0f,  1.0f, 1.0f,  // 右下
         1.0f,  1.0f, 0.0f,  1.0f, 0.0f   // 右上
    };
    
    // 创建VAO和VBO
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 纹理坐标属性
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // 创建纹理（初始为空）
    glGenTextures(1, &m_textureY);
    glGenTextures(1, &m_textureU);
    glGenTextures(1, &m_textureV);
    
    m_glInitialized = true;
    qDebug() << "[VideoRendererGL] OpenGL initialized successfully";
}

bool VideoRendererGL::initShaders()
{
    m_shaderProgram = new QOpenGLShaderProgram(this);
    
    // 编译vertex shader
    if (!m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qWarning() << "[VideoRendererGL] Vertex shader compilation failed:" << m_shaderProgram->log();
        return false;
    }
    
    // 编译fragment shader
    if (!m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qWarning() << "[VideoRendererGL] Fragment shader compilation failed:" << m_shaderProgram->log();
        return false;
    }
    
    // 链接程序
    if (!m_shaderProgram->link()) {
        qWarning() << "[VideoRendererGL] Shader program linking failed:" << m_shaderProgram->log();
        return false;
    }
    
    // 绑定纹理单元
    m_shaderProgram->bind();
    m_shaderProgram->setUniformValue("texY", 0);
    m_shaderProgram->setUniformValue("texU", 1);
    m_shaderProgram->setUniformValue("texV", 2);
    m_shaderProgram->release();
    
    qDebug() << "[VideoRendererGL] Shaders compiled and linked successfully";
    return true;
}

void VideoRendererGL::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    // 无锁设计：直接访问m_currentFrame（renderNextFrame在同一线程）
    if (!m_currentFrame || !m_frameUpdated) {
        return;
    }
    
    // 上传纹理数据
    updateTextures(m_currentFrame);
    m_frameUpdated = false;
    
    // 使用shader程序
    m_shaderProgram->bind();
    
    // 绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureY);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textureU);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_textureV);
    
    // 绘制四边形
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    m_shaderProgram->release();
}

void VideoRendererGL::updateTextures(VideoFrame* frame)
{
    if (!frame || !frame->data[0]) {
        return;
    }
    
    int width = frame->width;
    int height = frame->height;
    
    // 如果视频尺寸变化，更新缓冲区
    if (m_videoSize.width() != width || m_videoSize.height() != height) {
        m_videoSize = QSize(width, height);
        updateVertexBuffer(this->width(), this->height());
    }
    
    // 更新Y平面纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureY);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // 更新U平面纹理
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textureU);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width/2, height/2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // 更新V平面纹理
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_textureV);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width/2, height/2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void VideoRendererGL::updateVertexBuffer(int windowWidth, int windowHeight)
{
    if (windowWidth <= 0 || windowHeight <= 0 || m_videoSize.width() <= 0 || m_videoSize.height() <= 0) {
        return;
    }
    
    // 计算视频和窗口的宽高比
    float videoAspect = (float)m_videoSize.width() / (float)m_videoSize.height();
    float windowAspect = (float)windowWidth / (float)windowHeight;
    
    qDebug() << "[VideoRendererGL] updateVertexBuffer - Video:" << m_videoSize.width() << "x" << m_videoSize.height() 
             << "Window:" << windowWidth << "x" << windowHeight
             << "VideoAspect:" << videoAspect << "WindowAspect:" << windowAspect;
    
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    
    // 保持宽高比，适应窗口大小
    if (videoAspect > windowAspect) {
        // 视频更宽，以宽度为准
        scaleY = windowAspect / videoAspect;
        qDebug() << "[VideoRendererGL] Video wider, scaleY:" << scaleY << "Display size:" << windowWidth << "x" << (windowHeight * scaleY);
    } else {
        // 视频更高，以高度为准
        scaleX = videoAspect / windowAspect;
        qDebug() << "[VideoRendererGL] Video taller, scaleX:" << scaleX << "Display size:" << (windowWidth * scaleX) << "x" << windowHeight;
    }
    
    // 更新顶点数据
    float vertices[] = {
        // 位置                               // 纹理坐标
        -scaleX,  scaleY, 0.0f,  0.0f, 0.0f,  // 左上
        -scaleX, -scaleY, 0.0f,  0.0f, 1.0f,  // 左下
         scaleX, -scaleY, 0.0f,  1.0f, 1.0f,  // 右下
         
        -scaleX,  scaleY, 0.0f,  0.0f, 0.0f,  // 左上
         scaleX, -scaleY, 0.0f,  1.0f, 1.0f,  // 右下
         scaleX,  scaleY, 0.0f,  1.0f, 0.0f   // 右上
    };
    
    // 更新VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VideoRendererGL::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    
    // 如果有视频尺寸，计算缩放比例以保持宽高比
    if (m_videoSize.width() > 0 && m_videoSize.height() > 0) {
        updateVertexBuffer(w, h);
    }
}

void VideoRendererGL::cleanupGL()
{
    if (m_shaderProgram) {
        delete m_shaderProgram;
        m_shaderProgram = nullptr;
    }
    
    if (m_textureY) {
        glDeleteTextures(1, &m_textureY);
        m_textureY = 0;
    }
    if (m_textureU) {
        glDeleteTextures(1, &m_textureU);
        m_textureU = 0;
    }
    if (m_textureV) {
        glDeleteTextures(1, &m_textureV);
        m_textureV = 0;
    }
    
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    
    m_glInitialized = false;
}

void VideoRendererGL::start()
{
    qDebug() << "[VideoRendererGL] Start";
    
    m_playing = true;
    m_renderTimer->start(m_frameInterval);
}

void VideoRendererGL::pause()
{
    qDebug() << "[VideoRendererGL] Pause";
    
    m_playing = false;
    m_buffering = false;
    m_renderTimer->stop();
}

void VideoRendererGL::stop()
{
    qDebug() << "[VideoRendererGL] Stop";
    
    m_playing = false;
    m_buffering = false;
    m_renderTimer->stop();
}

void VideoRendererGL::startBuffering()
{
    qDebug() << "[VideoRendererGL] Start buffering (waiting for" << m_bufferingThreshold << "frames)";
    m_buffering = true;
}

void VideoRendererGL::setVideoBuffer(VideoBuffer* buffer)
{
    m_buffer = buffer;
}

void VideoRendererGL::setTargetFPS(int fps)
{
    m_targetFPS = fps;
    m_frameInterval = 1000 / fps;
    
    qDebug() << "[VideoRendererGL] Target FPS set to:" << fps << "Interval:" << m_frameInterval << "ms";
    
    if (m_playing) {
        m_renderTimer->setInterval(m_frameInterval);
    }
}

void VideoRendererGL::renderNextFrame()
{
    static int callCount = 0;
    callCount++;
    
    if (callCount <= 20) {
        qDebug() << "[VideoRendererGL] renderNextFrame called, count:" << callCount;
    }
    
    if (!m_playing || !m_buffer) {
        if (callCount <= 20) {
            qDebug() << "[VideoRendererGL] renderNextFrame blocked - playing:" << m_playing << "buffer:" << (m_buffer != nullptr);
        }
        return;
    }
    
    // 检查缓冲状态：seek后等待缓冲区填充
    if (m_buffering) {
        int bufferSize = m_buffer->size();
        qDebug() << "[VideoRendererGL] Buffering... buffer size:" << bufferSize << "/" << m_bufferingThreshold;
        if (bufferSize >= m_bufferingThreshold) {
            m_buffering = false;
            qDebug() << "[VideoRendererGL] Buffering complete, buffer size:" << bufferSize;
        } else {
            // 继续等待缓冲
            return;
        }
    }
    
    // 使用音频时钟作为主时钟（音视频同步）
    qint64 audioClock = AudioPlayer::instance().getPlaybackPosition();
    
    // 从缓冲区查看下一帧
    VideoFrame* frame = m_buffer->peek();
    if (!frame) {
        static int emptyCounter = 0;
        if (emptyCounter++ % 10 == 0) {
            qDebug() << "[VideoRendererGL] No frame in buffer (empty" << emptyCounter << "times)";
        }
        return;  // 缓冲区空，等待更多帧
    }
    
    // 调试：打印每一帧的时间信息（前10帧）
    static int debugCounter = 0;
    debugCounter++;
    if (debugCounter <= 10) {  // 前10帧每帧都打印
        qint64 diff = frame->pts - audioClock;
        qDebug() << "[VideoRendererGL] Frame" << debugCounter 
                 << "audioClock:" << audioClock 
                 << "frame->pts:" << frame->pts 
                 << "diff:" << diff
                 << "buffer_size:" << m_buffer->size();
    } else if (debugCounter % 30 == 0) {  // 之后每30帧打印一次
        qint64 diff = frame->pts - audioClock;
        qDebug() << "[VideoRendererGL] audioClock:" << audioClock 
                 << "frame->pts:" << frame->pts 
                 << "diff:" << diff
                 << "buffer_size:" << m_buffer->size();
    }
    
    // 同步逻辑：如果视频帧超前音频时钟超过100ms，等待
    // 注意：容忍度从40ms增加到100ms，避免因短暂的时间波动导致等待
    if (frame->pts > audioClock + 100) {
        static int waitCounter = 0;
        if (waitCounter++ % 10 == 0) {  // 每10次等待打印一次
            qDebug() << "[VideoRendererGL] Video ahead of audio, waiting..." << "video PTS:" << frame->pts << "audio clock:" << audioClock;
        }
        return;
    }
    
    // 如果视频严重落后（超过200ms），连续跳过帧直到追上
    // 注意：阈值从100ms增加到200ms，减少不必要的跳帧
    int skippedCount = 0;
    while (frame && audioClock > frame->pts + 200) {
        skippedCount++;
        m_buffer->pop();  // 从缓冲区移除
        delete frame;     // 释放内存
        frame = m_buffer->peek();  // 查看下一帧
        
        // 安全限制：最多跳过100帧，防止无限循环
        if (skippedCount >= 100) {
            qDebug() << "[VideoRendererGL] WARNING: Skipped" << skippedCount << "frames, stopping to prevent infinite loop";
            break;
        }
    }
    
    if (skippedCount > 0) {
        qDebug() << "[VideoRendererGL] Skipped" << skippedCount << "frames to catch up (audio:" << audioClock << "ms)";
    }
    
    // 如果跳帧后没有更多帧，等待
    if (!frame) {
        qDebug() << "[VideoRendererGL] No more frames after skipping" << skippedCount << "frames";
        return;
    }
    
    // 从缓冲区获取当前同步的帧
    frame = m_buffer->pop();
    if (!frame) {
        qDebug() << "[VideoRendererGL] ERROR: pop() returned nullptr after peek() succeeded!";
        return;
    }
    
    // 前10帧打印pop确认
    static int popCount = 0;
    popCount++;
    if (popCount <= 10) {
        qDebug() << "[VideoRendererGL] Popped frame" << popCount << "PTS:" << frame->pts;
    }
    
    // 更新视频尺寸
    if (m_videoSize.width() != frame->width || m_videoSize.height() != frame->height) {
        qDebug() << "[VideoRendererGL] Video size changed from" << m_videoSize << "to" << QSize(frame->width, frame->height);
        m_videoSize = QSize(frame->width, frame->height);
        updateVertexBuffer(this->width(), this->height());
        qDebug() << "[VideoRendererGL] updateVertexBuffer called with window size:" << this->width() << "x" << this->height();
        emit videoSizeChanged(m_videoSize);
        qDebug() << "[VideoRendererGL] videoSizeChanged signal emitted";
    }
    
    // 更新当前帧（无锁设计）
    if (popCount <= 10) {
        qDebug() << "[VideoRendererGL] Updating current frame...";
    }
    
    // 先保存旧帧
    VideoFrame* oldFrame = m_currentFrame;
    
    if (popCount <= 10) {
        qDebug() << "[VideoRendererGL] Saved old frame:" << oldFrame;
    }
    
    // 设置新帧（原子操作）
    m_currentFrame = frame;
    m_lastPTS = frame->pts;
    m_frameUpdated = true;
    
    if (popCount <= 10) {
        qDebug() << "[VideoRendererGL] New frame set, will delete old frame...";
    }
    
    // 删除旧帧（在这之前先触发update，让paintGL使用新帧）
    if (oldFrame) {
        if (popCount <= 10) {
            qDebug() << "[VideoRendererGL] Deleting old frame...";
        }
        delete oldFrame;
        oldFrame = nullptr;
        if (popCount <= 10) {
            qDebug() << "[VideoRendererGL] Old frame deleted, pointer nullified";
        }
    }
    
    if (popCount <= 10) {
        qDebug() << "[VideoRendererGL] Current frame updated";
    }
    
    // 前10帧打印渲染确认
    static int renderCount = 0;
    renderCount++;
    if (renderCount <= 10) {
        qDebug() << "[VideoRendererGL] Rendered frame" << renderCount << "PTS:" << frame->pts;
    }
    
    // 在锁外调用update()，避免死锁
    update();
    
    // 发送信号（在锁外）
    emit frameRendered(frame->pts);
}
