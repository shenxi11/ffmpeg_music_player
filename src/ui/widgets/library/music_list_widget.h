#ifndef MUSICLISTWIDGET_H
#define MUSICLISTWIDGET_H

#include <QObject>
#include <QWidget>
#include <QListWidget>
#include <QMouseEvent>
#include "headers.h"
#include "music_item.h"
#include "music_item_qml.h"
class MusicListWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit MusicListWidget(QWidget *parent = nullptr);


public slots:
    void receiveSongOp(bool flag, QString fileName);
    void playClick(QString songPath);
    void removeClick(QString songPath);
    void lastClick(QString songName);
    void nextClick(QString songName);

    void onAddSong(const QString songName,const QString path, bool isNetMusic = false);
    void onAddSonglist(const QStringList filename_list, const QList<double> duration);
    void removeAll();
signals:
    void selectMusic(QString path);
    void play_click(QString songName);
    void remove_click(QString songName);
    void signalDownloadClick(QString songName);
private:
    std::map<QString, QWidget*> pathMap;  // 改为 QWidget* 以兼容 MusicItem 和 MusicItemQml
protected:

    void paintEvent(QPaintEvent *event) override
        {
            QPainter painter(viewport());
            painter.setRenderHint(QPainter::Antialiasing);

            QColor backgroundColor("#F7F9FC");
            painter.fillRect(this->rect(), backgroundColor);

            QListWidget::paintEvent(event);
        }
};

#endif // MUSICLISTWIDGET_H
