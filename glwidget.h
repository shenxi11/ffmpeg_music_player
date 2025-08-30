// video_widget.h
#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QImage>
#include "headers.h"

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit GLWidget(QWidget *parent = nullptr);
    ~GLWidget();

public slots:
    void slot_receive(const QImage& frame);  // 保留原有接口
    void slot_update(const QImage& frame);
signals:
    void signal_updateFrame();  // 内部刷新信号
    void signal_update(const QImage& frame);
protected:
    // OpenGL核心函数
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    // OpenGL资源
    GLuint m_texture = 0;
    QOpenGLShaderProgram m_shaderProgram;

    // 帧处理
    QQueue<QImage> m_frameQueue;
    std::mutex m_mutex;
    std::condition_variable m_frameAvailable;

    // 线程控制
    std::atomic_bool m_running{true};
    std::thread m_renderThread;

    // 辅助函数
    void initShaders();
    void uploadTexture(const QImage& frame);
};
