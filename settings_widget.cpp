#include "settings_widget.h"

SettingsWidget::SettingsWidget(QWidget *parent)
    : QQuickWidget(parent)
{
    qDebug() << "[SettingsWidget] Initializing...";
    
    // 配置QML引擎
    engine()->addImportPath("qrc:/");
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    
    // 加载QML文件
    setSource(QUrl("qrc:/qml/components/Settings.qml"));
    
    if (status() == QQuickWidget::Error) {
        qWarning() << "[SettingsWidget] Failed to load Settings.qml";
        return;
    }
    
    qDebug() << "[SettingsWidget] QML loaded successfully";
    
    // 连接信号
    QQuickItem* root = rootObject();
    if (root) {
        // 从设置管理器加载初始值
        SettingsManager& settings = SettingsManager::instance();
        QMetaObject::invokeMethod(root, "setDownloadPath",
            Q_ARG(QVariant, settings.downloadPath()));
        QMetaObject::invokeMethod(root, "setDownloadLyrics",
            Q_ARG(QVariant, settings.downloadLyrics()));
        QMetaObject::invokeMethod(root, "setDownloadCover",
            Q_ARG(QVariant, settings.downloadCover()));
        
        // 连接选择下载路径信号
        connect(root, SIGNAL(chooseDownloadPath()),
                this, SLOT(onChooseDownloadPath()));
        
        // 连接关闭信号
        connect(root, SIGNAL(settingsClosed()),
                this, SLOT(close()));
        
        // 监听属性变化并保存到设置管理器
        connect(root, SIGNAL(downloadPathChanged()),
                this, SLOT(onDownloadPathChanged()));
        connect(root, SIGNAL(downloadLyricsChanged()),
                this, SLOT(onDownloadLyricsChanged()));
        connect(root, SIGNAL(downloadCoverChanged()),
                this, SLOT(onDownloadCoverChanged()));
        
        qDebug() << "[SettingsWidget] Signals connected";
    }
    
    // 设置窗口属性
    setWindowTitle("设置");
    setFixedSize(600, 500);
    setAttribute(Qt::WA_DeleteOnClose, false);  // 关闭时不删除
    
    qDebug() << "[SettingsWidget] Initialization complete";
}

void SettingsWidget::onChooseDownloadPath()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "选择下载保存路径",
        SettingsManager::instance().downloadPath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (!dir.isEmpty()) {
        qDebug() << "[SettingsWidget] Download path selected:" << dir;
        
        // 更新QML显示
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setDownloadPath",
                Q_ARG(QVariant, dir));
        }
        
        // 保存到设置管理器
        SettingsManager::instance().setDownloadPath(dir);
    }
}

void SettingsWidget::onDownloadPathChanged()
{
    QQuickItem* root = rootObject();
    if (root) {
        QString path = root->property("downloadPath").toString();
        SettingsManager::instance().setDownloadPath(path);
        qDebug() << "[SettingsWidget] Download path changed:" << path;
    }
}

void SettingsWidget::onDownloadLyricsChanged()
{
    QQuickItem* root = rootObject();
    if (root) {
        bool enable = root->property("downloadLyrics").toBool();
        SettingsManager::instance().setDownloadLyrics(enable);
        qDebug() << "[SettingsWidget] Download lyrics changed:" << enable;
    }
}

void SettingsWidget::onDownloadCoverChanged()
{
    QQuickItem* root = rootObject();
    if (root) {
        bool enable = root->property("downloadCover").toBool();
        SettingsManager::instance().setDownloadCover(enable);
        qDebug() << "[SettingsWidget] Download cover changed:" << enable;
    }
}
