#include "settings_widget.h"

SettingsWidget::SettingsWidget(QWidget *parent)
    : QQuickWidget(parent)
{
    qDebug() << "[SettingsWidget] Initializing...";

    engine()->addImportPath("qrc:/");
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/components/Settings.qml"));

    if (status() == QQuickWidget::Error) {
        qWarning() << "[SettingsWidget] Failed to load Settings.qml";
        return;
    }

    qDebug() << "[SettingsWidget] QML loaded successfully";

    QQuickItem* root = rootObject();
    if (root) {
        SettingsManager& settings = SettingsManager::instance();
        QMetaObject::invokeMethod(root, "setDownloadPath", Q_ARG(QVariant, settings.downloadPath()));
        QMetaObject::invokeMethod(root, "setDownloadLyrics", Q_ARG(QVariant, settings.downloadLyrics()));
        QMetaObject::invokeMethod(root, "setDownloadCover", Q_ARG(QVariant, settings.downloadCover()));
        QMetaObject::invokeMethod(root, "setLogPath", Q_ARG(QVariant, settings.logPath()));

        connect(root, SIGNAL(chooseDownloadPath()), this, SLOT(onChooseDownloadPath()));
        connect(root, SIGNAL(chooseLogPath()), this, SLOT(onChooseLogPath()));
        connect(root, SIGNAL(settingsClosed()), this, SLOT(close()));

        connect(root, SIGNAL(downloadPathChanged()), this, SLOT(onDownloadPathChanged()));
        connect(root, SIGNAL(downloadLyricsChanged()), this, SLOT(onDownloadLyricsChanged()));
        connect(root, SIGNAL(downloadCoverChanged()), this, SLOT(onDownloadCoverChanged()));
        connect(root, SIGNAL(logPathChanged()), this, SLOT(onLogPathChanged()));

        qDebug() << "[SettingsWidget] Signals connected";
    }

    setWindowTitle("设置");
    setFixedSize(600, 560);
    setAttribute(Qt::WA_DeleteOnClose, false);

    qDebug() << "[SettingsWidget] Initialization complete";
}

void SettingsWidget::onChooseDownloadPath()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        "选择下载保存路径",
        SettingsManager::instance().downloadPath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!dir.isEmpty()) {
        qDebug() << "[SettingsWidget] Download path selected:" << dir;

        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setDownloadPath", Q_ARG(QVariant, dir));
        }

        SettingsManager::instance().setDownloadPath(dir);
    }
}

void SettingsWidget::onChooseLogPath()
{
    const QString currentPath = SettingsManager::instance().logPath();
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "选择日志文件路径",
        currentPath,
        "Text File (*.txt);;All Files (*)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    QFileInfo info(filePath);
    if (info.suffix().isEmpty()) {
        filePath += ".txt";
    }

    qDebug() << "[SettingsWidget] Log path selected:" << filePath;

    QQuickItem* root = rootObject();
    if (root) {
        QMetaObject::invokeMethod(root, "setLogPath", Q_ARG(QVariant, filePath));
    }

    SettingsManager::instance().setLogPath(filePath);
}

void SettingsWidget::onDownloadPathChanged()
{
    QQuickItem* root = rootObject();
    if (root) {
        const QString path = root->property("downloadPath").toString();
        SettingsManager::instance().setDownloadPath(path);
        qDebug() << "[SettingsWidget] Download path changed:" << path;
    }
}

void SettingsWidget::onDownloadLyricsChanged()
{
    QQuickItem* root = rootObject();
    if (root) {
        const bool enable = root->property("downloadLyrics").toBool();
        SettingsManager::instance().setDownloadLyrics(enable);
        qDebug() << "[SettingsWidget] Download lyrics changed:" << enable;
    }
}

void SettingsWidget::onDownloadCoverChanged()
{
    QQuickItem* root = rootObject();
    if (root) {
        const bool enable = root->property("downloadCover").toBool();
        SettingsManager::instance().setDownloadCover(enable);
        qDebug() << "[SettingsWidget] Download cover changed:" << enable;
    }
}

void SettingsWidget::onLogPathChanged()
{
    QQuickItem* root = rootObject();
    if (root) {
        const QString path = root->property("logPath").toString();
        SettingsManager::instance().setLogPath(path);
        qDebug() << "[SettingsWidget] Log path changed:" << path;
    }
}
