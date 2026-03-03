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
    
    // 閰嶇疆QML寮曟搸
    engine()->addImportPath("qrc:/");
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    
    // 灏嗘ā鍨嬫毚闇茬粰QML
    rootContext()->setContextProperty("downloadTaskModel", m_downloadTaskModel);
    rootContext()->setContextProperty("localMusicModel", m_localMusicModel);
    
    // 杩炴帴鏈湴闊充箰妯″瀷鐨勬坊鍔犺姹備俊鍙?
    connect(m_localMusicModel, &LocalMusicModel::addMusicRequested,
            this, &LocalAndDownloadWidget::addLocalMusicRequested);
    
    // 鍒濆鍖栨椂鍒锋柊娲昏穬浠诲姟鍒楄〃
    qDebug() << "[LocalAndDownloadWidget] Initial refresh of active tasks";
    m_downloadTaskModel->refresh(false);
    
    // 鍔犺浇QML鏂囦欢
    setSource(QUrl("qrc:/qml/components/library/LocalAndDownloadWidget.qml"));
    
    if (status() == QQuickWidget::Error) {
        qWarning() << "[LocalAndDownloadWidget] Failed to load QML";
        for (const auto& error : errors()) {
            qWarning() << "  " << error.toString();
        }
    }
    
    // 杩炴帴QML淇″彿鍒癈++妲?
    QQuickItem* root = rootObject();
    if (root) {
        qDebug() << "[LocalAndDownloadWidget] Root object loaded successfully";
        
        // 杩炴帴playMusic淇″彿
        connect(root, SIGNAL(playMusic(QString)), this, SLOT(onPlayMusic(QString)));
        // 杩炴帴deleteMusic淇″彿
        connect(root, SIGNAL(deleteMusic(QString)), this, SLOT(onDeleteMusic(QString)));
        // 杩炴帴addToFavorite淇″彿
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

