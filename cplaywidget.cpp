#include "cplaywidget.h"
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QMouseEvent>
#include <QTimer>

CPlayWidget::CPlayWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    textureUniformY = 0;
    textureUniformU = 0;
    textureUniformV = 0;
    id_y = 0;
    id_u = 0;
    id_v = 0;
    m_pBufYuv420p = NULL;
    m_pVSHader = NULL;
    m_pFSHader = NULL;
    m_pShaderProgram = NULL;
    m_pTextureY = NULL;
    m_pTextureU = NULL;
    m_pTextureV = NULL;
    m_pYuvFile = NULL;
    m_nVideoH = 0;
    m_nVideoW = 0;

}
void CPlayWidget::thread_func(){
    while(1){
        std::unique_lock lock(mtx);
        cv.wait(lock, [=]{return (!frame_queue.empty() || end_flag) && m_nVideoH && m_nVideoW;});
        if(end_flag)
            return;
        int nLen = m_nVideoW * m_nVideoH * 3 / 2;
        auto decode_data  = new unsigned char[nLen];
        auto frame = frame_queue.front();
        CopyYUV420P(frame, decode_data, m_nVideoW, m_nVideoH);
        frame_queue.pop_front();
        av_frame_free(&frame);
        data_.push_back(decode_data);
    }
}
void CPlayWidget::slot_resize(int width, int height){
    m_nVideoH = height;
    m_nVideoW = width;
    int nLen = m_nVideoW * m_nVideoH * 3 / 2;

    if (NULL == m_pBufYuv420p)
    {
        m_pBufYuv420p = new unsigned char[nLen];
        qDebug("CPlayWidget::PlayOneFrame new data memory. Len=%d width=%d height=%d\n",
               nLen, m_nVideoW, m_nVideoW);
    }
}
void CPlayWidget::slot_receiveFrame(AVFrame* frame){
    {
        std::lock_guard<std::mutex> lock(mtx);
        frame_queue.enqueue(frame);
    }
    cv.notify_one();
}
CPlayWidget::~CPlayWidget()
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        end_flag = true;
    }cv.notify_all();
}

void CPlayWidget::PlayOneFrame()
{
    std::unique_lock lock(mtx);

    if(!data_.empty()){
        if(m_pBufYuv420p)
        {
            delete  []m_pBufYuv420p;
            m_pBufYuv420p = nullptr;
        }
        m_pBufYuv420p = data_.front();
        data_.pop_front();
    }
    update();
    return;
}

void CPlayWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);

    m_pVSHader = new QOpenGLShader(QOpenGLShader::Vertex, this);

    const char *vsrc = "attribute vec4 vertexIn; \
            attribute vec2 textureIn; \
    varying vec2 textureOut;  \
    void main(void)           \
    {                         \
        gl_Position = vertexIn; \
        textureOut = textureIn; \
    }";

    bool bCompile = m_pVSHader->compileSourceCode(vsrc);
    if(!bCompile)
    {
    }

    m_pFSHader = new QOpenGLShader(QOpenGLShader::Fragment, this);

    const char *fsrc = "varying vec2 textureOut; \
            uniform sampler2D tex_y; \
    uniform sampler2D tex_u; \
    uniform sampler2D tex_v; \
    void main(void) \
    { \
        vec3 yuv; \
        vec3 rgb; \
        yuv.x = texture2D(tex_y, textureOut).r; \
        yuv.y = texture2D(tex_u, textureOut).r - 0.5; \
        yuv.z = texture2D(tex_v, textureOut).r - 0.5; \
        rgb = mat3( 1,       1,         1, \
                    0,       -0.39465,  2.03211, \
                    1.13983, -0.58060,  0) * yuv; \
        gl_FragColor = vec4(rgb, 1); \
    }";

    bCompile = m_pFSHader->compileSourceCode(fsrc);
    if(!bCompile)
    {
    }

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

    m_pShaderProgram = new QOpenGLShaderProgram;

    m_pShaderProgram->addShader(m_pFSHader);

    m_pShaderProgram->addShader(m_pVSHader);

    m_pShaderProgram->bindAttributeLocation("vertexIn", ATTRIB_VERTEX);

    m_pShaderProgram->bindAttributeLocation("textureIn", ATTRIB_TEXTURE);

    m_pShaderProgram->link();

    m_pShaderProgram->bind();

    textureUniformY = m_pShaderProgram->uniformLocation("tex_y");
    textureUniformU = m_pShaderProgram->uniformLocation("tex_u");
    textureUniformV = m_pShaderProgram->uniformLocation("tex_v");

    static const GLfloat vertexVertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, 1.0f,
    };

    static const GLfloat textureVertices[] = {
        0.0f,  1.0f,
        1.0f,  1.0f,
        0.0f,  0.0f,
        1.0f,  0.0f,
    };

    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, vertexVertices);

    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);

    glEnableVertexAttribArray(ATTRIB_VERTEX);

    glEnableVertexAttribArray(ATTRIB_TEXTURE);

    m_pTextureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureV = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureY->create();
    m_pTextureU->create();
    m_pTextureV->create();

    id_y = m_pTextureY->textureId();
    id_u = m_pTextureU->textureId();
    id_v = m_pTextureV->textureId();
    glClearColor(0.3,0.3,0.3,0.0);

    QTimer *ti = new QTimer(this);
    connect(ti, SIGNAL(timeout()), this, SLOT(PlayOneFrame()));
    ti->start(17);
}
void CPlayWidget::resizeGL(int w, int h)
{
    if (h == 0)
    {
        h = 1;
    }

    glViewport(0, 0, w, h);
}

void CPlayWidget::paintGL()
{
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, id_y);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_nVideoW, m_nVideoH, 0, GL_RED, GL_UNSIGNED_BYTE, m_pBufYuv420p);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, id_u);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_nVideoW / 2, m_nVideoH / 2, 0, GL_RED,
                 GL_UNSIGNED_BYTE, (char *)m_pBufYuv420p + m_nVideoW * m_nVideoH);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, id_v);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_nVideoW / 2, m_nVideoH / 2, 0, GL_RED,
                 GL_UNSIGNED_BYTE, (char *)m_pBufYuv420p + m_nVideoW * m_nVideoH * 5 / 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glUniform1i(textureUniformY, 0);
    glUniform1i(textureUniformU, 1);
    glUniform1i(textureUniformV, 2);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    return;
}
void CPlayWidget::CopyYUV420P(AVFrame* frame, unsigned char* dst, int width, int height) {

    if(frame->data[0]){
        unsigned char* dstY = dst;
        int yLinesize = frame->linesize[0];
        for (int i = 0; i < height; ++i) {
            int copyWidth = std::min(width, yLinesize);
            memcpy(dstY + i * width, frame->data[0] + i * yLinesize, copyWidth);
        }
        if(frame->data[1]){
            unsigned char* dstU = dstY + width * height;
            int chromaWidth = width / 2;
            int chromaHeight = height / 2;
            int uLinesize = frame->linesize[1];
            for (int i = 0; i < chromaHeight; ++i) {
                int copyWidth = std::min(chromaWidth, uLinesize);
                memcpy(dstU + i * chromaWidth,
                       frame->data[1] + i * uLinesize,
                        copyWidth);
            }
            if(frame->data[2]){
                unsigned char* dstV = dstU + chromaWidth * chromaHeight;
                int vLinesize = frame->linesize[2];
                for (int i = 0; i < chromaHeight; ++i) {
                    int copyWidth = std::min(chromaWidth, vLinesize);
                    memcpy(dstV + i * chromaWidth,
                           frame->data[2] + i * vLinesize,
                            copyWidth);
                }
            }
        }
    }
}
