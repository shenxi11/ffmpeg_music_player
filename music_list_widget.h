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

public slots:
    void receive_song_op(bool flag, QString fileName);

    void _play_click(QString songName);

    void _remove_click(QString songName);
signals:
    void selectMusic(QString path);

    void play_click(QString songName);

    void remove_click(QString songName);
private:
    std::map<QString,MusicItem*>pathMap;

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);

    void paintEvent(QPaintEvent *event) override
        {
            QPainter painter(viewport());
            painter.fillRect(rect(), QColor(248, 248, 255));  // 使用背景颜色填充整个窗口

            // 调用父类的 paintEvent 以确保绘制子项等内容
            QListWidget::paintEvent(event);
        }
};

#endif // MUSICLISTWIDGET_H
