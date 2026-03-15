#include "music_list_widget_local.h"

#include <QDebug>
#include <QFileInfo>

/*
模块名称: MusicListWidgetLocal 连接配置
功能概述: 统一管理本地音乐列表的交互信号与列表刷新连接。
对外接口: MusicListWidgetLocal::setupConnections()
维护说明: 列表与 ViewModel 的连接关系集中维护，避免构造函数膨胀。
*/

void MusicListWidgetLocal::setupConnections()
{
    connect(listWidget, &MusicListWidgetQml::signalAddSong,
            this, &MusicListWidgetLocal::onAddButtonClicked);
    connect(listWidget, &MusicListWidgetQml::signalPlayButtonClick,
            this, &MusicListWidgetLocal::onPlayClick);
    connect(listWidget, &MusicListWidgetQml::signalRemoveClick,
            this, &MusicListWidgetLocal::onRemoveClick);
    connect(this, &MusicListWidgetLocal::signalNext, listWidget, &MusicListWidgetQml::signalNext);
    connect(this, &MusicListWidgetLocal::signalLast, listWidget, &MusicListWidgetQml::signalLast);

    connect(this, &MusicListWidgetLocal::signalAddSong,
            this, &MusicListWidgetLocal::handleAddSongToListAndModel);

    connect(this, &MusicListWidgetLocal::signalPlayButtonClick,
            this, &MusicListWidgetLocal::handlePlayButtonStateChanged);

    connect(m_viewModel, &LocalMusicListViewModel::localMusicPathsReady, this,
            &MusicListWidgetLocal::handleLocalMusicPathsReady);
}

void MusicListWidgetLocal::handleAddSongToListAndModel(const QString& filename, const QString& path)
{
    if (m_viewModel) {
        m_viewModel->addMusic(path);
    }
    listWidget->addSong(filename, path, QString(), QStringLiteral("0:00"), QString(), QStringLiteral("-"));
}

void MusicListWidgetLocal::handlePlayButtonStateChanged(bool flag, const QString& filename)
{
    listWidget->setPlayingState(filename, flag);
}

void MusicListWidgetLocal::handleLocalMusicPathsReady(const QStringList& musicPaths)
{
    qDebug() << __FUNCTION__ << "local list refresh";
    listWidget->clearAll();
    for (const QString& path : musicPaths) {
        QFileInfo info(path);
        listWidget->addSong(info.fileName(),
                            info.filePath(),
                            QString(),
                            QStringLiteral("0:00"),
                            QString(),
                            QStringLiteral("-"));
    }
}
