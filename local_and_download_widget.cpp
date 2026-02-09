#include "local_and_download_widget.h"
#include "local_music_cache.h"
#include <QDebug>
#include <QQuickItem>

LocalAndDownloadWidget::LocalAndDownloadWidget(QWidget *parent)
    : QQuickWidget(parent)
    , m_downloadTaskModel(new DownloadTaskModel(this))
    , m_localMusicModel(new LocalMusicModel(this))
{
    qDebug() << "[LocalAndDownloadWidget] Initializing...";
    
    // 配置QML引擎
    engine()->addImportPath("qrc:/");
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    
    // 将模型暴露给QML
    rootContext()->setContextProperty("downloadTaskModel", m_downloadTaskModel);
    rootContext()->setContextProperty("localMusicModel", m_localMusicModel);
    
    // 连接本地音乐模型的添加请求信号
    connect(m_localMusicModel, &LocalMusicModel::addMusicRequested,
            this, &LocalAndDownloadWidget::addLocalMusicRequested);
    
    // 初始化时刷新活跃任务列表
    qDebug() << "[LocalAndDownloadWidget] Initial refresh of active tasks";
    m_downloadTaskModel->refresh(false);
    
    // 加载QML文件
    setSource(QUrl("qrc:/qml/components/LocalAndDownloadWidget.qml"));
    
    if (status() == QQuickWidget::Error) {
        qWarning() << "[LocalAndDownloadWidget] Failed to load QML";
        for (const auto& error : errors()) {
            qWarning() << "  " << error.toString();
        }
    }
    
    // 连接QML信号到C++槽
    QQuickItem* root = rootObject();
    if (root) {
        qDebug() << "[LocalAndDownloadWidget] Root object loaded successfully";
        
        // 连接playMusic信号
        connect(root, SIGNAL(playMusic(QString)), this, SLOT(onPlayMusic(QString)));
        // 连接deleteMusic信号
        connect(root, SIGNAL(deleteMusic(QString)), this, SLOT(onDeleteMusic(QString)));
        // 连接addToFavorite信号
        connect(root, SIGNAL(addToFavorite(QString,QString,QString,QString)), 
                this, SIGNAL(addToFavorite(QString,QString,QString,QString)));
        
        qDebug() << "[LocalAndDownloadWidget] Signals connected";
    } else {
        qWarning() << "[LocalAndDownloadWidget] Root object is null";
    }
    
    qDebug() << "[LocalAndDownloadWidget] Initialized successfully";
}

void LocalAndDownloadWidget::onPlayMusic(const QString& filename)
{
    qDebug() << "[LocalAndDownloadWidget] Play music:" << filename;
    emit playMusic(filename);
}

void LocalAndDownloadWidget::onDeleteMusic(const QString& filename)
{
    qDebug() << "[LocalAndDownloadWidget] Delete music:" << filename;
    emit deleteMusic(filename);
}

void LocalAndDownloadWidget::setCurrentPlayingPath(const QString& path)
{
    if (m_downloadTaskModel) {
        m_downloadTaskModel->setCurrentPlayingPath(path);
    }
    if (m_localMusicModel) {
        m_localMusicModel->setCurrentPlayingPath(path);
    }
}
