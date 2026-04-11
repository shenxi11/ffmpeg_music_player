#include "VideoRendererGL.h"

#include "AudioPlayer.h"
#include "VideoBuffer.h"

#include <QDebug>
#include <QOpenGLContext>
#include <QSurfaceFormat>

namespace {

static const char* kVertexShaderSource = R"(
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

static const char* kYuvFragmentShaderSource = R"(
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

    // BT.709
    float r = y + 1.5748 * v;
    float g = y - 0.1873 * u - 0.4681 * v;
    float b = y + 1.8556 * u;

    FragColor = vec4(r, g, b, 1.0);
}
)";

static const char* kPresentFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D texRgb;
uniform vec2 texelSize;
uniform float sharpenStrength;
uniform int highQualityScaling;

float cubicWeight(float x)
{
    x = abs(x);
    if (x <= 1.0) {
        return (1.5 * x - 2.5) * x * x + 1.0;
    }
    if (x < 2.0) {
        return ((-0.5 * x + 2.5) * x - 4.0) * x + 2.0;
    }
    return 0.0;
}

vec3 sampleLinear(vec2 uv)
{
    return texture(texRgb, uv).rgb;
}

vec3 sampleBicubic(vec2 uv)
{
    vec2 textureSizeF = vec2(textureSize(texRgb, 0));
    vec2 xy = uv * textureSizeF - 0.5;
    vec2 base = floor(xy);
    vec2 frac = fract(xy);

    vec3 color = vec3(0.0);
    float weightSum = 0.0;

    for (int j = -1; j <= 2; ++j) {
        for (int i = -1; i <= 2; ++i) {
            vec2 samplePixel = base + vec2(float(i), float(j)) + 0.5;
            vec2 sampleUv = samplePixel / textureSizeF;
            float w = cubicWeight(float(i) - frac.x) * cubicWeight(float(j) - frac.y);
            color += texture(texRgb, sampleUv).rgb * w;
            weightSum += w;
        }
    }

    return color / max(weightSum, 1e-5);
}

void main()
{
    vec2 uv = vec2(TexCoord.x, 1.0 - TexCoord.y);
    vec3 color = (highQualityScaling == 1) ? sampleBicubic(uv) : sampleLinear(uv);

    vec3 blur = (
        sampleLinear(uv + vec2(texelSize.x, 0.0)) +
        sampleLinear(uv - vec2(texelSize.x, 0.0)) +
        sampleLinear(uv + vec2(0.0, texelSize.y)) +
        sampleLinear(uv - vec2(0.0, texelSize.y)) +
        color
    ) / 5.0;

    vec3 sharpened = color + sharpenStrength * (color - blur);
    FragColor = vec4(clamp(sharpened, 0.0, 1.0), 1.0);
}
)";

} // namespace

VideoRendererGL::VideoRendererGL(QWidget* parent)
    : QOpenGLWidget(parent), m_playing(false), m_buffering(false), m_bufferingThreshold(8),
      m_bufferingTimeoutMs(450), m_bufferingTimeoutLogged(false), m_earlyWaitPts(-1),
      m_earlyWaitLogged(false), m_videoSize(-1, -1), m_pixelAspectRatio(1.0f), m_displayMode(0),
      m_lastPTS(0), m_shaderProgram(nullptr), m_presentShaderProgram(nullptr), m_textureY(0),
      m_textureU(0), m_textureV(0), m_vao(0), m_vbo(0), m_fullscreenVao(0), m_fullscreenVbo(0),
      m_rgbFbo(0), m_rgbTexture(0), m_rgbFboWidth(0), m_rgbFboHeight(0), m_currentFrame(nullptr),
      m_frameUpdated(false), m_vertexBufferDirty(true), m_renderTimer(new QTimer(this)),
      m_targetFPS(30), m_frameInterval(33), m_buffer(nullptr), m_glInitialized(false),
      m_qualityPreset(Standard1080P), m_renderMaxWidth(1920), m_renderMaxHeight(1080),
      m_sharpenStrength(0.08f), m_enableHighQualityScaling(false),
      m_fullscreenTransitionActive(false), m_pendingRenderTargetSize() {
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);

    connect(m_renderTimer, &QTimer::timeout, this, &VideoRendererGL::renderNextFrame);
    qDebug() << "[VideoRendererGL] Created";
}

VideoRendererGL::~VideoRendererGL() {
    makeCurrent();
    cleanupGL();
    doneCurrent();

    if (m_currentFrame) {
        delete m_currentFrame;
        m_currentFrame = nullptr;
    }

    qDebug() << "[VideoRendererGL] Destroyed";
}

void VideoRendererGL::initializeGL() {
    initializeOpenGLFunctions();

    qDebug() << "[VideoRendererGL] Initializing OpenGL";
    qDebug() << "  OpenGL Version:" << reinterpret_cast<const char*>(glGetString(GL_VERSION));
    qDebug() << "  GLSL Version:"
             << reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    qDebug() << "  Renderer:" << reinterpret_cast<const char*>(glGetString(GL_RENDERER));

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    if (!initShaders() || !initPresentShaders()) {
        qWarning() << "[VideoRendererGL] Failed to initialize shader programs";
        return;
    }

    const float fullscreenVertices[] = {-1.0f, 1.0f, 0.0f, 0.0f,  0.0f, -1.0f, -1.0f, 0.0f,
                                        0.0f,  1.0f, 1.0f, -1.0f, 0.0f, 1.0f,  1.0f,  -1.0f,
                                        1.0f,  0.0f, 0.0f, 0.0f,  1.0f, -1.0f, 0.0f,  1.0f,
                                        1.0f,  1.0f, 1.0f, 0.0f,  1.0f, 0.0f};

    // Display quad (fit/fill)
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fullscreenVertices), fullscreenVertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Fullscreen quad for offscreen conversion
    glGenVertexArrays(1, &m_fullscreenVao);
    glGenBuffers(1, &m_fullscreenVbo);
    glBindVertexArray(m_fullscreenVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_fullscreenVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fullscreenVertices), fullscreenVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenTextures(1, &m_textureY);
    glGenTextures(1, &m_textureU);
    glGenTextures(1, &m_textureV);

    m_glInitialized = true;
    qDebug() << "[VideoRendererGL] OpenGL initialized";
}

bool VideoRendererGL::initShaders() {
    m_shaderProgram = new QOpenGLShaderProgram(this);
    if (!m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, kVertexShaderSource)) {
        qWarning() << "[VideoRendererGL] Vertex shader compile failed:" << m_shaderProgram->log();
        return false;
    }
    if (!m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                                  kYuvFragmentShaderSource)) {
        qWarning() << "[VideoRendererGL] YUV fragment shader compile failed:"
                   << m_shaderProgram->log();
        return false;
    }
    if (!m_shaderProgram->link()) {
        qWarning() << "[VideoRendererGL] YUV shader link failed:" << m_shaderProgram->log();
        return false;
    }

    m_shaderProgram->bind();
    m_shaderProgram->setUniformValue("texY", 0);
    m_shaderProgram->setUniformValue("texU", 1);
    m_shaderProgram->setUniformValue("texV", 2);
    m_shaderProgram->release();

    return true;
}

bool VideoRendererGL::initPresentShaders() {
    m_presentShaderProgram = new QOpenGLShaderProgram(this);
    if (!m_presentShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                                         kVertexShaderSource)) {
        qWarning() << "[VideoRendererGL] Present vertex shader compile failed:"
                   << m_presentShaderProgram->log();
        return false;
    }
    if (!m_presentShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                                         kPresentFragmentShaderSource)) {
        qWarning() << "[VideoRendererGL] Present fragment shader compile failed:"
                   << m_presentShaderProgram->log();
        return false;
    }
    if (!m_presentShaderProgram->link()) {
        qWarning() << "[VideoRendererGL] Present shader link failed:"
                   << m_presentShaderProgram->log();
        return false;
    }

    m_presentShaderProgram->bind();
    m_presentShaderProgram->setUniformValue("texRgb", 0);
    m_presentShaderProgram->release();

    return true;
}

void VideoRendererGL::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);
    ensureVertexBufferUpdated();

    if (!m_currentFrame || !m_shaderProgram || !m_presentShaderProgram) {
        return;
    }

    if (m_frameUpdated) {
        updateTextures(m_currentFrame);
        m_frameUpdated = false;
    }

    const qreal dpr = devicePixelRatioF();
    const int screenW = qMax(1, static_cast<int>(width() * dpr));
    const int screenH = qMax(1, static_cast<int>(height() * dpr));
    const QSize internalSize = calculateInternalRenderSize(screenW, screenH);

    if (m_fullscreenTransitionActive) {
        if (m_pendingRenderTargetSize != internalSize) {
            m_pendingRenderTargetSize = internalSize;
            qDebug() << "[VideoRendererGL] Deferring render target resize during fullscreen "
                        "transition to"
                     << internalSize.width() << "x" << internalSize.height();
        }
        if (m_rgbFbo == 0 || m_rgbTexture == 0) {
            ensureRenderTarget(internalSize.width(), internalSize.height());
            m_pendingRenderTargetSize = QSize();
        }
    } else {
        const QSize targetSize =
            m_pendingRenderTargetSize.isValid() ? m_pendingRenderTargetSize : internalSize;
        ensureRenderTarget(targetSize.width(), targetSize.height());
        m_pendingRenderTargetSize = QSize();
    }

    if (m_rgbFbo == 0 || m_rgbTexture == 0) {
        return;
    }

    // Pass 1: YUV -> RGB (offscreen)
    glBindFramebuffer(GL_FRAMEBUFFER, m_rgbFbo);
    glViewport(0, 0, m_rgbFboWidth, m_rgbFboHeight);

    m_shaderProgram->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureY);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textureU);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_textureV);

    glBindVertexArray(m_fullscreenVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    m_shaderProgram->release();

    // Pass 2: upscale/downscale + sharpening (onscreen)
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    glViewport(0, 0, screenW, screenH);

    m_presentShaderProgram->bind();
    m_presentShaderProgram->setUniformValue("texelSize", 1.0f / m_rgbFboWidth,
                                            1.0f / m_rgbFboHeight);
    m_presentShaderProgram->setUniformValue("sharpenStrength", m_sharpenStrength);
    m_presentShaderProgram->setUniformValue("highQualityScaling",
                                            m_enableHighQualityScaling ? 1 : 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_rgbTexture);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    m_presentShaderProgram->release();
}

void VideoRendererGL::updateTextures(VideoFrame* frame) {
    if (!frame || !frame->data[0]) {
        return;
    }

    const int width = frame->width;
    const int height = frame->height;

    if (m_videoSize.width() != width || m_videoSize.height() != height) {
        m_videoSize = QSize(width, height);
        updateVertexBuffer(this->width(), this->height());
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE,
                 frame->data[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textureU);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE,
                 frame->data[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_textureV);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE,
                 frame->data[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void VideoRendererGL::updateVertexBuffer(int windowWidth, int windowHeight) {
    if (windowWidth <= 0 || windowHeight <= 0 || m_videoSize.width() <= 0 ||
        m_videoSize.height() <= 0) {
        return;
    }

    QOpenGLContext* currentCtx = QOpenGLContext::currentContext();
    if (!currentCtx || currentCtx != context()) {
        m_vertexBufferDirty = true;
        return;
    }

    const float effectiveVideoWidth = static_cast<float>(m_videoSize.width()) * m_pixelAspectRatio;
    const float videoAspect = effectiveVideoWidth / static_cast<float>(m_videoSize.height());
    const float windowAspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

    float scaleX = 1.0f;
    float scaleY = 1.0f;

    if (m_displayMode == 0) {
        if (videoAspect > windowAspect) {
            scaleY = windowAspect / videoAspect;
        } else {
            scaleX = videoAspect / windowAspect;
        }
    } else {
        if (videoAspect > windowAspect) {
            scaleX = videoAspect / windowAspect;
        } else {
            scaleY = windowAspect / videoAspect;
        }
    }

    const float vertices[] = {-scaleX, scaleY, 0.0f,   0.0f,    0.0f,   -scaleX, -scaleY, 0.0f,
                              0.0f,    1.0f,   scaleX, -scaleY, 0.0f,   1.0f,    1.0f,    -scaleX,
                              scaleY,  0.0f,   0.0f,   0.0f,    scaleX, -scaleY, 0.0f,    1.0f,
                              1.0f,    scaleX, scaleY, 0.0f,    1.0f,   0.0f};

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_vertexBufferDirty = false;
}

void VideoRendererGL::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    if (m_videoSize.width() > 0 && m_videoSize.height() > 0) {
        updateVertexBuffer(w, h);
    }
}

void VideoRendererGL::ensureVertexBufferUpdated() {
    if (!m_vertexBufferDirty) {
        return;
    }
    if (m_videoSize.width() <= 0 || m_videoSize.height() <= 0) {
        return;
    }
    updateVertexBuffer(this->width(), this->height());
}

QSize VideoRendererGL::calculateInternalRenderSize(int screenWidth, int screenHeight) const {
    const int safeScreenW = qMax(1, screenWidth);
    const int safeScreenH = qMax(1, screenHeight);

    const int maxW = qMax(1, m_renderMaxWidth);
    const int maxH = qMax(1, m_renderMaxHeight);

    if (safeScreenW <= maxW && safeScreenH <= maxH) {
        return QSize(safeScreenW, safeScreenH);
    }

    const double scale =
        qMin(static_cast<double>(maxW) / safeScreenW, static_cast<double>(maxH) / safeScreenH);
    return QSize(qMax(1, static_cast<int>(safeScreenW * scale)),
                 qMax(1, static_cast<int>(safeScreenH * scale)));
}

void VideoRendererGL::ensureRenderTarget(int width, int height) {
    const int targetW = qMax(1, width);
    const int targetH = qMax(1, height);
    if (m_rgbFbo != 0 && m_rgbTexture != 0 && m_rgbFboWidth == targetW &&
        m_rgbFboHeight == targetH) {
        return;
    }

    if (m_rgbFbo) {
        glDeleteFramebuffers(1, &m_rgbFbo);
        m_rgbFbo = 0;
    }
    if (m_rgbTexture) {
        glDeleteTextures(1, &m_rgbTexture);
        m_rgbTexture = 0;
    }

    glGenFramebuffers(1, &m_rgbFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_rgbFbo);

    glGenTextures(1, &m_rgbTexture);
    glBindTexture(GL_TEXTURE_2D, m_rgbTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, targetW, targetH, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_rgbTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        qWarning() << "[VideoRendererGL] Failed to create render target" << targetW << "x"
                   << targetH;
        glDeleteFramebuffers(1, &m_rgbFbo);
        glDeleteTextures(1, &m_rgbTexture);
        m_rgbFbo = 0;
        m_rgbTexture = 0;
        m_rgbFboWidth = 0;
        m_rgbFboHeight = 0;
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
        return;
    }

    m_rgbFboWidth = targetW;
    m_rgbFboHeight = targetH;
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    qDebug() << "[VideoRendererGL] Render target resized to" << targetW << "x" << targetH
             << "preset:" << (m_qualityPreset == Enhanced2K ? "2K" : "1080P");
}

void VideoRendererGL::cleanupGL() {
    if (m_shaderProgram) {
        delete m_shaderProgram;
        m_shaderProgram = nullptr;
    }
    if (m_presentShaderProgram) {
        delete m_presentShaderProgram;
        m_presentShaderProgram = nullptr;
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
    if (m_rgbTexture) {
        glDeleteTextures(1, &m_rgbTexture);
        m_rgbTexture = 0;
    }

    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_fullscreenVao) {
        glDeleteVertexArrays(1, &m_fullscreenVao);
        m_fullscreenVao = 0;
    }
    if (m_fullscreenVbo) {
        glDeleteBuffers(1, &m_fullscreenVbo);
        m_fullscreenVbo = 0;
    }
    if (m_rgbFbo) {
        glDeleteFramebuffers(1, &m_rgbFbo);
        m_rgbFbo = 0;
    }

    m_rgbFboWidth = 0;
    m_rgbFboHeight = 0;
    m_glInitialized = false;
}

void VideoRendererGL::start() {
    m_playing = true;
    m_renderTimer->start(m_frameInterval);
    qDebug() << "[VideoRendererGL] Start";
}

void VideoRendererGL::pause() {
    m_playing = false;
    m_buffering = false;
    m_earlyWaitPts = -1;
    m_earlyWaitLogged = false;
    m_earlyWaitTimer.invalidate();
    m_renderTimer->stop();
    qDebug() << "[VideoRendererGL] Pause";
}

void VideoRendererGL::stop() {
    m_playing = false;
    m_buffering = false;
    m_earlyWaitPts = -1;
    m_earlyWaitLogged = false;
    m_earlyWaitTimer.invalidate();
    m_renderTimer->stop();
    m_lastPTS = 0;

    if (m_currentFrame) {
        delete m_currentFrame;
        m_currentFrame = nullptr;
    }
    m_frameUpdated = false;
    m_videoSize = QSize(-1, -1);
    m_fullscreenTransitionActive = false;
    m_pendingRenderTargetSize = QSize();
    qDebug() << "[VideoRendererGL] Stop";
}

void VideoRendererGL::startBuffering() {
    const int dynamicThreshold = qBound(3, m_targetFPS / 6, 8);
    m_bufferingThreshold = dynamicThreshold;
    m_buffering = true;
    m_bufferingTimeoutLogged = false;
    m_bufferingTimer.restart();
    qDebug() << "[VideoRendererGL] Start buffering, threshold:" << m_bufferingThreshold
             << "timeout:" << m_bufferingTimeoutMs << "ms";
}

void VideoRendererGL::setVideoBuffer(VideoBuffer* buffer) {
    m_buffer = buffer;
}

void VideoRendererGL::setTargetFPS(int fps) {
    if (fps <= 0) {
        return;
    }

    m_targetFPS = fps;
    m_frameInterval = 1000 / fps;

    if (m_playing) {
        m_renderTimer->setInterval(m_frameInterval);
    }

    qDebug() << "[VideoRendererGL] Target FPS:" << fps;
}

void VideoRendererGL::setPixelAspectRatio(int num, int den) {
    float newPar = 1.0f;
    if (num > 0 && den > 0) {
        newPar = static_cast<float>(num) / static_cast<float>(den);
    }

    if (newPar < 0.01f || newPar > 100.0f) {
        qWarning() << "[VideoRendererGL] Invalid pixel aspect ratio" << num << "/" << den;
        newPar = 1.0f;
    }

    m_pixelAspectRatio = newPar;
    if (m_videoSize.width() > 0 && m_videoSize.height() > 0) {
        updateVertexBuffer(this->width(), this->height());
        update();
    }
}

void VideoRendererGL::setDisplayMode(int mode) {
    const int normalized = (mode == 1) ? 1 : 0;
    const bool changed = (m_displayMode != normalized);

    m_displayMode = normalized;
    qDebug() << "[VideoRendererGL] Display mode" << (changed ? "changed:" : "reapplied:")
             << (m_displayMode == 0 ? "Fit" : "Fill");

    if (m_videoSize.width() > 0 && m_videoSize.height() > 0) {
        updateVertexBuffer(this->width(), this->height());
        update();
    }
}

void VideoRendererGL::setFullscreenTransitionActive(bool active) {
    if (m_fullscreenTransitionActive == active) {
        return;
    }

    m_fullscreenTransitionActive = active;
    if (!m_fullscreenTransitionActive && m_pendingRenderTargetSize.isValid()) {
        qDebug()
            << "[VideoRendererGL] Fullscreen transition settled, applying deferred render target"
            << m_pendingRenderTargetSize.width() << "x" << m_pendingRenderTargetSize.height();
    }
    update();
}

void VideoRendererGL::setQualityPreset(int preset) {
    QualityPreset normalized =
        (preset == static_cast<int>(Enhanced2K)) ? Enhanced2K : Standard1080P;
    if (m_qualityPreset == normalized) {
        return;
    }

    m_qualityPreset = normalized;
    if (m_qualityPreset == Enhanced2K) {
        m_renderMaxWidth = 2560;
        m_renderMaxHeight = 1440;
        m_sharpenStrength = 0.18f;
        m_enableHighQualityScaling = true;
    } else {
        m_renderMaxWidth = 1920;
        m_renderMaxHeight = 1080;
        m_sharpenStrength = 0.08f;
        m_enableHighQualityScaling = false;
    }

    if (context()) {
        makeCurrent();
        if (m_rgbFbo) {
            glDeleteFramebuffers(1, &m_rgbFbo);
            m_rgbFbo = 0;
        }
        if (m_rgbTexture) {
            glDeleteTextures(1, &m_rgbTexture);
            m_rgbTexture = 0;
        }
        m_rgbFboWidth = 0;
        m_rgbFboHeight = 0;
        doneCurrent();
    }

    qDebug() << "[VideoRendererGL] Quality preset set to"
             << (m_qualityPreset == Enhanced2K ? "Enhanced2K" : "Standard1080P")
             << "max:" << m_renderMaxWidth << "x" << m_renderMaxHeight
             << "hqScaling:" << m_enableHighQualityScaling << "sharpen:" << m_sharpenStrength;

    update();
}

void VideoRendererGL::renderNextFrame() {
    static constexpr qint64 kRenderAheadBaseMs = 120;
    static constexpr qint64 kDropLateThresholdBaseMs = 350;

    if (!m_playing || !m_buffer) {
        return;
    }

    if (m_buffering) {
        const int bufferSize = m_buffer->size();
        const bool enoughFrames = bufferSize >= m_bufferingThreshold;
        const bool timeoutReached =
            m_bufferingTimer.isValid() && m_bufferingTimer.elapsed() >= m_bufferingTimeoutMs;

        if (enoughFrames || (timeoutReached && bufferSize > 0)) {
            m_buffering = false;
            qDebug() << "[VideoRendererGL] Buffering complete, size:" << bufferSize
                     << "elapsed:" << (m_bufferingTimer.isValid() ? m_bufferingTimer.elapsed() : 0)
                     << "ms";
        } else {
            if (timeoutReached && bufferSize == 0 && !m_bufferingTimeoutLogged) {
                m_bufferingTimeoutLogged = true;
                qWarning() << "[VideoRendererGL] Buffering timeout with empty queue,"
                           << "waiting for decoder frames";
            }
            return;
        }
    }

    qint64 audioClock = 0;
    double playbackRate = 1.0;
    if (AudioPlayer::instance().isPlaying()) {
        playbackRate = AudioPlayer::instance().playbackRate();
        audioClock = AudioPlayer::instance().getSyncPlaybackPosition();
    } else {
        audioClock = m_lastPTS;
    }

    const bool highSpeedMode = playbackRate >= 1.5;

    qint64 renderAheadMs = kRenderAheadBaseMs;
    if (playbackRate > 1.01) {
        renderAheadMs = static_cast<qint64>(kRenderAheadBaseMs + (playbackRate - 1.0) * 260.0);
        renderAheadMs = qBound<qint64>(220, renderAheadMs, 420);
    }
    if (highSpeedMode) {
        // 高倍速优先流畅度：尽量不因“视频略超前”而阻塞渲染。
        renderAheadMs = 1200;
    }

    qint64 dropLateThresholdMs = kDropLateThresholdBaseMs;
    if (playbackRate > 1.01) {
        dropLateThresholdMs = 260;
    }
    if (highSpeedMode) {
        dropLateThresholdMs = 140;
    }

    const int maxDropPerTick = highSpeedMode ? 40 : 10;

    VideoFrame* frame = m_buffer->peek();
    if (!frame) {
        return;
    }

    if (!highSpeedMode && frame->pts > audioClock + renderAheadMs) {
        if (!m_earlyWaitTimer.isValid()) {
            m_earlyWaitPts = frame->pts;
            m_earlyWaitLogged = false;
            m_earlyWaitTimer.restart();
            return;
        }

        const int earlyWaitTimeoutMs = (playbackRate > 1.01) ? 90 : 260;
        const bool waitTooLong =
            m_earlyWaitTimer.isValid() && m_earlyWaitTimer.elapsed() >= earlyWaitTimeoutMs;
        if (!waitTooLong) {
            return;
        }

        if (!m_earlyWaitLogged) {
            m_earlyWaitLogged = true;
            qWarning() << "[VideoRendererGL] Force render after early-frame stall"
                       << "firstFramePts:" << m_earlyWaitPts << "currentFramePts:" << frame->pts
                       << "audioClock:" << audioClock << "aheadThreshold:" << renderAheadMs
                       << "rate:" << playbackRate << "waited:" << m_earlyWaitTimer.elapsed()
                       << "ms";
        }
    }
    m_earlyWaitPts = -1;
    m_earlyWaitLogged = false;
    m_earlyWaitTimer.invalidate();

    int skippedCount = 0;
    while (frame && audioClock > frame->pts + dropLateThresholdMs) {
        ++skippedCount;
        m_buffer->pop();
        delete frame;
        frame = m_buffer->peek();
        if (skippedCount >= maxDropPerTick) {
            break;
        }
    }

    if (!frame) {
        return;
    }

    frame = m_buffer->pop();
    if (!frame) {
        return;
    }

    if (m_videoSize.width() != frame->width || m_videoSize.height() != frame->height) {
        m_videoSize = QSize(frame->width, frame->height);
        updateVertexBuffer(this->width(), this->height());
        emit videoSizeChanged(m_videoSize);
    }

    VideoFrame* oldFrame = m_currentFrame;
    m_currentFrame = frame;
    m_lastPTS = frame->pts;
    m_frameUpdated = true;

    if (oldFrame) {
        delete oldFrame;
        oldFrame = nullptr;
    }

    update();
    emit frameRendered(frame->pts);
}
