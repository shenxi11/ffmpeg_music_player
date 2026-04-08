#include "music_list_widget_local.h"

#include <QDebug>
#include <QFileInfo>
#include <QVBoxLayout>

#include "play_widget.h"

MusicListWidgetLocal::MusicListWidgetLocal(QWidget* parent)
    : QWidget(parent)
{
    listWidget = new MusicListWidgetQml(false, this);
    m_viewModel = new LocalMusicListViewModel(this);

    QVBoxLayout* v_layout = new QVBoxLayout(this);
    v_layout->setContentsMargins(0, 0, 0, 0);
    v_layout->addWidget(listWidget);
    setLayout(v_layout);

    setupConnections();
    m_viewModel->refreshLocalMusicPaths();
}

void MusicListWidgetLocal::onAddButtonClicked()
{
    emit signalAddButtonClicked();
}

void MusicListWidgetLocal::onAddSong(const QString filename, const QString path)
{
    emit signalAddSong(filename, path);
}

void MusicListWidgetLocal::onPlayButtonClick(bool flag, const QString filename)
{
    qDebug() << "[PLAY_STATE] MusicListWidgetLocal::onPlayButtonClick 收到信号, flag="
             << flag << ", filename=" << filename;
    if (auto* sender_ = qobject_cast<PlayWidget*>(sender())) {
        qDebug() << "[PLAY_STATE] sender 是PlayWidget, net_flag=" << sender_->getNetFlag();
        if (!sender_->getNetFlag()) {
            listWidget->setPlayingState(filename, flag);
            emit signalPlayButtonClick(flag, filename);
        }
    }
}

void MusicListWidgetLocal::onPlayClick(const QString songName)
{
    emit signalPlayClick(songName, false);
}

void MusicListWidgetLocal::onRemoveClick(const QString songeName)
{
    emit signalRemoveClick(songeName);
}

void MusicListWidgetLocal::onTranslateButtonClicked()
{
    emit signalTranslateButtonClicked();
}

void MusicListWidgetLocal::onUpdateMetadata(QString filePath, QString coverUrl, QString duration,
                                            QString artist)
{
    qDebug() << "[METADATA] MusicListWidgetLocal 收到元数据更新:" << filePath << coverUrl
             << duration << artist;
    listWidget->updateSongMetadata(filePath, coverUrl, duration, artist);
}
