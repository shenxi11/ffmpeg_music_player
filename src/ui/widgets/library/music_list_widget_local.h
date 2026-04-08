#ifndef MUSICLISTWIDGETLOCAL_H
#define MUSICLISTWIDGETLOCAL_H

#include <QObject>
#include <QStringList>
#include <QWidget>

#include "music_list_widget_qml.h"
#include "viewmodels/LocalMusicListViewModel.h"

class MusicListWidgetLocal : public QWidget
{
    Q_OBJECT
public:
    explicit MusicListWidgetLocal(QWidget* parent = nullptr);

    MusicListWidgetQml* getListWidget() const { return listWidget; }

public slots:
    void onAddButtonClicked();
    void onAddSong(const QString filename, const QString path);
    void onPlayButtonClick(bool flag, const QString filename);
    void onPlayClick(const QString songName);
    void onRemoveClick(const QString songeName);
    void onTranslateButtonClicked();
    void onUpdateMetadata(QString filePath, QString coverUrl, QString duration, QString artist);

signals:
    void signalAddButtonClicked();
    void signalAddSong(const QString filename, const QString path);
    void signalPlayButtonClick(bool flag, const QString filename);
    void signalPlayClick(const QString songName, bool flag);
    void signalRemoveClick(const QString songName);
    void signalLast(QString songName);
    void signalNext(QString songName);
    void signalTranslateButtonClicked();

private:
    // 连接拆分：将列表交互信号集中在独立实现中维护。
    void setupConnections();
    void handleAddSongToListAndModel(const QString& filename, const QString& path);
    void handlePlayButtonStateChanged(bool flag, const QString& filename);
    void handleLocalMusicPathsReady(const QStringList& musicPaths);

    MusicListWidgetQml* listWidget = nullptr;
    QPushButton* add = nullptr;
    LocalMusicListViewModel* m_viewModel = nullptr;
};

#endif // MUSICLISTWIDGETLOCAL_H
