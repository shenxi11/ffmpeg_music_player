#include "play_history_widget.h"

PlayHistoryWidget::PlayHistoryWidget(QWidget *parent)
    : QQuickWidget(parent)
{
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/components/PlayHistoryList.qml"));
    
    setAttribute(Qt::WA_TranslucentBackground);
    
    qDebug() << "PlayHistoryWidget: Created";
    
    // 连接QML信号
    QQuickItem* root = rootObject();
    if (root) {
        connect(root, SIGNAL(playMusic(QString)),
                this, SIGNAL(playMusic(QString)));
        connect(root, SIGNAL(playMusicWithMetadata(QString,QString,QString,QString)),
                this, SIGNAL(playMusicWithMetadata(QString,QString,QString,QString)));
        connect(root, SIGNAL(deleteHistory(QVariant)),
                this, SLOT(handleDeleteHistory(QVariant)));
        connect(root, SIGNAL(loginRequested()),
                this, SIGNAL(loginRequested()));
        connect(root, SIGNAL(refreshRequested()),
                this, SIGNAL(refreshRequested()));
        qDebug() << "PlayHistoryWidget: Signals connected";
    } else {
        qDebug() << "PlayHistoryWidget: ERROR - Root object is null!";
    }
}

void PlayHistoryWidget::setLoggedIn(bool loggedIn, const QString& userAccount)
{
    QQuickItem* root = rootObject();
    if (root) {
        root->setProperty("isLoggedIn", loggedIn);
        root->setProperty("userAccount", userAccount);
        qDebug() << "PlayHistoryWidget: Set logged in:" << loggedIn << "account:" << userAccount;
    }
}

void PlayHistoryWidget::loadHistory(const QVariantList& historyData)
{
    QQuickItem* root = rootObject();
    if (root) {
        QMetaObject::invokeMethod(root, "loadHistory",
            Q_ARG(QVariant, QVariant::fromValue(historyData)));
        qDebug() << "PlayHistoryWidget: Loaded" << historyData.size() << "history items";
    }
}

void PlayHistoryWidget::setCurrentPlayingPath(const QString& filePath)
{
    QQuickItem* root = rootObject();
    if (root) {
        qDebug() << "[PlayHistoryWidget] Setting currentPlayingPath to:" << filePath;
        root->setProperty("currentPlayingPath", filePath);
        QString currentValue = root->property("currentPlayingPath").toString();
        qDebug() << "[PlayHistoryWidget] Verified currentPlayingPath is now:" << currentValue;
    } else {
        qDebug() << "[PlayHistoryWidget] WARNING: root object is null!";
    }
}

void PlayHistoryWidget::setPlayingState(const QString& filePath, bool playing)
{
    QQuickItem* root = rootObject();
    if (!root) {
        qDebug() << "[PlayHistoryWidget] WARNING: root object is null when setting playing state";
        return;
    }

    QMetaObject::invokeMethod(root, "setPlayingState",
                              Q_ARG(QVariant, filePath),
                              Q_ARG(QVariant, playing));
}

void PlayHistoryWidget::clearHistory()
{
    QQuickItem* root = rootObject();
    if (root) {
        QMetaObject::invokeMethod(root, "clearHistory");
    }
}

void PlayHistoryWidget::handleDeleteHistory(const QVariant& selectedPaths)
{
    QStringList paths;
    QVariantList variantList = selectedPaths.toList();
    for (const QVariant& item : variantList) {
        paths.append(item.toString());
    }
    emit deleteHistory(paths);
}
