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
    void receive_song_op(bool flag, QString fileName);
    void _play_click(QString songPath);
    void _remove_click(QString songPath);
    void Last_click(QString songName);
    void Next_click(QString songName);

    void on_signal_add_song(const QString songName,const QString path, bool isNetMusic = false);
    void on_signal_add_songlist(const QStringList filename_list, const QList<double> duration);
    void remove_all();
signals:
    void selectMusic(QString path);
    void play_click(QString songName);
    void remove_click(QString songName);
    void signal_download_click(QString songName);
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
