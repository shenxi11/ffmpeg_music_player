// video_widget.cpp
#include "glwidget.h"
#include <QPainter>
#include <memory>
#include <mutex>
#include <chrono>

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    // 设置OpenGL版本
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);
    connect(this, &GLWidget::signal_update, this, &GLWidget::slot_update);

    m_renderThread = std::thread([this]{
        while(m_running) {
            QImage frame;
            {
                std::unique_lock<std::mutex> locker(m_mutex);
                m_frameAvailable.wait(locker, [this]{
                    return !m_frameQueue.empty() || !m_running;
                });

                if(!m_running) break;
                frame = m_frameQueue.front();
                m_frameQueue.pop_front();
            }
            emit signal_update(frame);

            QThread::msleep(500);
        }
    });
}
void GLWidget::slot_update(const QImage& frame){
    makeCurrent();
    if(m_texture == 0) {
        initializeOpenGLFunctions();
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    uploadTexture(frame);
    doneCurrent();
    update();
}
GLWidget::~GLWidget() {
    m_running = false;
    m_frameAvailable.notify_all();
    if(m_renderThread.joinable()) {
        m_renderThread.join();
    }

    makeCurrent();
    glDeleteTextures(1, &m_texture);
    doneCurrent();
}

void GLWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    initShaders();
}

void GLWidget::initShaders() {
    const char *vert = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    const char *frag = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D tex;
        void main() {
            FragColor = texture(tex, TexCoord);
        }
    )";

    m_shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vert);
    m_shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, frag);
    m_shaderProgram.link();
}

void GLWidget::uploadTexture(const QImage& frame) {
    QImage img = frame.convertToFormat(QImage::Format_RGBA8888);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                img.width(), img.height(), 0,
                GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
}

void GLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);

    m_shaderProgram.bind();
    glBindTexture(GL_TEXTURE_2D, m_texture);

    const GLfloat vertices[] = {
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDeleteBuffers(1, &vbo);
    m_shaderProgram.release();
}

void GLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void GLWidget::slot_receive(const QImage& frame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_frameQueue.enqueue(frame);
    m_frameAvailable.notify_one();
}
