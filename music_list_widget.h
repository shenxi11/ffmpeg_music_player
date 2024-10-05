#ifndef MUSICLISTWIDGET_H
#define MUSICLISTWIDGET_H

#include <QObject>
#include <QWidget>
#include <QListWidget>
#include <QMouseEvent>
#include "headers.h"
#include "music_item.h"
class MusicListWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit MusicListWidget(QWidget *parent = nullptr);

    void addSong(const QString songName,const QString path);


signals:
    void selectMusic(QString path);
private:
    std::map<QString,MusicItem*>pathMap;
protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
};

#endif // MUSICLISTWIDGET_H
